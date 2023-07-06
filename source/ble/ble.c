
#include <stdlib.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "loglevels.h"
#include "config.h"

#include "app.h"
#include "audio_task.h"
#include "ble.h"
#include "ble_uart_commands.h"
#include "ble_uart_recv.h"
#include "ble_uart_send.h"
#include "eeg_reader.h"
#include "rtc.h"

#include "interpreter.h"
#include "settings.h"
#include "fs_commands.h"
#include "fatfs_utils.h"
#include "memfault/metrics/metrics.h"

#if (defined(ENABLE_BLE_TASK) && (ENABLE_BLE_TASK > 0U))

#define BLE_EVENT_QUEUE_SIZE 10

static const char *TAG = "ble";	// Logging prefix for this module

#define   BLE_POWER_OFF_DELAY_MS 50

#define MEMORY_LEVEL_WARN_THRESHOLD 180000000
#define MEMORY_LEVEL_OK_THRESHOLD 300000000

typedef enum
{
	FACTORY_RESET_IDLE		= 0,
	FACTORY_RESET_ACTIVE	= 1,
	FACTORY_RESET_ERROR		= 2,
} factory_reset_t;

typedef enum
{
	MEMORY_LEVEL_OK			= 0,
	MEMORY_LEVEL_WARN		= 1,
	MEMORY_LEVEL_FULL		= 2,
} memory_level_t;

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
  BLE_EVENT_CHARGER_STATUS_REQUEST,
  BLE_EVENT_CHARGER_STATUS_UPDATE,
  BLE_EVENT_SETTINGS_REQUEST,
  BLE_EVENT_SETTINGS_COMMAND,
  BLE_EVENT_SETTINGS_UPDATE,
  BLE_EVENT_MEMORY_LEVEL_REQUEST,
  BLE_EVENT_MEMORY_LEVEL_UPDATE,
  BLE_EVENT_FACTORY_RESET_REQUEST,
  BLE_EVENT_FACTORY_RESET_COMMAND,
  BLE_EVENT_FACTORY_RESET_UPDATE,
  BLE_EVENT_SOUND_CONTROL_REQUEST,
  BLE_EVENT_SOUND_CONTROL_COMMAND,
  BLE_EVENT_SOUND_CONTROL_UPDATE,
  BLE_EVENT_ADDR_COMMAND,
  BLE_EVENT_CONNECTED,
  BLE_EVENT_DISCONNECTED,
  BLE_EVENT_OTA_START,
  BLE_EVENT_OTA_END,
} ble_event_type_t;

// Events are passed to the g_event_queue with an optional
// void *user_data pointer (which may be NULL).
typedef struct
{
  ble_event_type_t type;
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
  uint8_t charger_status;
  uint8_t app_settings;
  memory_level_t memory_level;
  factory_reset_t factory_reset;
  uint8_t sound_control;
  uint8_t addr[ADDR_NUM];
  bool ota;
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
  .software_version = FW_VERSION_FULL,
  .quality_check = 0,
  .alarm = {.all = {0},},
  .sound = 0,
  .time = 0,
  .charger_status = 0,
  .app_settings = 0,
  .memory_level = MEMORY_LEVEL_OK,
  .factory_reset = FACTORY_RESET_IDLE,
  .sound_control = 0,
  .addr = {0,0,0,0,0,0},
  .ota = false,
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
    case BLE_EVENT_CHARGER_STATUS_REQUEST:
	  return "BLE_EVENT_CHARGER_STATUS_REQUEST";
	case BLE_EVENT_CHARGER_STATUS_UPDATE:
	  return "BLE_EVENT_CHARGER_STATUS_UPDATE";
	case BLE_EVENT_SETTINGS_REQUEST:
	  return "BLE_EVENT_SETTINGS_REQUEST";
	case BLE_EVENT_SETTINGS_COMMAND:
	  return "BLE_EVENT_SETTINGS_COMMAND";
	case BLE_EVENT_SETTINGS_UPDATE:
	  return "BLE_EVENT_SETTINGS_UPDATE";
	case BLE_EVENT_MEMORY_LEVEL_REQUEST:
	  return "BLE_EVENT_MEMORY_LEVEL_REQUEST";
	case BLE_EVENT_MEMORY_LEVEL_UPDATE:
	  return "BLE_EVENT_MEMORY_LEVEL_UPDATE";
	 case BLE_EVENT_FACTORY_RESET_REQUEST:
	  return "BLE_EVENT_FACTORY_RESET_REQUEST";
	case BLE_EVENT_FACTORY_RESET_COMMAND:
	  return "BLE_EVENT_FACTORY_RESET_COMMAND";
	case BLE_EVENT_FACTORY_RESET_UPDATE:
	  return "BLE_EVENT_FACTORY_RESET_UPDATE";
	case BLE_EVENT_SOUND_CONTROL_REQUEST:
	  return "BLE_EVENT_SOUND_CONTROL_REQUEST";
	case BLE_EVENT_SOUND_CONTROL_COMMAND:
	  return "BLE_EVENT_SOUND_CONTROL_COMMAND";
	case BLE_EVENT_SOUND_CONTROL_UPDATE:
	  return "BLE_EVENT_SOUND_CONTROL_UPDATE";
    case BLE_EVENT_ADDR_COMMAND:
      return "BLE_EVENT_ADDR_COMMAND";
    case BLE_EVENT_CONNECTED:
      return "BLE_EVENT_CONNECTED";
    case BLE_EVENT_DISCONNECTED:
      return "BLE_EVENT_DISCONNECTED";
    case BLE_EVENT_OTA_START:
    	return "BLE_EVENT_OTA_START";
    case BLE_EVENT_OTA_END:
    	return "BLE_EVENT_OTA_END";
    default:
      break;
  }
  return "BLE_EVENT UNKNOWN";
}

void ble_connected_event(void)
{
	ble_event_t event = {.type = BLE_EVENT_CONNECTED };
	xQueueSend(g_event_queue, &event, 0);
}

void ble_disconnected_event(void)
{
	ble_event_t event = {.type = BLE_EVENT_DISCONNECTED };
	xQueueSend(g_event_queue, &event, 0);
}

void ble_ota_start_event(void)
{
	ble_event_t event = {.type = BLE_EVENT_OTA_START };
	xQueueSend(g_event_queue, &event, 0);
}

void ble_ota_end_event(void)
{
	ble_event_t event = {.type = BLE_EVENT_OTA_END };
	xQueueSend(g_event_queue, &event, 0);
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
  LOGV(TAG,"THERAPY: %d",therapy);

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
  memcpy(event.user_data, &(params->all[0]), sizeof(alarm_params_t));
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

void ble_charger_status_request(void)
{
	ble_event_t event = { .type = BLE_EVENT_CHARGER_STATUS_REQUEST };
	xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_charger_status_update(uint8_t charger_status)
{
	ble_event_t event = { .type = BLE_EVENT_CHARGER_STATUS_UPDATE,
			.user_data = {charger_status} };
	xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_settings_request(void)
{
	ble_event_t event = { .type = BLE_EVENT_SETTINGS_REQUEST };
	xQueueSend(g_event_queue, &event, 0);
}

void ble_settings_command(uint8_t settings)
{
	ble_event_t event = { .type = BLE_EVENT_SETTINGS_COMMAND,
			.user_data = {settings} };
	xQueueSend(g_event_queue, &event, 0);
}

void ble_settings_update(uint8_t settings)
{
	ble_event_t event = { .type = BLE_EVENT_SETTINGS_UPDATE,
			.user_data = {settings} };
	xQueueSend(g_event_queue, &event, 0);
}

void ble_memory_level_request(void)
{
	ble_event_t event = { .type = BLE_EVENT_MEMORY_LEVEL_REQUEST };
	xQueueSend(g_event_queue, &event, 0);
}

void ble_memory_level_update(uint8_t memory_level)
{
	ble_event_t event = { .type = BLE_EVENT_MEMORY_LEVEL_UPDATE,
			.user_data = {memory_level} };
	xQueueSend(g_event_queue, &event, 0);
}

void ble_factory_reset_request(void)
{
	 ble_event_t event = { .type = BLE_EVENT_FACTORY_RESET_REQUEST };
	 xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_factory_reset_command(uint8_t factory_reset)
{
	ble_event_t event = { .type = BLE_EVENT_FACTORY_RESET_COMMAND,
			.user_data = {factory_reset} };
	xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_factory_reset_update(uint8_t factory_reset)
{
	 ble_event_t event = { .type = BLE_EVENT_FACTORY_RESET_UPDATE,
			 .user_data = {factory_reset} };
	 xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_sound_control_request(void)
{
	ble_event_t event = { .type = BLE_EVENT_SOUND_CONTROL_REQUEST };
	xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_sound_control_command(uint8_t sound_control)
{
	 ble_event_t event = { .type = BLE_EVENT_SOUND_CONTROL_COMMAND,
			 .user_data = {sound_control} };
	 xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void ble_sound_control_update(uint8_t sound_control)
{
	ble_event_t event = { .type = BLE_EVENT_SOUND_CONTROL_UPDATE,
			.user_data = {sound_control} };
	xQueueSend(g_event_queue, &event, portMAX_DELAY);
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

bool ble_is_ota_running(void) {
	return g_ble_context.ota;
}

static void
log_event(ble_event_t *event)
{
  switch (event->type) {
    case BLE_EVENT_ELECTRODE_QUALITY_UPDATE:
    // suppress electrode quality update event
      break;
    default:
      LOGV(TAG, "[%s] Event: %s\n\r", ble_state_name(g_ble_context.state),
        ble_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(ble_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s\n\r", ble_state_name(g_ble_context.state), ble_event_type_name(event->type));
}

static void
set_state(ble_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]\n\r", ble_state_name(g_ble_context.state), ble_state_name(state));

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

  if (g_ble_context.therapy == THERAPY_TYPE_NONE)
  {
	  // send RTC event
    interpreter_event_start_alarm();
  }
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
  memcpy(&(g_ble_context.alarm), &(event->user_data[0]), sizeof(alarm_params_t));
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
  rtc_set(g_ble_context.time);
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
handle_charger_status_request(ble_event_t *event)
{
	ble_uart_send_charger_status(g_ble_context.charger_status);
}

static void
handle_charger_status_update(ble_event_t *event)
{
	g_ble_context.charger_status = event->user_data[0];
	ble_uart_send_charger_status(g_ble_context.charger_status);
}

static void
handle_settings_request(ble_event_t *event)
{
	ble_uart_send_settings(g_ble_context.app_settings);
}

static void
handle_settings_command(ble_event_t *event)
{
	// Make sure settings have changed to minimize file writes
	if(g_ble_context.app_settings == event->user_data[0])
	{
		return;
	}

	g_ble_context.app_settings = event->user_data[0];

	// Update settings.ini file
	if(write_app_settings(g_ble_context.app_settings) != SETTINGS_RESULT_SUCCESS)	{
		LOGE(TAG, "Failed to write app settings to settings.ini");
	}
}

static void
handle_settings_update(ble_event_t *event)
{
	g_ble_context.app_settings = event->user_data[0];

	// Update settings.ini file
	if(write_app_settings(g_ble_context.app_settings) != SETTINGS_RESULT_SUCCESS)
	{
		LOGE(TAG, "Failed to write app settings to settings.ini");
	}

	ble_uart_send_settings(g_ble_context.app_settings);
}

static void
handle_memory_level_request(ble_event_t *event)
{
	DWORD free_bytes;

	// Check the current memory level on the file system, and update char
	if(f_getfreebytes(&free_bytes, NULL) == FR_OK)
	{
		if(free_bytes >= MEMORY_LEVEL_OK_THRESHOLD)
		{
			g_ble_context.memory_level = MEMORY_LEVEL_OK;
		}
		else if(free_bytes >= MEMORY_LEVEL_WARN_THRESHOLD)
		{
			g_ble_context.memory_level = MEMORY_LEVEL_WARN;
		}
		else
		{
			g_ble_context.memory_level = MEMORY_LEVEL_FULL;
		}

		ble_uart_send_memory_level(g_ble_context.memory_level);
	}
	else
	{
		LOGE(TAG, "Unable to check memory level of file system");
	}


}

static void
handle_memory_level_update(ble_event_t *event)
{
	g_ble_context.memory_level = event->user_data[0];
	ble_uart_send_memory_level(g_ble_context.memory_level);
}

static void
handle_factory_reset_request(ble_event_t *event)
{
	ble_uart_send_factory_reset(g_ble_context.factory_reset);
}

static void
handle_factory_reset_command(ble_event_t *event)
{
	FRESULT result;
	DIR dir;
	FILINFO finfo;
	char buf[MAX_PATH_LENGTH+2];
	const char *datalog_name = "/datalogs";
	const char *usermetrics_name = "/user_metrics";
	char *sub;
	int ret = 0;

	LOGV(TAG, "Handle Factory Reset");

	g_ble_context.factory_reset = event->user_data[0];


	if(g_ble_context.factory_reset == FACTORY_RESET_ACTIVE)
	{
		// Remove raw data logs
		result = f_opendir(&dir, datalog_name);
		if (FR_OK != result) {
			LOGW(TAG, "Can't open '%s' directory", datalog_name);
		}
		else {
			result = f_readdir(&dir, &finfo);
			// FatFS indicates end of directory with a null name
			while (FR_OK == result && 0 != finfo.fname[0]) {
				strncpy(buf, datalog_name, sizeof(buf)-1);
				buf[sizeof(buf)-1] = '\0';
				sub = buf;
				if (datalog_name[strlen(datalog_name)-1] != '/') {
					sub = strcat(buf, "/");
				}
				sub = strcat(sub, finfo.fname);
				ret = f_unlink(sub);
				if (ret == 0) {
					LOGV(TAG, "Delete '%s' succ.", sub);
				}
				else {
					LOGE(TAG, "Delete '%s' fail!", sub);
				}
				// move to next entry
				result = f_readdir(&dir, &finfo);
			}

			result = f_closedir(&dir);

			if(result != FR_OK)
			{
				LOGE(TAG, "Failed to delete raw data log files");
				ble_factory_reset_update(FACTORY_RESET_ERROR);
				return;
			}
		}

		// Remove user_metric files
		result = f_opendir(&dir, usermetrics_name);
		if (FR_OK != result) {
			LOGW(TAG, "Can't open '%s' directory", usermetrics_name);
		}
		else {
			result = f_readdir(&dir, &finfo);
			// FatFS indicates end of directory with a null name
			while (FR_OK == result && 0 != finfo.fname[0]) {
				strncpy(buf, usermetrics_name, sizeof(buf)-1);
				buf[sizeof(buf)-1] = '\0';
				sub = buf;
				if (usermetrics_name[strlen(usermetrics_name)-1] != '/') {
					sub = strcat(buf, "/");
				}
				sub = strcat(sub, finfo.fname);
				ret = f_unlink(sub);
				if (ret == 0) {
					LOGV(TAG, "Delete '%s' succ.", sub);
				}
				else {
					LOGE(TAG, "Delete '%s' fail!", sub);
				}
				// move to next entry
				result = f_readdir(&dir, &finfo);
			}

			result = f_closedir(&dir);

			if(result != FR_OK)
			{
				LOGE(TAG, "Failed to delete user metric files");
				ble_factory_reset_update(FACTORY_RESET_ERROR);
				return;
			}
		}


		// Reset settings to default values
		if(reset_default_settings() == SETTINGS_RESULT_SUCCESS)
		{
			uint8_t app_settings = 0;
			LOGV(TAG, "Settings reset");

			// Update app settings characteristic
			if(read_app_settings(&app_settings) == SETTINGS_RESULT_SUCCESS)
			{
				g_ble_context.app_settings = app_settings;
				ble_settings_update(g_ble_context.app_settings);
			}

			// Update factory setting characteristic
			ble_factory_reset_update(FACTORY_RESET_IDLE);
		}
		else
		{
			LOGE(TAG, "Failed settings reset");
			ble_factory_reset_update(FACTORY_RESET_ERROR);
			return;
		}

	}
	else
	{
		LOGV(TAG,"Set Factory Reset to: %d", g_ble_context.factory_reset);
	}
}

static void
handle_factory_reset_update(ble_event_t *event)
{
	g_ble_context.factory_reset = event->user_data[0];
	ble_uart_send_factory_reset(g_ble_context.factory_reset);
}

static void
handle_sound_control_request(ble_event_t *event)
{
	ble_uart_send_sound_control(g_ble_context.sound_control);
}

static void
handle_sound_control_command(ble_event_t *event)
{
  g_ble_context.sound_control = event->user_data[0];

  // if sound command is OFF
  if (g_ble_context.sound_control == 0)
  {
    if (g_ble_context.therapy == 1)
    {
      // do nothing
      return;
    }
    else
    {
      // stop audio
      audio_bg_fadeout(0);
      audio_bgwav_stop();
    }
  }
  // if sound command is ON, play audio
  else
  {
    const char* select_filepath = "S.bgwav.path.select";
    audio_bg_fadeout(0);
    audio_bgwav_stop();
    audio_set_volume_ble(audio_get_volume());
    audio_bg_script_volume(0.2);
    audio_bg_fadein(0);

    // FOLLOWING CODE PULLS FROM COMMAND FUNCTION (audio_bgwav_play_command, PARSE_VAR_RETURN_SETTINGS_VALUE)
    // TODO: figure out cleaner implementation (scripts? commands? etc)

    bool success = true;
    char* audio_file = NULL;
    char parse_var_buf[10] = {0};
    char settings_bgwav_path[256] = {0};
    parse_variable_from_string((char *)select_filepath, parse_var_buf, sizeof(parse_var_buf));

    // TODO: Do something with tokens
    token_t tokens;
    parse_dot_notation(parse_var_buf, sizeof(parse_var_buf), &tokens);

    int settings_bgwav_select = atoi(parse_var_buf);
    char settings_bgwav_path_key[20] = {0};
    snprintf(settings_bgwav_path_key, sizeof(settings_bgwav_path_key), "bgwav.path.%d", settings_bgwav_select);
    if ( 0 == settings_get_string(settings_bgwav_path_key, settings_bgwav_path, sizeof(settings_bgwav_path)) )
    {
      audio_file = settings_bgwav_path;
      audio_bgwav_play(audio_file, true);
    }
    else
    {
      LOGE(TAG,"Failed to get path to audio file.");
    }
  }
}

static void
handle_sound_control_update(ble_event_t *event)
{
	g_ble_context.sound_control = event->user_data[0];
	ble_uart_send_sound_control(g_ble_context.sound_control);
}

static void
handle_ble_connected(ble_event_t *event)
{
	eeg_reader_event_ble_connected();
	if (interpreter_get_alarm_status())
	{
		// turn off alarm on BLE connect if running
		interpreter_event_stop_script(false);
	}

	// Measure and send memory level to BLE
	ble_memory_level_request();

	memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(ble_num_connections), 1);

    // flush memfault event logs to file system on connection
    memfault_save_eventlog_chunks();
}

static void
handle_ble_disconnected(ble_event_t *event)
{
	eeg_reader_event_ble_disconnected();
	memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(ble_num_disconnections), 1);
}

static void
handle_ble_ota_started(ble_event_t *event)
{
	// tell interpreter to stop
	interpreter_event_stop_script(false);

	// tell syncs to stop
	ymodem_end_session();

	// set OTA flag to true to prevent Sleeping
	g_ble_context.ota = true;

	// TODO: add Memfault metric and flush to FS
}

static void
handle_ble_ota_ended(ble_event_t *event)
{
	// set OTA flag to false
	g_ble_context.ota = false;

	// TODO: add Memfault metric and flush to FS
}

static void
handle_state_standby(ble_event_t *event)
{

  switch (event->type) {
    case BLE_EVENT_ENTER_STATE:
      // Generic code to always execute when entering this state goes here.
      break;

    case BLE_EVENT_CONNECTED:
    	handle_ble_connected(event);
    	break;

    case BLE_EVENT_DISCONNECTED:
    	handle_ble_disconnected(event);
    	break;

    case BLE_EVENT_OTA_START:
    	handle_ble_ota_started(event);
    	break;

    case BLE_EVENT_OTA_END:
    	handle_ble_ota_ended(event);
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

    case BLE_EVENT_CHARGER_STATUS_REQUEST:
      handle_charger_status_request(event);
      break;

    case BLE_EVENT_CHARGER_STATUS_UPDATE:
      handle_charger_status_update(event);
      break;

    case BLE_EVENT_SETTINGS_REQUEST:
      handle_settings_request(event);
      break;

    case BLE_EVENT_SETTINGS_COMMAND:
      handle_settings_command(event);
      break;

    case BLE_EVENT_SETTINGS_UPDATE:
      handle_settings_update(event);
      break;

    case BLE_EVENT_MEMORY_LEVEL_REQUEST:
      handle_memory_level_request(event);
      break;

    case BLE_EVENT_MEMORY_LEVEL_UPDATE:
      handle_memory_level_update(event);
      break;

    case BLE_EVENT_FACTORY_RESET_REQUEST:
      handle_factory_reset_request(event);
      break;

    case BLE_EVENT_FACTORY_RESET_COMMAND:
      handle_factory_reset_command(event);
      break;

    case BLE_EVENT_FACTORY_RESET_UPDATE:
      handle_factory_reset_update(event);
      break;

    case BLE_EVENT_SOUND_CONTROL_REQUEST:
      handle_sound_control_request(event);
      break;

    case BLE_EVENT_SOUND_CONTROL_COMMAND:
      handle_sound_control_command(event);
      break;

    case BLE_EVENT_SOUND_CONTROL_UPDATE:
      handle_sound_control_update(event);
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
      app_event_ble_activity();
      handle_state_standby(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown ble state: %d\n\r", (int) g_ble_context.state);
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

  // Read off last saved settings from the settings.ini file
  if(read_app_settings(&g_ble_context.app_settings) != SETTINGS_RESULT_SUCCESS)
  {
	  LOGE(TAG, "Error reading app settings from settings.ini");
  }

  LOGV(TAG, "Task launched. Entering event loop.\n\r");
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

#else // (defined(ENABLE_BLE_TASK) && (ENABLE_BLE_TASK > 0U))

void ble_pretask_init(void){}
void ble_task(void *ignored){}

void ble_battery_level_request(void){}
void ble_battery_level_update(uint8_t battery_level){}
void ble_serial_number_request(void){}
void ble_software_version_request(void){}
void ble_dfu_request(void){}

void ble_electrode_quality_request(void){}
void ble_electrode_quality_update(uint8_t qualities[ELECTRODE_NUM]){}
void ble_volume_request(void){}
void ble_volume_command(uint8_t volume){}
void ble_volume_update(uint8_t volume){}
void ble_power_request(void){}
void ble_power_command(uint8_t power){}
void ble_power_update(uint8_t power){}
void ble_therapy_request(void){}
void ble_therapy_command(uint8_t therapy){}
void ble_therapy_update(uint8_t therapy){}
void ble_heart_rate_request(void){}
void ble_heart_rate_update(uint8_t heart_rate){}
void ble_reset(void){}
void ble_blink_status_request(void){}
void ble_blink_status_update(uint8_t status[BLINK_NUM]){}
void ble_quality_check_request(void){}
void ble_quality_check_command(uint8_t quality_check){}
void ble_quality_check_update(uint8_t quality_check){}
void ble_alarm_request(void){}
void ble_alarm_command(alarm_params_t *params){}
void ble_alarm_update(alarm_params_t *params){}
void ble_sound_request(void){}
void ble_sound_command(uint8_t sound){}
void ble_sound_update(uint8_t sound){}
void ble_time_request(void){}
void ble_time_command(uint64_t unix_epoch_time_sec){}
void ble_time_update(uint64_t unix_epoch_time_sec){}
void ble_addr_command(uint8_t* addr){}

uint8_t* ble_get_addr(){ static uint8_t addr[] = {0xFF}; return addr; }

void ble_power_off(void){}
void ble_power_on(void){}

#endif // (defined(ENABLE_BLE_TASK) && (ENABLE_BLE_TASK > 0U))
