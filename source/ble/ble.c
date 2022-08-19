
#include <stdlib.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "loglevels.h"
#include "config.h"

#include "app.h"
#include "audio.h"
#include "ble.h"
#include "ble_uart_commands.h"
#include "ble_uart_recv.h"
#include "ble_uart_send.h"
#include "eeg_reader.h"
#include "rtc.h"

#include "interpreter.h"
#include "settings.h"

#define BLE_EVENT_QUEUE_SIZE 10

static const char *TAG = "ble";	// Logging prefix for this module

#define   BLE_POWER_OFF_DELAY_MS 50


//
// Task events:
//
typedef enum
{
  BLE_EVENT_ENTER_STATE,	// (used for state transitions)
  BLE_EVENT_BATTERY_LEVEL_REQUEST,
  BLE_EVENT_BATTERY_LEVEL_UPDATE,
  BLE_EVENT_SERIAL_NUMBER_REQUEST,
  BLE_EVENT_SOFTWARE_VERSION_REQUEST,
  BLE_EVENT_DFU_REQUEST,
  BLE_EVENT_ELECTRODE_QUALITY_REQUEST,
  BLE_EVENT_ELECTRODE_QUALITY_UPDATE,
  BLE_EVENT_VOLUME_REQUEST,
  BLE_EVENT_VOLUME_COMMAND,
  BLE_EVENT_VOLUME_UPDATE,
  BLE_EVENT_POWER_REQUEST,
  BLE_EVENT_POWER_COMMAND,
  BLE_EVENT_POWER_UPDATE,
  BLE_EVENT_THERAPY_REQUEST,
  BLE_EVENT_THERAPY_COMMAND,
  BLE_EVENT_THERAPY_UPDATE,
  BLE_EVENT_HEART_RATE_REQUEST,
  BLE_EVENT_HEART_RATE_UPDATE,
  BLE_EVENT_RESET,
  BLE_EVENT_BLINK_STATUS_REQUEST,
  BLE_EVENT_BLINK_STATUS_UPDATE,
  BLE_EVENT_QUALITY_CHECK_REQUEST,
  BLE_EVENT_QUALITY_CHECK_COMMAND,
  BLE_EVENT_QUALITY_CHECK_UPDATE,
  BLE_EVENT_ALARM_REQUEST,
  BLE_EVENT_ALARM_COMMAND,
  BLE_EVENT_ALARM_UPDATE,
  BLE_EVENT_SOUND_REQUEST,
  BLE_EVENT_SOUND_COMMAND,
  BLE_EVENT_SOUND_UPDATE,
  BLE_EVENT_TIME_REQUEST,
  BLE_EVENT_TIME_COMMAND,
  BLE_EVENT_TIME_UPDATE,
  BLE_EVENT_ADDR_COMMAND,
} ble_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  ble_event_type_t type;
//  uint8_t user_data[ELECTRODE_NUM]; // Max data is 1 byte per electrode
  uint8_t user_data[10]; // Max data is 1 byte per blink status
} ble_event_t;


//
// State machine states:
//
typedef enum
{
  BLE_STATE_NONE,
  BLE_STATE_STANDBY,
} ble_state_t;

//
// Global context data:
//
typedef struct
{
  ble_state_t state;
  uint8_t electrode_quality[ELECTRODE_NUM];
  uint8_t blink_status[BLINK_NUM];
  uint8_t volume;
  uint8_t power;
  uint8_t therapy;
  uint8_t heart_rate;
  uint8_t battery_level;
  char serial_number[SERIAL_NUMBER_NUM];
  char* software_version;
  uint8_t quality_check;
  alarm_params_t alarm;
  uint8_t sound;
  uint64_t time;
  uint8_t addr[ADDR_NUM];
} ble_context_t;

///@todo BH Get real values at task startup?
static ble_context_t g_ble_context = {
  .state = BLE_STATE_NONE,
  .electrode_quality = {0, 0, 0, 0, 0, 0, 0, 0},
  .blink_status = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  .volume = 0,
  .power = 0,
  .therapy = 0,
  .heart_rate = 0xFF,
  .battery_level = 100,
  .serial_number = {0},
  .software_version = FW_VERSION_GIT_STRING,
  .quality_check = 0,
  .alarm = {.all = {0},},
  .sound = 0,
  .time = 0,
  .addr = {0,0,0,0,0,0},
};

static void init_g_ble_context(){
  strncpy(g_ble_context.serial_number,"<unspecified>",SERIAL_NUMBER_NUM);
  g_ble_context.serial_number[SERIAL_NUMBER_NUM-1] = '\0';
}

// Global event queue and handler:
static uint8_t g_event_queue_array[BLE_EVENT_QUEUE_SIZE*sizeof(ble_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(ble_event_t *event);

// For logging and debug:
static const char *
ble_state_name(ble_state_t state)
{
  switch (state) {
    case BLE_STATE_STANDBY: return "BLE_STATE_STANDBY";
    default:
      break;
  }
  return "BLE_STATE UNKNOWN";
}

static const char *
ble_event_type_name(ble_event_type_t event_type)
{
  switch (event_type) {
    case BLE_EVENT_ENTER_STATE:
      return "BLE_EVENT_ENTER_STATE";
    case BLE_EVENT_BATTERY_LEVEL_REQUEST:
      return "BLE_EVENT_BATTERY_LEVEL_REQUEST";
    case BLE_EVENT_BATTERY_LEVEL_UPDATE:
      return "BLE_EVENT_BATTERY_LEVEL_UPDATE";
    case BLE_EVENT_SERIAL_NUMBER_REQUEST:
      return "BLE_EVENT_SERIAL_NUMBER_REQUEST";
    case BLE_EVENT_SOFTWARE_VERSION_REQUEST:
      return "BLE_EVENT_SOFTWARE_VERSION_REQUEST";
    case BLE_EVENT_DFU_REQUEST:
      return "BLE_EVENT_DFU_REQUEST";
    case BLE_EVENT_ELECTRODE_QUALITY_REQUEST:
      return "BLE_EVENT_ELECTRODE_QUALITY_REQUEST";
    case BLE_EVENT_ELECTRODE_QUALITY_UPDATE:
      return "BLE_EVENT_ELECTRODE_QUALITY_UPDATE";
    case BLE_EVENT_VOLUME_REQUEST:
      return "BLE_EVENT_VOLUME_REQUEST";
    case BLE_EVENT_VOLUME_COMMAND:
      return "BLE_EVENT_VOLUME_COMMAND";
    case BLE_EVENT_VOLUME_UPDATE:
      return "BLE_EVENT_VOLUME_UPDATE";
    case BLE_EVENT_POWER_REQUEST:
      return "BLE_EVENT_POWER_REQUEST";
    case BLE_EVENT_POWER_COMMAND:
      return "BLE_EVENT_POWER_COMMAND";
    case BLE_EVENT_POWER_UPDATE:
      return "BLE_EVENT_POWER_UPDATE";
    case BLE_EVENT_THERAPY_REQUEST:
      return "BLE_EVENT_THERAPY_REQUEST";
    case BLE_EVENT_THERAPY_COMMAND:
      return "BLE_EVENT_THERAPY_COMMAND";
    case BLE_EVENT_THERAPY_UPDATE:
      return "BLE_EVENT_THERAPY_UPDATE";
    case BLE_EVENT_HEART_RATE_REQUEST:
      return "BLE_EVENT_HEART_RATE_REQUEST";
    case BLE_EVENT_HEART_RATE_UPDATE:
      return "BLE_EVENT_HEART_RATE_UPDATE";
    case BLE_EVENT_RESET:
      return "BLE_EVENT_RESET";
    case BLE_EVENT_BLINK_STATUS_REQUEST:
      return "BLE_EVENT_BLINK_STATUS_REQUEST";
    case BLE_EVENT_BLINK_STATUS_UPDATE:
      return "BLE_EVENT_BLINK_STATUS_UPDATE";
    case BLE_EVENT_QUALITY_CHECK_REQUEST:
      return "BLE_EVENT_QUALITY_CHECK_REQUEST";
    case BLE_EVENT_QUALITY_CHECK_COMMAND:
      return "BLE_EVENT_QUALITY_CHECK_COMMAND";
    case BLE_EVENT_QUALITY_CHECK_UPDATE:
      return "BLE_EVENT_QUALITY_CHECK_UPDATE";
    case BLE_EVENT_ALARM_REQUEST:
      return "BLE_EVENT_ALARM_REQUEST";
    case BLE_EVENT_ALARM_COMMAND:
      return "BLE_EVENT_ALARM_COMMAND";
    case BLE_EVENT_ALARM_UPDATE:
      return "BLE_EVENT_ALARM_UPDATE";
    case BLE_EVENT_SOUND_REQUEST:
      return "BLE_EVENT_SOUND_REQUEST";
    case BLE_EVENT_SOUND_COMMAND:
      return "BLE_EVENT_SOUND_COMMAND";
    case BLE_EVENT_SOUND_UPDATE:
      return "BLE_EVENT_SOUND_UPDATE";
    case BLE_EVENT_TIME_REQUEST:
      return "BLE_EVENT_TIME_REQUEST";
    case BLE_EVENT_TIME_COMMAND:
      return "BLE_EVENT_TIME_COMMAND";
    case BLE_EVENT_TIME_UPDATE:
      return "BLE_EVENT_TIME_UPDATE";
    case BLE_EVENT_ADDR_COMMAND:
      return "BLE_EVENT_ADDR_COMMAND";
    default:
      break;
  }
  return "BLE_EVENT UNKNOWN";
}

void
ble_battery_level_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_BATTERY_LEVEL_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_battery_level_update(uint8_t battery_level)
{
  ble_event_t event = { .type = BLE_EVENT_BATTERY_LEVEL_UPDATE,
    .user_data = {battery_level} };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_serial_number_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_SERIAL_NUMBER_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_software_version_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_SOFTWARE_VERSION_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_dfu_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_DFU_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_electrode_quality_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_ELECTRODE_QUALITY_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_electrode_quality_update(uint8_t qualities[ELECTRODE_NUM])
{
  ble_event_t event = { .type = BLE_EVENT_ELECTRODE_QUALITY_UPDATE };
  memcpy(event.user_data, qualities, sizeof(event.user_data));
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_blink_status_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_BLINK_STATUS_REQUEST };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
ble_blink_status_update(uint8_t status[BLINK_NUM])
{
  ble_event_t event = { .type = BLE_EVENT_BLINK_STATUS_UPDATE };
  memcpy(event.user_data, status, sizeof(event.user_data));
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}


void
ble_volume_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_VOLUME_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_volume_command(uint8_t volume)
{
  ble_event_t event = { .type = BLE_EVENT_VOLUME_COMMAND,
    .user_data = {volume} };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_volume_update(uint8_t volume)
{
  ble_event_t event = { .type = BLE_EVENT_VOLUME_UPDATE,
    .user_data = {volume} };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_power_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_POWER_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_power_command(uint8_t power)
{
  ble_event_t event = { .type = BLE_EVENT_POWER_COMMAND,
    .user_data = {power} };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_power_update(uint8_t power)
{
  ble_event_t event = { .type = BLE_EVENT_POWER_UPDATE,
    .user_data = {power} };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_therapy_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_THERAPY_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_therapy_command(uint8_t therapy)
{
  LOGV("ble.c","THERAPY: %d",therapy);

  ble_event_t event = { .type = BLE_EVENT_THERAPY_COMMAND,
    .user_data = {therapy} };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_therapy_update(uint8_t therapy)
{
  ble_event_t event = { .type = BLE_EVENT_THERAPY_UPDATE,
    .user_data = {therapy} };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_heart_rate_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_HEART_RATE_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_heart_rate_update(uint8_t heart_rate)
{
  ble_event_t event = { .type = BLE_EVENT_HEART_RATE_UPDATE,
    .user_data = {heart_rate} };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_quality_check_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_QUALITY_CHECK_REQUEST };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
ble_quality_check_command(uint8_t quality_check)
{
  ble_event_t event = { .type = BLE_EVENT_QUALITY_CHECK_COMMAND,
    .user_data = {quality_check} };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
ble_quality_check_update(uint8_t quality_check)
{
  ble_event_t event = { .type = BLE_EVENT_QUALITY_CHECK_UPDATE,
    .user_data = {quality_check} };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_alarm_request(void){
  ble_event_t event = { .type = BLE_EVENT_ALARM_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void ble_alarm_command(alarm_params_t *params){
  ble_event_t event = { .type = BLE_EVENT_ALARM_COMMAND,
    .user_data = {0} };
  memcpy(event.user_data, &(params->all[0]), ALARM_NUM);
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_alarm_update(alarm_params_t *params){
  ble_event_t event = { .type = BLE_EVENT_ALARM_UPDATE,
    .user_data = {0} };
//  LOGV("ble.c 1", "%lu",  (unsigned long) params->minutes_after_midnight );
  memcpy(event.user_data, &(params->all[0]), ALARM_NUM);
//  LOGV("ble.c 1a","%d %d %d",event.user_data[0],event.user_data[1],event.user_data[2]);
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_sound_request(void)
{
  ble_event_t event = { .type = BLE_EVENT_SOUND_REQUEST };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
ble_sound_command(uint8_t sound)
{
  ble_event_t event = { .type = BLE_EVENT_SOUND_COMMAND,
    .user_data = {sound} };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
ble_sound_update(uint8_t sound)
{
  ble_event_t event = { .type = BLE_EVENT_SOUND_UPDATE,
    .user_data = {sound} };
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_time_request(void){
  ble_event_t event = { .type = BLE_EVENT_TIME_REQUEST };
  xQueueSend(g_event_queue, &event, 0);
}

void ble_time_command(uint64_t unix_epoch_time_sec){
  ble_event_t event = { .type = BLE_EVENT_TIME_COMMAND,
    .user_data = {0} };
  memcpy(event.user_data, &unix_epoch_time_sec, TIME_NUM);
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_time_update(uint64_t unix_epoch_time_sec){
  ble_event_t event = { .type = BLE_EVENT_TIME_UPDATE,
    .user_data = {0} };
  memcpy(event.user_data, &unix_epoch_time_sec, TIME_NUM);
  xQueueSend(g_event_queue, &event, 0);
}

void ble_addr_command(uint8_t* addr){
  ble_event_t event = { .type = BLE_EVENT_ADDR_COMMAND,
    .user_data = {0} };
  memcpy(event.user_data, addr, ADDR_NUM);
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

uint8_t* ble_get_addr(){
  return g_ble_context.addr;
}

void
ble_reset(void)
{
  ble_event_t event = { .type = BLE_EVENT_RESET };
  xQueueSend(g_event_queue, &event, 0);
}

void
ble_power_off(void)
{
  if (g_ble_context.power != 0) {
    ble_power_update(0);

    // Wait for BLE task to transmit message over UART
    vTaskDelay(pdMS_TO_TICKS(BLE_POWER_OFF_DELAY_MS));
  }
};

void
ble_power_on(void)
{
  if (g_ble_context.power == 0) {
    g_ble_context.power = 1;
    ble_reset();
  }
};


static void
log_event(ble_event_t *event)
{
  switch (event->type) {
    case BLE_EVENT_ELECTRODE_QUALITY_UPDATE:
    // suppress electrode quality update event
      break;
    default:
      LOGV(TAG, "[%s] Event: %s", ble_state_name(g_ble_context.state),
        ble_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(ble_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s", ble_state_name(g_ble_context.state), ble_event_type_name(event->type));
}

static void
set_state(ble_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]", ble_state_name(g_ble_context.state), ble_state_name(state));

  g_ble_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  ble_event_t event = { BLE_EVENT_ENTER_STATE };
  handle_event(&event);
}



//
// Event handlers for the various application states:
//

static void
handle_battery_level_request(ble_event_t *event)
{
  ble_uart_send_battery_level(g_ble_context.battery_level);
}

static void
handle_battery_level_update(ble_event_t *event)
{
  g_ble_context.battery_level = event->user_data[0];
  ble_uart_send_battery_level(g_ble_context.battery_level);
}

static void
handle_serial_number_request(ble_event_t *event)
{
  // compute serial number based on BLE addr
    memset(g_ble_context.serial_number,0,sizeof(g_ble_context.serial_number));
    snprintf(g_ble_context.serial_number, sizeof(g_ble_context.serial_number),
      "%02X%02X%02X%02X%02X%02X\n",
      g_ble_context.addr[5], g_ble_context.addr[4], g_ble_context.addr[3],
      g_ble_context.addr[2], g_ble_context.addr[1], g_ble_context.addr[0]);
  ble_uart_send_serial_number(g_ble_context.serial_number);
}

static void
handle_software_version_request(ble_event_t *event)
{
  ble_uart_send_software_version(g_ble_context.software_version);
}

static void
handle_ble_dfu_request(ble_event_t *event)
{
  ble_uart_send_dfu();
}

static void
handle_electrode_quality_request(ble_event_t *event)
{
  ble_uart_send_electrode_quality(g_ble_context.electrode_quality);
}

static void
handle_electrode_quality_update(ble_event_t *event)
{
  memcpy(g_ble_context.electrode_quality, event->user_data,
    sizeof(g_ble_context.electrode_quality));
  ble_uart_send_electrode_quality(g_ble_context.electrode_quality);
}

static void
handle_blink_status_request(ble_event_t *event)
{
  memcpy(g_ble_context.blink_status, event->user_data,
    sizeof(g_ble_context.blink_status));
  ble_uart_send_blink_status(g_ble_context.blink_status);
}

static void
handle_blink_status_update(ble_event_t *event)
{
  memcpy(g_ble_context.blink_status, event->user_data,
    sizeof(g_ble_context.blink_status));
  ble_uart_send_blink_status(g_ble_context.blink_status);
}

static void
handle_volume_request(ble_event_t *event)
{
  ble_uart_send_volume(g_ble_context.volume);
}

static void
handle_volume_command(ble_event_t *event)
{
  audio_set_volume_ble(event->user_data[0]);
}

static void
handle_volume_update(ble_event_t *event)
{
  g_ble_context.volume = event->user_data[0];
  ble_uart_send_volume(g_ble_context.volume);
}

static void
handle_power_request(ble_event_t *event)
{
  ble_uart_send_power(g_ble_context.power);
}

static void
handle_power_command(ble_event_t *event)
{
  g_ble_context.power = event->user_data[0];
  ///@todo BH Route to power task (sysmon task?)
//  uint8_t power = event->user_data[0];

}

static void
handle_power_update(ble_event_t *event)
{
  g_ble_context.power = event->user_data[0];
  ble_uart_send_power(g_ble_context.power);
}

static void
handle_therapy_request(ble_event_t *event)
{
  ble_uart_send_therapy(g_ble_context.therapy);
}

static void
handle_therapy_command(ble_event_t *event)
{
  // Directly kick off interpreter with the given script
  uint8_t therapy = event->user_data[0];
  g_ble_context.therapy = therapy;
  if(therapy==0){
    interpreter_event_forced_stop_therapy();
  }else{
    interpreter_event_start_therapy(therapy);
  }
}

static void
handle_therapy_update(ble_event_t *event)
{
  g_ble_context.therapy = event->user_data[0];
  ble_uart_send_therapy(g_ble_context.therapy);
}

static void
handle_heart_rate_request(ble_event_t *event)
{
  ble_uart_send_heart_rate(g_ble_context.heart_rate);
}

static void
handle_heart_rate_update(ble_event_t *event)
{
  g_ble_context.heart_rate = event->user_data[0];
  ble_uart_send_heart_rate(g_ble_context.heart_rate);
}

void
handle_ble_reset(ble_event_t *event)
{
//  debug_uart_puts("handle_ble_reset 0");
  // Reset BLE chip
  GPIO_PinWrite(GPIO, BLE_RESETN_PORT, BLE_RESETN_PIN, 0);
  vTaskDelay(10); // Hold reset for the rest of this tick + one more
  GPIO_PinWrite(GPIO, BLE_RESETN_PORT, BLE_RESETN_PIN, 1);
//  debug_uart_puts("handle_ble_reset 1");
}

static void
handle_quality_check_request(ble_event_t *event)
{
  ble_uart_send_quality_check(g_ble_context.quality_check);
}

static void
handle_quality_check_command(ble_event_t *event)
{
  ///@todo BH Route to power task (sysmon task?)
  g_ble_context.quality_check = event->user_data[0];
  uint8_t quality_check = g_ble_context.quality_check;
  if(quality_check==1){
    // on
    interpreter_event_blink_start_test();
  }else{
    // off
    interpreter_event_blink_stop_test();
  }
}

static void
handle_quality_check_update(ble_event_t *event)
{
  g_ble_context.quality_check = event->user_data[0];
  ble_uart_send_quality_check(g_ble_context.quality_check);
}

static void
handle_alarm_request(ble_event_t *event)
{
  rtc_alarm_get(&(g_ble_context.alarm));
  ble_uart_send_alarm(&(g_ble_context.alarm));
}

static void
handle_alarm_command(ble_event_t *event)
{
  memcpy(&(g_ble_context.alarm), &(event->user_data[0]), ALARM_NUM);
  rtc_alarm_set(&(g_ble_context.alarm));
}

static void
handle_alarm_update(ble_event_t *event)
{
  memcpy(g_ble_context.alarm.all, &(event->user_data[0]), ALARM_NUM);
//  LOGV("ble.c 2", "%lu",  (unsigned long) g_ble_context.alarm.minutes_after_midnight );
  ble_uart_send_alarm(&(g_ble_context.alarm));
}

static void
handle_sound_request(ble_event_t *event)
{
  ble_uart_send_sound(g_ble_context.sound);
}

static void
handle_sound_command(ble_event_t *event)
{
  g_ble_context.sound = event->user_data[0];

  char cbuf[5] = {0};
  snprintf(cbuf, sizeof(cbuf), "%d", g_ble_context.sound);
  // TODO: standardize the settings file key naming.
  if( 0 != settings_set_string("bgwav.path.select", cbuf)) {
    // TODO: do something in the event of error.
  }
}

static void
handle_sound_update(ble_event_t *event)
{
  g_ble_context.sound = event->user_data[0];
  ble_uart_send_sound(g_ble_context.sound);
}

static void
handle_time_request(ble_event_t *event)
{
  ble_uart_send_time(g_ble_context.time);
}

static void
handle_time_command(ble_event_t *event)
{
  memcpy(&(g_ble_context.time), &(event->user_data[0]), TIME_NUM);

  // TODO: Save the time, reset the RTC
//  asd

}

static void
handle_time_update(ble_event_t *event)
{
  memcpy(&(g_ble_context.time), &(event->user_data[0]), TIME_NUM);
  ble_uart_send_time(g_ble_context.time);
}

static void
handle_addr_command(ble_event_t *event){
  memcpy(&(g_ble_context.addr), &(event->user_data[0]), ADDR_NUM);
}

static void
handle_state_standby(ble_event_t *event)
{

  switch (event->type) {
    case BLE_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.
      break;

    case BLE_EVENT_BATTERY_LEVEL_REQUEST:
      handle_battery_level_request(event);
      break;

    case BLE_EVENT_BATTERY_LEVEL_UPDATE:
      handle_battery_level_update(event);
      break;

    case BLE_EVENT_SERIAL_NUMBER_REQUEST:
      handle_serial_number_request(event);
      break;

    case BLE_EVENT_SOFTWARE_VERSION_REQUEST:
      handle_software_version_request(event);
      break;

    case BLE_EVENT_DFU_REQUEST:
      handle_ble_dfu_request(event);
      break;

    case BLE_EVENT_ELECTRODE_QUALITY_REQUEST:
      handle_electrode_quality_request(event);
      break;

    case BLE_EVENT_ELECTRODE_QUALITY_UPDATE:
      handle_electrode_quality_update(event);
      break;

    case BLE_EVENT_VOLUME_REQUEST:
      handle_volume_request(event);
      break;

    case BLE_EVENT_VOLUME_COMMAND:
      handle_volume_command(event);
      break;

    case BLE_EVENT_VOLUME_UPDATE:
      handle_volume_update(event);
      break;

    case BLE_EVENT_POWER_REQUEST:
      handle_power_request(event);
      break;

    case BLE_EVENT_POWER_COMMAND:
      handle_power_command(event);
      break;

    case BLE_EVENT_POWER_UPDATE:
      handle_power_update(event);
      break;

    case BLE_EVENT_THERAPY_REQUEST:
      handle_therapy_request(event);
      break;

    case BLE_EVENT_THERAPY_COMMAND:
      handle_therapy_command(event);
      break;

    case BLE_EVENT_THERAPY_UPDATE:
      handle_therapy_update(event);
      break;

    case BLE_EVENT_HEART_RATE_REQUEST:
      handle_heart_rate_request(event);
      break;

    case BLE_EVENT_HEART_RATE_UPDATE:
      handle_heart_rate_update(event);
      break;

    case BLE_EVENT_RESET:
      handle_ble_reset(event);
      break;

    case BLE_EVENT_BLINK_STATUS_REQUEST:
      handle_blink_status_request(event);
      break;

    case BLE_EVENT_BLINK_STATUS_UPDATE:
      handle_blink_status_update(event);
      break;

    case BLE_EVENT_QUALITY_CHECK_REQUEST:
      handle_quality_check_request(event);
      break;

    case BLE_EVENT_QUALITY_CHECK_COMMAND:
      handle_quality_check_command(event);
      break;

    case BLE_EVENT_QUALITY_CHECK_UPDATE:
      handle_quality_check_update(event);
      break;

    case BLE_EVENT_ALARM_REQUEST:
      handle_alarm_request(event);
      break;

    case BLE_EVENT_ALARM_COMMAND:
      handle_alarm_command(event);
      break;

    case BLE_EVENT_ALARM_UPDATE:
      handle_alarm_update(event);
      break;

    case BLE_EVENT_SOUND_REQUEST:
      handle_sound_request(event);
      break;

    case BLE_EVENT_SOUND_COMMAND:
      handle_sound_command(event);
      break;

    case BLE_EVENT_SOUND_UPDATE:
      handle_sound_update(event);
      break;

    case BLE_EVENT_TIME_REQUEST:
      handle_time_request(event);
      break;

    case BLE_EVENT_TIME_COMMAND:
      handle_time_command(event);
      break;

    case BLE_EVENT_TIME_UPDATE:
      handle_time_update(event);
      break;

    case BLE_EVENT_ADDR_COMMAND:
      handle_addr_command(event);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_event(ble_event_t *event)
{
  switch (g_ble_context.state) {
    case BLE_STATE_STANDBY:
      handle_state_standby(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown ble state: %d", (int) g_ble_context.state);
      break;
  }
}

void
ble_pretask_init(void)
{
  // Any pre-scheduler init goes here.
  init_g_ble_context();

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(BLE_EVENT_QUEUE_SIZE,sizeof(ble_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "ble_event_queue");
}


static void
task_init()
{
  // Any post-scheduler init goes here.
  set_state(BLE_STATE_STANDBY);

  LOGV(TAG, "Task launched. Entering event loop.");
}

void
ble_task(void *ignored)
{

  task_init();

  ble_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

  }
}
