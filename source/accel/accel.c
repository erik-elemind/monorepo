/*
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Sept, 2021
 * Author:  Paul Adelsbach
 *
 * Description: Accelerometer task
 *
 */


#include "board_config.h"
#include "accel.h"
#if defined(VARIANT_FF3)
#include "lis2dtw12.h"
#else
#include "bma253.h"
#endif
#include "data_log.h"

#include "fsl_pint.h"
#include "loglevels.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#if (defined(ENABLE_ACCEL_TASK) && (ENABLE_ACCEL_TASK > 0U))

static const char *TAG = "accel"; // Logging prefix for this module

//
// Task events:
//
typedef enum
{
  ACCEL_EVENT_ENTER_STATE,
  ACCEL_EVENT_ISR_DONE,
  ACCEL_EVENT_TURN_OFF,
  ACCEL_EVENT_START_SAMPLE,
  ACCEL_EVENT_START_MOTION_DETECT,
} accel_event_type_t;

// Events are passed to the g_event_queue.
typedef struct
{
  accel_event_type_t type;
} accel_event_t;

//
// State machine states:
//
typedef enum
{
  ACCEL_STATE_BOOT,           // initial state, to ensure we get an "edge" to OFF
  ACCEL_STATE_OFF,            // sensor sleep
  ACCEL_STATE_SAMPLE,         // providing individual samples
  ACCEL_STATE_MOTION_DETECT,  // motion checking only
} accel_state_t;

#if defined(VARIANT_FF3)
#define ACCEL_SAMPLE_TYPE lis2dtw12_sample_t

#define accel_softreset lis2dtw12_softreset
#define accel_init lis2dtw12_init
static void accel_power_set() {
  lis2dtw12_power_set(LIS2DTW12_SINGLE_LOW_PWR_12bit, LIS2DTW12_XL_ODR_OFF);
}
static void accel_range_set() {
  lis2dtw12_range_set(LIS2DTW12_2g);
}
static void accel_fifo_config_set() {
  lis2dtw12_fifo_config_set(LIS2DTW12_FIFO_MODE, 20, true);
}
static bool accel_fifo_samples_get(ACCEL_SAMPLE_TYPE* samples, uint8_t* num_samples) {
  int32_t status = lis2dtw12_fifo_samples_get(samples, num_samples);
  
  return status == 0;
}
// temperature reading in two's compliment. 0x00 is 25c. 16 LSB / degC.
static bool accel_temp_get(int16_t* temperature) {
  int32_t status = lis2dtw12_temp_get(temperature);
  
  return status == 0;
}
static void accel_slope_config_set() {
  lis2dtw12_slope_config_set(4, true); // 2g / 64 * 4 = 125mg
}
#else
#define ACCEL_SAMPLE_TYPE bma253_sample_t

#define accel_softreset bma253_softreset
#define accel_init bma253_init
static void accel_power_set() {
  bma253_power_mode_set(BMA253_POWER_MODE_DEEP_SUSPEND, BMA253_SLEEP_DUR_1000, false);
}
static void accel_range_set() {
  bma253_range_set(BMA253_RANGE_2G);
}
static void accel_fifo_config_set() {
  bma253_fifo_config_set(BMA253_FIFO_MODE_STREAM, 20, true);
}
static bool accel_fifo_samples_get(ACCEL_SAMPLE_TYPE* samples, uint8_t* num_samples) {
  bma253_status_t status = bma253_fifo_samples_get(samples, num_samples);
  if (BMA253_STATUS_SUCCESS == status) {
    return true;
  }
  return false;
}
// temperature reading in two's compliment. 0x00 is 23c. 0.5k/LSB.
static bool accel_temp_get(int16_t* temperature) {
  int8_t temp;
  int32_t status = bma253_temp_get(&temp);

  // Convert to uint16_t
  *temperature = temp;
  
  return status == 0;
}
static void accel_slope_config_set() {
  bma253_slope_config_set(0, true);
}
#endif
//
// Global context data.
// This is used by the accel task only. Public functions and the ISR defer all 
// actual work to the task. The global context is not protected by any OS 
// mechanism (eg. semaphore) and must not be modified in public functions or ISR.
//
typedef struct
{
  accel_state_t state;
  i2c_rtos_handle_t* i2c_handle;
  ACCEL_SAMPLE_TYPE samples[32];    // sample buffer
  int16_t temperature;
  uint32_t accel_sample_num;
} accel_context_t;

static accel_context_t g_context;

// Global event queue and handler:
#define ACCEL_EVENT_QUEUE_SIZE 8
static uint8_t g_event_queue_array[ACCEL_EVENT_QUEUE_SIZE * sizeof(accel_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;

// Function prototypes
static void handle_event(const accel_event_t *event);

static const char *
accel_state_name(accel_state_t state)
{
  switch (state) {
    case ACCEL_STATE_BOOT:          return "ACCEL_STATE_BOOT";
    case ACCEL_STATE_OFF:           return "ACCEL_STATE_OFF";
    case ACCEL_STATE_SAMPLE:        return "ACCEL_STATE_SAMPLE";
    case ACCEL_STATE_MOTION_DETECT: return "ACCEL_STATE_MOTION_DETECT";
    default: 
      break;
  }

  return "ACCEL_STATE_UNKNOWN";
}

static const char *
accel_event_type_name(accel_event_type_t event_type)
{
  switch (event_type) {
    case ACCEL_EVENT_ENTER_STATE:         return "ACCEL_EVENT_ENTER_STATE";
    case ACCEL_EVENT_ISR_DONE:            return "ACCEL_EVENT_ISR_DONE";
    case ACCEL_EVENT_TURN_OFF:            return "ACCEL_EVENT_TURN_OFF";
    case ACCEL_EVENT_START_SAMPLE:        return "ACCEL_EVENT_START_SAMPLE";
    case ACCEL_EVENT_START_MOTION_DETECT: return "ACCEL_EVENT_START_MOTION_DETECT";
    default: 
      break;
  }

  return "ACCEL_EVENT_UNKNOWN";
}

static void
log_event(const accel_event_t *event)
{
  switch (event->type) {
    case ACCEL_EVENT_ISR_DONE:
      // squelch this print
      break;
    default:
      LOGV(TAG, "[%s] Event: %s", accel_state_name(g_context.state), accel_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(const accel_event_t *event)
{
  LOGD(TAG, "[%s] Ignored Event: %s", accel_state_name(g_context.state), accel_event_type_name(event->type));
}

static void
set_state(accel_state_t state)
{
  if (g_context.state == state) {
    LOGD(TAG, "already in state %s", accel_state_name(state));
    return;
  }

  LOGD(TAG, "[%s] -> [%s]", accel_state_name(g_context.state), accel_state_name(state));

  g_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  accel_event_t event = { ACCEL_EVENT_ENTER_STATE };
  handle_event(&event);
}

static void
handle_state_off(const accel_event_t *event)
{
  switch (event->type) {
    case ACCEL_EVENT_ENTER_STATE:
      {
        // perform a softreset of the chip to start fresh
        accel_softreset();
        // wait for max wakeup time of 3ms (Ts,up in the datasheet)
        vTaskDelay(pdMS_TO_TICKS(3));
        // put the accel to sleep
        accel_power_set();
      }
      break;

    case ACCEL_EVENT_START_SAMPLE:
      set_state(ACCEL_STATE_SAMPLE);
      break;

    case ACCEL_EVENT_START_MOTION_DETECT:
      set_state(ACCEL_STATE_MOTION_DETECT);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_sample(const accel_event_t *event)
{
  switch (event->type) {
    case ACCEL_EVENT_ENTER_STATE:
      // perform a softreset of the chip to start fresh
      accel_softreset();
      // wait for max wakeup time of 3ms (Ts,up in the datasheet)
      vTaskDelay(pdMS_TO_TICKS(3));
      // enable low-power mode
      accel_power_set();
      // configure fifo and start sampling
      accel_range_set();
      accel_fifo_config_set();
      // reset sample number
      g_context.accel_sample_num = 0;
      break;

    case ACCEL_EVENT_ISR_DONE:
      {
        uint8_t num_samples = ARRAY_SIZE(g_context.samples);
        // capture the current samples
        bool status = accel_fifo_samples_get(g_context.samples, &num_samples);
        if (status) {

#if 0
          for (uint32_t i=0; i<num_samples; i++) {
            // save the samples
            data_log_accel(
              // save the sample number
              g_context.accel_sample_num++,
              // save the samples
              g_context.samples[i].x,
              g_context.samples[i].y,
              g_context.samples[i].z
            );
          }
          // read the temperature as well
          status = accel_temp_get(&g_context.temperature);
          if (status) {
            // save the temp
            uint32_t accel_temp_sample_num = g_context.accel_sample_num > 0 ? g_context.accel_sample_num-1 : 0;
            data_log_accel_temp(g_context.accel_sample_num, g_context.temperature);
            // log a summary to the console
//            LOGD(TAG, "sample read. num_samples=%u, temp=%d, x=%d, y=%d, z=%d",
//              num_samples, g_context.temperature,
//              g_context.samples[0].x, g_context.samples[0].y, g_context.samples[0].z);
          }
          else {
            LOGW(TAG, "temperature read failed. status=%d", status);
          }
#else
          uint64_t time_us = micros();

          for (uint32_t i=0; i<num_samples; i++) {
            data_log_accel(
              // save the sample number
              time_us - (num_samples-1-i)*40000,
              // save the samples
              g_context.samples[i].x,
              g_context.samples[i].y,
              g_context.samples[i].z
            );
          }
          // read the temperature as well
          status = accel_temp_get(&g_context.temperature);
          if (status) {
            // save the temp
            data_log_accel_temp(time_us, g_context.temperature);
            // log a summary to the console
//            LOGD(TAG, "sample read. num_samples=%u, temp=%d, x=%d, y=%d, z=%d",
//              num_samples, g_context.temperature,
//              g_context.samples[0].x, g_context.samples[0].y, g_context.samples[0].z);
          }
          else {
            LOGW(TAG, "temperature read failed. status=%d", status);
          }

#endif

          // TODO: log data to BLE and flash
        }
        else {
          LOGW(TAG, "sample read failed. status=%d", status);
          // attempt to reinit the chip by entering the state again
          static const accel_event_t event = {.type = ACCEL_EVENT_ENTER_STATE };
          xQueueSend(g_event_queue, &event, 0);
        }
      }
      break;

    case ACCEL_EVENT_TURN_OFF:
      set_state(ACCEL_STATE_OFF);
      break;

    case ACCEL_EVENT_START_MOTION_DETECT:
      set_state(ACCEL_STATE_MOTION_DETECT);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_motion_detect(const accel_event_t *event)
{
    switch (event->type) {
    case ACCEL_EVENT_ENTER_STATE:
      // perform a softreset of the chip to start fresh
      accel_softreset();
      // wait for max wakeup time of 3ms (Ts,up in the datasheet)
      vTaskDelay(pdMS_TO_TICKS(3));

      // configure for motion detection, using default threshold
      accel_range_set();
      accel_slope_config_set();
      break;

    case ACCEL_EVENT_ISR_DONE:
      LOGD(TAG, "motion event");
      break;

    case ACCEL_EVENT_TURN_OFF:
      set_state(ACCEL_STATE_OFF);
      break;

    case ACCEL_EVENT_START_SAMPLE:
      set_state(ACCEL_STATE_SAMPLE);
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_event(const accel_event_t *event)
{
  switch (g_context.state) {
    case ACCEL_STATE_OFF:
      handle_state_off(event);
      break;

    case ACCEL_STATE_SAMPLE:
      handle_state_sample(event);
      break;

    case ACCEL_STATE_MOTION_DETECT:
      handle_state_motion_detect(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown state: %d", (int) g_context.state);
      break;
  }
}

#if defined(VARIANT_FF3)
static int32_t
accel_read_reg(void* ctx, uint8_t reg_addr, uint8_t* data, uint16_t len)
{
  status_t status = i2c_mem_read(
    g_context.i2c_handle,
    (LIS2DTW12_I2C_ADD_L>>1),
    reg_addr, sizeof(reg_addr),
    data, len);
  return kStatus_Success == status ? 0 : -1;
}

static int32_t
accel_write_reg(void* ctx, uint8_t reg_addr, const uint8_t *data, uint16_t len) 
{
  status_t status = i2c_mem_write(
    g_context.i2c_handle,
    (LIS2DTW12_I2C_ADD_L>>1),
    reg_addr, sizeof(reg_addr),
    (uint8_t*)data, len);
  return kStatus_Success == status ? 0 : -1;
}
#else
static int
accel_read_reg(uint8_t reg_addr, uint8_t* data, uint8_t len)
{
  status_t status = i2c_mem_read(
    g_context.i2c_handle,
    BMA253_I2C_ADDR,
    reg_addr, sizeof(reg_addr),
    data, len);
  return kStatus_Success == status ? 0 : -1;
}

static int
accel_write_reg(uint8_t reg_addr, uint8_t data) 
{
  status_t status = i2c_mem_write(
    g_context.i2c_handle,
    BMA253_I2C_ADDR,
    reg_addr, sizeof(reg_addr),
    &data, 1);
  return kStatus_Success == status ? 0 : -1;
}
#endif

void
accel_pretask_init(i2c_rtos_handle_t* i2c_handle)
{
  // Cache the handle of the i2c interface
  g_context.i2c_handle = i2c_handle;

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(
    ACCEL_EVENT_QUEUE_SIZE,
    sizeof(accel_event_t),
    g_event_queue_array,
    &g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "accel_event_queue");
}

static void
task_init(void)
{
  // Any post-scheduler init goes here.
  
  // Init the chip interface
  accel_init(accel_read_reg, accel_write_reg);


  // Initialize the state
  set_state(ACCEL_STATE_OFF);

  LOGV(TAG, "Task launched. Entering event loop.");
}

void
accel_task(void *ignored)
{
  task_init();

  accel_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);

  } // while(1)
}

void
accel_turn_off(void)
{
  static const accel_event_t event = {.type = ACCEL_EVENT_TURN_OFF };
  xQueueSend(g_event_queue, &event, 0);
}

void 
accel_start_sample(void)
{
  static const accel_event_t event = {.type = ACCEL_EVENT_START_SAMPLE };
  xQueueSend(g_event_queue, &event, 0);
}

void 
accel_start_motion_detect(void)
{
  static const accel_event_t event = {.type = ACCEL_EVENT_START_MOTION_DETECT };
  xQueueSend(g_event_queue, &event, 0);
}

/*
 * Pin change interrupt, registered in the function call
 * "accel_pretask_init()" and connected to the accel chip.
 */
void
accel_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // Send Event
  accel_event_t event = {.type = ACCEL_EVENT_ISR_DONE };
  xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);
  // Always do this when calling a FreeRTOS "...FromISR()" function:
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

#else // (defined(ENABLE_ACCEL_TASK) && (ENABLE_ACCEL_TASK > 0U))

void accel_pretask_init(i2c_rtos_handle_t* i2c_handle){}
void accel_task(void *ignored){}
void accel_turn_off(void){}
void accel_start_sample(void){}
void accel_start_motion_detect(void){}

void accel_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status){}

#endif // (defined(ENABLE_ACCEL_TASK) && (ENABLE_ACCEL_TASK > 0U))
