


#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "message_buffer.h"

#include "loglevels.h"
#include "config.h"

#include "interpreter.h"
#include "ads129x.h"
#include "audio.h"
#include "data_log.h"
#include "eeg_reader.h"
#include "app.h"
#include "eeg_datatypes.h"
#include "data_log.h"
#include "string_util.h"
#include "ff.h"
#include "fatfs_utils.h"
#include "command_parser.h"
#include "ble.h"
#include "string_util.h"
#include "ble_uart_send.h"
#include "eeg_processor.h"
#include "erp.h"
#include "rtc.h"
#include "accel.h"
#include "system_monitor.h"
#include "hrm.h"
#include "data_log_commands.h"
#include "ml.h"

#if (defined(ENABLE_INTERPRETER_TASK) && (ENABLE_INTERPRETER_TASK > 0U))

static const char *TAG = "interpreter";	// Logging prefix for this module

#define INTERPRETER_EVENT_QUEUE_SIZE (10)

#define MAX_PATH_LEN (256)

#define INTERPRETER_READ_BUFFER_CAPACITY (MAX_PATH_LEN)

#define SCRIPTNAME_MBUF_NUM_MSG (INTERPRETER_EVENT_QUEUE_SIZE/2)
#define SCRIPTNAME_MBUF_MSG_LEN (MAX_PATH_LEN+4) // bytes+4 overhead bytes
#define SCRIPTNAME_MBUF_SIZE_BYTES ((SCRIPTNAME_MBUF_NUM_MSG*SCRIPTNAME_MBUF_MSG_LEN)+1) // bytes

// When trying to set "xTimeInMs" to 26999000 (~7.5 hours), the default pdMS_TO_TICKS() macro multiplies 26999000 by 200, which is equal to 5399800000
// 5399800000 is bigger than the value that can be stored by TickType_t (uint32_t)
// To prevent rounding errors, we use the original multiply-then-divide order used by pdMS_TO_TICKS() if we are under some threshold.
// If over the threshold, we reverse the order to divide-them-multiply.
// We choose a threshold that tries to preserve accuracy, currently equal to (2^32 - 2^14)
// Instead of 2^14, we could use the tighter bound 2^8, which is close to the multiplier 200hz, the current configTICK_RATE_HZ.
#define pdMS_TO_TICKS_SAFE( xTimeInMs )    ( ( xTimeInMs < 262144 ) ? \
                                             ( ( TickType_t ) ( ( ( TickType_t ) ( xTimeInMs ) * ( TickType_t ) configTICK_RATE_HZ ) / ( TickType_t ) 1000U ) ) \
                                             : \
                                             ( ( TickType_t ) ( ( ( TickType_t ) ( xTimeInMs ) / ( TickType_t ) 1000U ) * ( TickType_t ) configTICK_RATE_HZ ) ) )

typedef enum{
  BLINK_NONE = 0,
  BLINK_MISSED = 1,
  BLINK_GOOD = 2
} blink_status_t;


const char* prompt_wav_filepath = "/audio/BLINK_22M.wav";
const unsigned long prompt_wav_dur_ms = 16000;

const char* pip_wav_filepath = "/audio/PIP_22M.wav";
const unsigned long pip_dur_ms = 2000;

const char* blink_success_wav_filepath = "/audio/SUCCESS_22M.wav";
const unsigned long blink_success_wav_dur_ms = 8000;

const char* blink_failed_wav_filepath = "/audio/FAIL_22M.wav";
const unsigned long blink_failed_wav_dur_ms = 13000;


#define NUM_BLINK_PROBES 10
#define NUM_BLINK_FOR_SUCCESS 7


//
// Task events:
//
typedef enum
{
  INTERPRETER_EVENT_ENTER_STATE,	// (used for state transitions)
  INTERPRETER_EVENT_INTRO_WAV_TIMEOUT,
  INTERPRETER_EVENT_START,
  INTERPRETER_EVENT_NATURAL_STOP,
  INTERPRETER_EVENT_FORCED_STOP,
  INTERPRETER_EVENT_STIMULATION_TIMEOUT,
  INTERPRETER_EVENT_DURATION_TIMEOUT,
  INTERPRETER_EVENT_AUDIO_STOP_TIMEOUT,
  INTERPRETER_EVENT_START_SCRIPT,
  INTERPRETER_EVENT_STOP_SCRIPT,
  INTERPRETER_EVENT_SCRIPT_COMMAND_COMPLETE,
  INTERPRETER_EVENT_DELAY_TIMEOUT,
  INTERPRETER_EVENT_TIMER1_TIMEOUT,
  INTERPRETER_EVENT_START_DELAY_TIMER,
  INTERPRETER_EVENT_START_TIMER1_TIMER,
  INTERPRETER_EVENT_WAIT_TIMER1_TIMER,
  INTERPRETER_EVENT_RTC_ALARM,
  INTERPRETER_EVENT_BLINK_START_TEST,
  INTERPRETER_EVENT_BLINK_STOP_TEST,
  INTERPRETER_EVENT_BLINK_PLAY_PROMPT,
  INTERPRETER_EVENT_BLINK_PLAY_PIP,
  INTERPRETER_EVENT_BLINK_PLAY_SUCCESS,
  INTERPRETER_EVENT_BLINK_PLAY_FAIL,
  INTERPRETER_EVENT_BLINK_DETECTED,
} interpreter_event_type_t;


// Events are passed to the g_event_queue with an optional 
// void *user_data pointer (which may be NULL).
typedef struct
{
  interpreter_event_type_t type;

  union{
    therapy_type_t therapy_type;
    struct {
      unsigned long time_ms;
    } timer;
    bool start_electrode_quality;
  } data;

} interpreter_event_t;

typedef enum
{
  BLINK_STATE_STANDBY = 0,
  BLINK_STATE_WAIT_PROMPT,
  BLINK_STATE_WAIT_PIP,
  BLINK_STATE_WAIT_SUCCESS,
  BLINK_STATE_WAIT_FAIL,
} blink_state_t;

//
// Global context data:
//
typedef struct
{
  interpreter_state_t state;

  TimerHandle_t therapy_delay_timer_handle;
  StaticTimer_t therapy_delay_timer_struct;
  TimerHandle_t therapy_timer1_timer_handle;
  StaticTimer_t therapy_timer1_timer_struct;

  therapy_type_t therapy_type;

  bool wait_for_delay;
  bool wait_for_timer1;

  command_parser_t shell;

  char script_filename[MAX_PATH_LEN];

  FIL  script_file;

  char read_buffer[INTERPRETER_READ_BUFFER_CAPACITY];
  size_t read_buffer_index;
  size_t read_buffer_size;

  bool read_complete;

  // blink state
  blink_state_t blink_state;
  uint8_t blink_index;
  uint8_t blink_success_counter;
  uint8_t blink_status[NUM_BLINK_PROBES];

} interpreter_context_t;

static interpreter_context_t g_interpreter_context;
static TaskHandle_t g_interpreter_task_handle = NULL;

// Global event queue and handler:
static uint8_t g_event_queue_array[INTERPRETER_EVENT_QUEUE_SIZE*sizeof(interpreter_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event (interpreter_event_t *event);

// Global filename message buffer:
static uint8_t g_scriptname_mbuf_array[ SCRIPTNAME_MBUF_SIZE_BYTES ];
static StaticMessageBuffer_t g_scriptname_mbuf_struct;
static MessageBufferHandle_t g_scriptname_mbuf_handle;

// Function prototypes
static void interpreter_handle_rtc_alarm(void);

// For logging and debug:
static const char *
interpreter_state_name (interpreter_state_t state)
{
  switch (state)
    {
    case INTERPRETER_STATE_STANDBY:    return "INTERPRETER_STATE_STANDBY";
    case INTERPRETER_STATE_RUNNING:    return "INTERPRETER_STATE_RUNNING";
    case INTERPRETER_STATE_BLINK_TEST: return "INTERPRETER_STATE_BLINK_TEST";
    default:
      break;
    }
  return "INTERPRETER_STATE UNKNOWN";
}

static const char *
interpreter_event_type_name (interpreter_event_type_t event_type)
{
  switch (event_type)
    {
    case INTERPRETER_EVENT_ENTER_STATE:        return "INTERPRETER_EVENT_ENTER_STATE";
    case INTERPRETER_EVENT_INTRO_WAV_TIMEOUT:  return "INTERPRETER_EVENT_INTRO_WAV_TIMEOUT";
    case INTERPRETER_EVENT_START:              return "INTERPRETER_EVENT_START";
    case INTERPRETER_EVENT_NATURAL_STOP:       return "INTERPRETER_EVENT_NATURAL_STOP";
    case INTERPRETER_EVENT_FORCED_STOP:        return "INTERPRETER_EVENT_FORCED_STOP";
    case INTERPRETER_EVENT_STIMULATION_TIMEOUT:   return "INTERPRETER_EVENT_STIMULATION_TIMEOUT";
    case INTERPRETER_EVENT_DURATION_TIMEOUT:   return "INTERPRETER_EVENT_DURATION_TIMEOUT";
    case INTERPRETER_EVENT_AUDIO_STOP_TIMEOUT: return "INTERPRETER_EVENT_AUDIO_STOP_TIMEOUT";

    case INTERPRETER_EVENT_START_SCRIPT:       return "INTERPRETER_EVENT_START_SCRIPT";
    case INTERPRETER_EVENT_STOP_SCRIPT:        return "INTERPRETER_EVENT_STOP_SCRIPT";
    case INTERPRETER_EVENT_SCRIPT_COMMAND_COMPLETE: return "INTERPRETER_EVENT_SCRIPT_COMMAND_COMPLETE";
    case INTERPRETER_EVENT_DELAY_TIMEOUT:      return "INTERPRETER_EVENT_DELAY_TIMEOUT";
    case INTERPRETER_EVENT_TIMER1_TIMEOUT:     return "INTERPRETER_EVENT_TIMER1_TIMEOUT";
    case INTERPRETER_EVENT_START_DELAY_TIMER:  return "INTERPRETER_EVENT_START_DELAY_TIMER";
    case INTERPRETER_EVENT_START_TIMER1_TIMER: return "INTERPRETER_EVENT_START_TIMER1_TIMER";
    case INTERPRETER_EVENT_WAIT_TIMER1_TIMER:  return "INTERPRETER_EVENT_WAIT_TIMER1_TIMER";
    case INTERPRETER_EVENT_RTC_ALARM:          return "INTERPRETER_EVENT_RTC_ALARM";

    case INTERPRETER_EVENT_BLINK_START_TEST:   return "INTERPRETER_EVENT_BLINK_START_TEST";
    case INTERPRETER_EVENT_BLINK_STOP_TEST:    return "INTERPRETER_EVENT_BLINK_STOP_TEST";
    case INTERPRETER_EVENT_BLINK_PLAY_PROMPT:  return "INTERPRETER_EVENT_BLINK_PLAY_PROMPT";
    case INTERPRETER_EVENT_BLINK_PLAY_PIP:     return "INTERPRETER_EVENT_BLINK_PLAY_PIP";
    case INTERPRETER_EVENT_BLINK_PLAY_SUCCESS: return "INTERPRETER_EVENT_BLINK_PLAY_SUCCESS";
    case INTERPRETER_EVENT_BLINK_PLAY_FAIL:    return "INTERPRETER_EVENT_BLINK_PLAY_FAIL";
    case INTERPRETER_EVENT_BLINK_DETECTED:     return "INTERPRETER_EVENT_BLINK_DETECTED";

    default:
      break;
    }
  return "INTERPRETER_EVENT UNKNOWN";
}

static void close_script(){
  if(f_is_open(&g_interpreter_context.script_file)){
    f_close(&g_interpreter_context.script_file);
  }
}

static bool open_script(char* filename){
  // ensure the previous file is closed!
  close_script(); // TODO: Fix this, the entire open_script function should be called from the interpreter_task, and not as an event.

  // ensure the data log folder exists
  f_mkdir(SCRIPT_DIR_PATH);
  // create log file name
  char script_fname[MAX_PATH_LENGTH];
  size_t log_fsize = 0;
  log_fsize = str_append2(script_fname, log_fsize, SCRIPT_DIR_PATH); // directory
  log_fsize = str_append2(script_fname, log_fsize, "/");               // path separator
  log_fsize = str_append2(script_fname, log_fsize, filename);             // log file name

  FRESULT result = f_open(&g_interpreter_context.script_file, script_fname, FA_READ);
  if (result) {
    LOGE(TAG, "f_open() for %s returned %u\n", script_fname, result);
    return false;
  }

  // clear the read buffer
  memset(g_interpreter_context.read_buffer, 0, sizeof(g_interpreter_context.read_buffer));
  g_interpreter_context.read_buffer_index = 0;
  g_interpreter_context.read_buffer_size = 0;
  g_interpreter_context.read_complete = false;

  return true;
}


static int getchar_script(void){
  if(!f_is_open(&g_interpreter_context.script_file)){
    LOGE(TAG, "getchar_script has been called with a null file pointer (This should never happen).");
    return EOF;
  }
  if (g_interpreter_context.read_buffer_index >= g_interpreter_context.read_buffer_size) {
    UINT bytes_read;
    FRESULT result = f_read(
      &g_interpreter_context.script_file, 
      &g_interpreter_context.read_buffer, 
      INTERPRETER_READ_BUFFER_CAPACITY,
      &bytes_read );
    if(FR_OK != result) {
      LOGE(TAG, "f_read() returned %u\n", result);
      return EOF;
    }else if(bytes_read < 0){
      g_interpreter_context.read_buffer_index = 0;
      g_interpreter_context.read_buffer_size = 0;
      g_interpreter_context.read_complete = true;
    }else if(bytes_read < INTERPRETER_READ_BUFFER_CAPACITY){
      g_interpreter_context.read_buffer_size = bytes_read;
      g_interpreter_context.read_buffer_index = 0;
      g_interpreter_context.read_complete = true;
    }else{
      g_interpreter_context.read_buffer_size = bytes_read;
      g_interpreter_context.read_buffer_index = 0;
      g_interpreter_context.read_complete = false;
    }
  }
  if (g_interpreter_context.read_complete && g_interpreter_context.read_buffer_index==g_interpreter_context.read_buffer_size) {
    return EOF;
  } else {
    return g_interpreter_context.read_buffer[g_interpreter_context.read_buffer_index++];
  }
}

interpreter_state_t interpreter_get_state(void){
	return g_interpreter_context.state;
}

void
interpreter_event_start_script(const char* filename){
  // stop the blink test, if it's running
  interpreter_event_blink_stop_test();
  vTaskDelay(pdMS_TO_TICKS(200));

  // TODO: Will this function be called from more than one task?
  // todo: open file
  xMessageBufferSend(g_scriptname_mbuf_handle, filename, strlen(filename), portMAX_DELAY);
  interpreter_event_t event = {.type = INTERPRETER_EVENT_START_SCRIPT };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
interpreter_event_stop_script(bool start_electrode_quality){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_STOP_SCRIPT, .data = {.start_electrode_quality = start_electrode_quality}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
interpter_event_script_command_complete(){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_SCRIPT_COMMAND_COMPLETE };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
interpreter_event_start_therapy(therapy_type_t therapy_type)
{
  if(therapy_type <=0){
    return;
  }

  // convert count to new log ID
  char therapy_number[5];
  snprintf ( therapy_number, 5, "%d", therapy_type );

  char therapy_fname[20];
  size_t index = 0;
  index = str_append2(therapy_fname, index, "script"); // directory
  index = str_append2(therapy_fname, index, therapy_number); // directory
  index = str_append2(therapy_fname, index, ".txt"); // directory

  LOGV(TAG, "Starting therapy: %s", therapy_fname);

  // TODO: somehow start the script, given only a therapy number
  interpreter_event_start_script((char*)therapy_fname);
}

void
interpreter_event_forced_stop_therapy(void)
{
  // stop the script
  interpreter_event_stop_script(false);

  // TODO: clean up the state machine in the interpreter,
  // Should both the script and the blink test be conisdered "therapies" to stop?
  interpreter_event_blink_stop_test();
}

void
interpreter_event_delay(unsigned long time_ms){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_START_DELAY_TIMER, .data = {.timer = {.time_ms = time_ms}}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
interpreter_event_start_timer1(unsigned long time_ms){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_START_TIMER1_TIMER, .data = {.timer = {.time_ms = time_ms}}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
interpreter_event_wait_timer1(){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_WAIT_TIMER1_TIMER };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

#if 0
static void
interpreter_event_delay_timeout(void)
{
  interpreter_event_t event = {.type = INTERPRETER_EVENT_DELAY_TIMEOUT};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
interpreter_event_timer1_timeout(void)
{
  interpreter_event_t event = {.type = INTERPRETER_EVENT_TIMER1_TIMEOUT};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}
#endif

void
interpreter_event_blink_start_test(){
  // stop the therapy, if it's running
  interpreter_event_forced_stop_therapy();
  vTaskDelay(pdMS_TO_TICKS(200));

  interpreter_event_t event = {.type = INTERPRETER_EVENT_BLINK_START_TEST };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
interpreter_event_blink_stop_test(){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_BLINK_STOP_TEST };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
interpreter_event_blink_play_prompt(){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_BLINK_PLAY_PROMPT };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
interpreter_event_blink_play_pip(){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_BLINK_PLAY_PIP };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
interpreter_event_blink_play_success(){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_BLINK_PLAY_SUCCESS };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
interpreter_event_blink_play_fail(){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_BLINK_PLAY_FAIL };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
interpreter_event_blink_detected(){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_BLINK_DETECTED };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

static void
log_event (interpreter_event_t *event)
{
  switch (event->type){
    default:
      LOGV (TAG, "[%s] Event: %s",
	    interpreter_state_name (g_interpreter_context.state),
	    interpreter_event_type_name (event->type));
      break;
  }
}

static void
log_event_ignored (interpreter_event_t *event)
{
  switch(event->type){
  default:
    LOGD (TAG, "[%s] Ignored Event: %s",
      interpreter_state_name (g_interpreter_context.state),
      interpreter_event_type_name (event->type));
    break;
  }
}

static void
set_state (interpreter_state_t state)
{
  LOGD (TAG, "[%s] -> [%s]",
	interpreter_state_name (g_interpreter_context.state),
	interpreter_state_name (state));

  g_interpreter_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  interpreter_event_t event =
    { .type = INTERPRETER_EVENT_ENTER_STATE };
  handle_event (&event);
}

static void
restart_therapy_delay_timer(uint32_t timeout_ms)
{
  // xTimerChangePeriod will start timer if it's not running already
  if (xTimerChangePeriod(g_interpreter_context.therapy_delay_timer_handle,
      pdMS_TO_TICKS_SAFE(timeout_ms), 0) == pdFAIL) {
    LOGE(TAG, "Unable to start delay timer!");
  }
}

static void
stop_therapy_delay_timer(void)
{
  if (xTimerStop(g_interpreter_context.therapy_delay_timer_handle, 0) == pdFAIL) {
    LOGE(TAG, "Unable to stop delay timer!");
  }
}

static void
restart_therapy_timer1_timer(uint32_t timeout_ms)
{
  // xTimerChangePeriod will start timer if it's not running already
  if (xTimerChangePeriod(g_interpreter_context.therapy_timer1_timer_handle,
      pdMS_TO_TICKS_SAFE(timeout_ms), 0) == pdFAIL) {
    LOGE(TAG, "Unable to start timer1 timer!");
  }
}

static void
stop_therapy_timer1_timer(void)
{
  if (xTimerStop(g_interpreter_context.therapy_timer1_timer_handle, 0) == pdFAIL) {
    LOGE(TAG, "Unable to stop timer1 timer!");
  }
}

static void
interpreter_stop_blink_test(){
  stop_therapy_delay_timer();
  eeg_processor_stop_blink_test();
  eeg_processor_stop_quality_check();
  eeg_reader_event_stop();
  audio_bg_script_volume(0);
  audio_bg_fadeout(0);
  audio_bgwav_stop();
  ble_quality_check_update(0);
}

static void
interpreter_stop_therapy(){
  // close the script file
  close_script();
  // stop logging
  data_log_close();
  user_metrics_log_close_command();
  // stop any timers
  stop_therapy_delay_timer();
  stop_therapy_timer1_timer();
  g_interpreter_context.wait_for_delay = false;
  g_interpreter_context.wait_for_timer1 = false;
  // stop audio
  audio_bg_fadeout(0);
  audio_pink_fadeout(0);
  // give time for the audio to fade out.
//      vTaskDelay(20);
  audio_bgwav_stop();
  audio_pink_stop();
  audio_sine_stop();
  // stop echt
  eeg_processor_stop_echt();
  // stop eeg
  eeg_reader_event_stop();
  // stop erp
  erp_event_stop();
  // stop any sensors
  accel_turn_off();
  system_monitor_event_als_stop();
  system_monitor_event_mic_stop();
  hrm_event_turn_off();
  // stop any ML
  ml_event_stop();

  // update ble
  ble_therapy_update( THERAPY_TYPE_NONE );
}



//
// Event handlers for the various application states:
//

static void
handle_state_standby (interpreter_event_t *event)
{

  switch (event->type)
    {
    case INTERPRETER_EVENT_ENTER_STATE:
      // STOP Everything
      break;

    case INTERPRETER_EVENT_START_SCRIPT:
    {
      //TODO: STOP the blink test

      //
      size_t msg_len = xMessageBufferReceive( g_scriptname_mbuf_handle, g_interpreter_context.script_filename, MAX_PATH_LEN-1, 0);
      if(msg_len == 0){
        LOGE(TAG, "No script filename corresponding to event_start_script.");
        break; // exit switch statement
      }else{
        g_interpreter_context.script_filename[msg_len] = '\0';
      }
      bool script_open = open_script(g_interpreter_context.script_filename);
      // TODO: start script
      if(script_open){
        set_state (INTERPRETER_STATE_RUNNING);
      }
      break;
    }

    case INTERPRETER_EVENT_BLINK_START_TEST:
      set_state(INTERPRETER_STATE_BLINK_TEST);
      break;

    case INTERPRETER_EVENT_RTC_ALARM:
      interpreter_handle_rtc_alarm();
      break;

    default:
      log_event_ignored (event);
      break;
    }
}

static void
handle_state_running (interpreter_event_t *event)
{
  switch (event->type)
  {
    case INTERPRETER_EVENT_ENTER_STATE:
      // reset interpreter shell
      memset(&g_interpreter_context.shell, 0, sizeof(g_interpreter_context.shell));
#if 0
      g_interpreter_context.shell.prompt_char = '\0';
      g_interpreter_context.shell.use_prompt = false;
#else
      g_interpreter_context.shell.prompt_char = '#';
      g_interpreter_context.shell.use_prompt = true;
#endif

      // reset context
      g_interpreter_context.wait_for_delay = false;
      g_interpreter_context.wait_for_timer1 = false;
      // script complete
      interpter_event_script_command_complete();
      break;

    case INTERPRETER_EVENT_SCRIPT_COMMAND_COMPLETE:
      if(!g_interpreter_context.wait_for_delay && 
         !g_interpreter_context.wait_for_timer1 && 
         f_is_open(&g_interpreter_context.script_file)){
        while (1) {
          int c = getchar_script();
          if (c == EOF) {
            interpreter_event_stop_script(false);
            break;
          } else {
#if 1
            handle_input( &g_interpreter_context.shell ,(char)c );
            if(g_interpreter_context.shell.status == PARSER_COMMAND_FOUND){
              if (!strncmp(g_interpreter_context.shell.bufsaved, "therapy_delay", strlen("therapy_delay"))){
                // do nothing - the timer timeout will issue the command_complete event
              }else if(!strncmp(g_interpreter_context.shell.bufsaved, "therapy_wait_timer1", strlen("therapy_wait_timer1"))){
                // do nothing - the timer timeout will issue the command_complete event
              }else{
                interpter_event_script_command_complete();
              }
              break;
            }
#else
            if ( g_interpreter_context.shell.status == PARSER_COMMAND_FOUND ){
              interpter_event_script_command_complete();
              break; // exit while loop
            }
            fflush(stdout);
#endif
          }
        }
      }
      break; // exit switch (event->type)

    case INTERPRETER_EVENT_START_SCRIPT:
    {
      // throw away any filenames received while running the previous script
      char dummy[MAX_PATH_LEN];
      xMessageBufferReceive( g_scriptname_mbuf_handle, dummy, MAX_PATH_LEN-1, 0);
      break;
    }

    case INTERPRETER_EVENT_DELAY_TIMEOUT:
      g_interpreter_context.wait_for_delay = false;
      // continue script execution
      interpter_event_script_command_complete();
      break;

    case INTERPRETER_EVENT_TIMER1_TIMEOUT:
      g_interpreter_context.wait_for_timer1 = false;
      // continue script execution
      interpter_event_script_command_complete();
      break;

    case INTERPRETER_EVENT_START_DELAY_TIMER:
      g_interpreter_context.wait_for_delay = true;
      restart_therapy_delay_timer(event->data.timer.time_ms);
      break;

    case INTERPRETER_EVENT_START_TIMER1_TIMER:
      g_interpreter_context.wait_for_timer1 = false;
      restart_therapy_timer1_timer(event->data.timer.time_ms);
      break;

    case INTERPRETER_EVENT_WAIT_TIMER1_TIMER:
      g_interpreter_context.wait_for_timer1 = true;
      // do nothing - pausing script execution until timer1 timeout event
      break;

    case INTERPRETER_EVENT_STOP_SCRIPT:
      interpreter_stop_therapy();
      // change the state to standy
      set_state(INTERPRETER_STATE_STANDBY);
      break;

    case INTERPRETER_EVENT_RTC_ALARM:
      interpreter_handle_rtc_alarm();
      break;

    default:
      log_event_ignored (event);
      break;
    }
}

static void
handle_state_blink_test (interpreter_event_t *event)
{
  switch (event->type)
  {
    case INTERPRETER_EVENT_ENTER_STATE:
    {
      eeg_reader_event_start();
      audio_set_volume(128);
      audio_bg_fadein(0);
      audio_bg_default_volume();
      audio_bg_script_volume(1);
      memset(g_interpreter_context.blink_status,BLINK_NONE,sizeof(g_interpreter_context.blink_status));
      // reset BLE display
      ble_blink_status_update((uint8_t*) g_interpreter_context.blink_status);
      uint8_t electrode_quality[ELECTRODE_NUM] = { 0 };
      ble_electrode_quality_update(electrode_quality);

      // reset blink counters
      g_interpreter_context.blink_index = 0;
      g_interpreter_context.blink_success_counter = 0;
      // set states
      g_interpreter_context.blink_state = BLINK_STATE_STANDBY;
      interpreter_event_blink_play_prompt();
      break;
    }
    case INTERPRETER_EVENT_BLINK_PLAY_PROMPT:
      audio_bgwav_play( (char*) prompt_wav_filepath , false);
      g_interpreter_context.blink_state = BLINK_STATE_WAIT_PROMPT;
      restart_therapy_delay_timer(prompt_wav_dur_ms);
      break;

    case INTERPRETER_EVENT_BLINK_PLAY_PIP:
      eeg_processor_start_blink_test();
      audio_bgwav_play( (char*) pip_wav_filepath , false);
      g_interpreter_context.blink_state = BLINK_STATE_WAIT_PIP;
      restart_therapy_delay_timer(pip_dur_ms);
      break;

    case INTERPRETER_EVENT_BLINK_PLAY_SUCCESS:
      eeg_processor_start_quality_check();
      audio_bgwav_play( (char*) blink_success_wav_filepath , false);
      g_interpreter_context.blink_state = BLINK_STATE_WAIT_SUCCESS;
      restart_therapy_delay_timer(blink_success_wav_dur_ms);
      break;

    case INTERPRETER_EVENT_BLINK_PLAY_FAIL:
      audio_bgwav_play( (char*) blink_failed_wav_filepath , false);
      g_interpreter_context.blink_state = BLINK_STATE_WAIT_FAIL;
      restart_therapy_delay_timer(blink_failed_wav_dur_ms);
      break;

    case INTERPRETER_EVENT_BLINK_DETECTED:
      // record blink detected
      if(g_interpreter_context.blink_status[ g_interpreter_context.blink_index] == BLINK_NONE){
        g_interpreter_context.blink_status[ g_interpreter_context.blink_index] = BLINK_GOOD;
        ble_blink_status_update((uint8_t*) g_interpreter_context.blink_status);
        g_interpreter_context.blink_success_counter++;
      }
      break;

    case INTERPRETER_EVENT_DELAY_TIMEOUT:
      switch(g_interpreter_context.blink_state){
        case BLINK_STATE_STANDBY:
          break;
        case BLINK_STATE_WAIT_PROMPT:
          interpreter_event_blink_play_pip();
          break;
        case BLINK_STATE_WAIT_PIP:
          eeg_processor_stop_blink_test();
          // record blink missed
          if(g_interpreter_context.blink_status[ g_interpreter_context.blink_index] == BLINK_NONE){
            g_interpreter_context.blink_status[ g_interpreter_context.blink_index] = BLINK_MISSED;
          }
          // send notification over bluetooth
          ble_blink_status_update((uint8_t*) g_interpreter_context.blink_status);

          g_interpreter_context.blink_index++;
          if(g_interpreter_context.blink_index >= NUM_BLINK_PROBES){
            if( g_interpreter_context.blink_success_counter >= NUM_BLINK_FOR_SUCCESS ){
              interpreter_event_blink_play_success();
            }else{
              interpreter_event_blink_play_fail();
            }
          }else{
            interpreter_event_blink_play_pip();
          }
          break;
        case BLINK_STATE_WAIT_SUCCESS:
//          set_state(INTERPRETER_STATE_STANDBY);
          break;
        case BLINK_STATE_WAIT_FAIL:
          interpreter_event_blink_stop_test();
          eeg_reader_event_stop();
          break;
      }
      break;

    case INTERPRETER_EVENT_BLINK_STOP_TEST:
      interpreter_stop_blink_test();
      set_state(INTERPRETER_STATE_STANDBY);
      break;

    default:
      log_event_ignored (event);
      break;
    }
}

static void
handle_event (interpreter_event_t *event)
{
  switch (g_interpreter_context.state)
    {
    case INTERPRETER_STATE_STANDBY:
      handle_state_standby (event);
      break;

    case INTERPRETER_STATE_RUNNING:
      handle_state_running (event);
      break;

    case INTERPRETER_STATE_BLINK_TEST:
      handle_state_blink_test (event);
      break;

    default:
      // (We should never get here.)
      LOGE (TAG, "Unknown interpreter_state: %d", (int) g_interpreter_context.state);
      break;
    }
}

static void
interpreter_handle_rtc_alarm(void)
{
  // static const char filename[] = "script_alarm.txt";
  // interpreter_event_start_script(filename);
  app_event_rtc_activity();
  rtc_alarm_init();
}

void
rtc_alarm_isr_cb(void) 
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  static const interpreter_event_t event = {.type = INTERPRETER_EVENT_RTC_ALARM };
  xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);
  // Always do this when calling a FreeRTOS "...FromISR()" function:
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

}

static void
therapy_delay_timer_timeout(TimerHandle_t timer_handle){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_DELAY_TIMEOUT};
  xQueueSend(g_event_queue, &event, 0);
}

static void
therapy_timer1_timer_timeout(TimerHandle_t timer_handle){
  interpreter_event_t event = {.type = INTERPRETER_EVENT_TIMER1_TIMEOUT};
  xQueueSend(g_event_queue, &event, 0);
}

void
interpreter_pretask_init (void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(INTERPRETER_EVENT_QUEUE_SIZE,sizeof(interpreter_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "interpreter_event_queue");

  // Create the message buffer
  memset(g_scriptname_mbuf_array, 0, sizeof(g_scriptname_mbuf_array));
  g_scriptname_mbuf_handle = xMessageBufferCreateStatic(sizeof(g_scriptname_mbuf_array), g_scriptname_mbuf_array, &(g_scriptname_mbuf_struct));
}


static void
task_init ()
{
  // Any post-scheduler init goes here.

  g_interpreter_task_handle = xTaskGetCurrentTaskHandle();

  set_state (INTERPRETER_STATE_STANDBY);

  // start with a dummy time
  g_interpreter_context.therapy_delay_timer_handle = xTimerCreateStatic("THERAPY_DELAY_TIMER_HANDLE",
      pdMS_TO_TICKS_SAFE(100), pdFALSE, NULL,
    therapy_delay_timer_timeout, &(g_interpreter_context.therapy_delay_timer_struct));

  // start with a dummy time
  g_interpreter_context.therapy_timer1_timer_handle = xTimerCreateStatic("THERAPY_TIMER1_TIMER_HANDLE",
      pdMS_TO_TICKS_SAFE(100), pdFALSE, NULL,
    therapy_timer1_timer_timeout, &(g_interpreter_context.therapy_timer1_timer_struct));

  // ensure the scripts directory
  f_mkdir(SCRIPT_DIR_PATH);

  LOGV (TAG, "Task launched. Entering event loop.");
}

void
interpreter_task (void *ignored)
{
  task_init ();

  interpreter_event_t event;

  while (1) {
    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);
  }

}

#else // (defined(ENABLE_INTERPRETER_TASK) && (ENABLE_INTERPRETER_TASK > 0U))

void interpreter_pretask_init(void){}
void interpreter_task(void *ignored){}

void interpreter_event_start_script(const char* filepath){}
void interpreter_event_stop_script(bool start_electrode_quality){}

void interpreter_event_delay(unsigned long time_ms){}
void interpreter_event_start_timer1(unsigned long time_ms){}
void interpreter_event_wait_timer1(){}

// Send various event types to this task:
void interpreter_event_start_therapy(therapy_type_t therapy_type){}
void interpreter_event_natural_stop_therapy(void){}
void interpreter_event_forced_stop_therapy(void){}

void interpreter_event_blink_start_test(){}
void interpreter_event_blink_stop_test(){}
void interpreter_event_blink_detected(){}

#endif // (defined(ENABLE_INTERPRETER_TASK) && (ENABLE_INTERPRETER_TASK > 0U))
