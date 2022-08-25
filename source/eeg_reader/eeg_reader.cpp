/*
 * eeg_reader.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: Aug, 2020
 * Author:  David Wang
 */

#include <stdlib.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "message_buffer.h"

#include "utils.h"
#include "loglevels.h"
#include "config.h"
#include "config_tracealyzer_isr.h"

#include "ads129x_command.h"
#include "ads129x.h"

#include "eeg_constants.h"
#include "eeg_reader.h"
#include "eeg_processor.h"
#include "interpreter.h"

#include "data_log.h"


#if (defined(ENABLE_EEG_READER_TASK) && (ENABLE_EEG_READER_TASK > 0U))


#define EEG_READER_EVENT_QUEUE_SIZE 20


static const char *TAG = "eeg_reader";  // Logging prefix for this module


// Optimization: Prevent the need for stack allocation of this in the ISR by 
// making it global:
#if 1
static spi_transfer_t g_eeg_spi_transfer;
#endif

//
// Task events:
//
typedef enum
{
  EEG_READER_EVENT_ENTER_STATE,	// (used for state transitions)
  EEG_READER_EVENT_GET_INFO,
  EEG_READER_EVENT_POWER_ON,
  EEG_READER_EVENT_POWER_OFF,
  EEG_READER_EVENT_START,
  EEG_READER_EVENT_STOP,
  EEG_READER_EVENT_SET_ADS_GAIN,
  EEG_READER_EVENT_GET_GAIN,
  EEG_READER_EVENT_DRDY,
  EEG_READER_EVENT_TEMP_SAMPLE,
} eeg_reader_event_type_t;

// Events are passed to the g_event_queue with an optional 
// void *user_data pointer (which may be NULL).
typedef struct
{
  eeg_reader_event_type_t type;
  union{
    uint8_t ads_gain;
    int32_t temp;
  } data;
} eeg_reader_event_t;


//
// State machine states:
//
typedef enum
{
  EEG_READER_STATE_OFF,
  EEG_READER_STATE_STANDBY,
  EEG_READER_STATE_SAMPLING,
} eeg_reader_state_t;

//
// Global context data:
//
typedef struct
{
  eeg_reader_state_t state;
  // ads object to be used in the task
  ads129x ads;
  uint8_t ads_gain;
} eeg_reader_context_t;

static eeg_reader_context_t g_eeg_reader_context;
static TaskHandle_t g_eeg_reader_task_handle = NULL;

static volatile timestamp_union_t g_timestamp_union;
static volatile sample_number_union_t g_sample_number_union;
// SPI input buffer - used only by ISR
static uint8_t g_spi_bytes[SPI_BUFFER_SIZE];

// Global event queue and handler:
static uint8_t g_event_queue_array[EEG_READER_EVENT_QUEUE_SIZE*sizeof(eeg_reader_event_t)];
static StaticQueue_t g_event_queue_struct;
static QueueHandle_t g_event_queue;
static void handle_event(eeg_reader_event_t *event);

static void arrange_and_send_eeg_channels_from_isr(BaseType_t *pxHigherPriorityTaskWoken);

#if (defined(USE_EEG_INTERRUPT_INITIATED_DMA) && (USE_EEG_INTERRUPT_INITIATED_DMA > 0U))
#else
static spi_master_handle_t g_spi_handle;
static void eeg_spi_rx_complete_isr(SPI_Type *base, spi_master_handle_t *handle, status_t status, void *userData);
static void handle_eeg_drdy_pint_isr();
#endif
static void handle_eeg_temp_sample(int32_t temp);

// For logging and debug:
static const char *
eeg_reader_state_name(eeg_reader_state_t state)
{
  switch (state) {
    case EEG_READER_STATE_OFF:      return "EEG_READER_STATE_OFF";
    case EEG_READER_STATE_STANDBY:  return "EEG_READER_STATE_STANDBY";
    case EEG_READER_STATE_SAMPLING: return "EEG_READER_STATE_SAMPLING";
    default:
      break;
  }
  return "EEG_READER_STATE UNKNOWN";
}

static const char *
eeg_reader_event_type_name(eeg_reader_event_type_t event_type)
{
  switch (event_type) {
    case EEG_READER_EVENT_ENTER_STATE:  return "EEG_READER_EVENT_ENTER_STATE";
    case EEG_READER_EVENT_GET_INFO:     return "EEG_READER_EVENT_GET_INFO";
    case EEG_READER_EVENT_POWER_ON:     return "EEG_READER_EVENT_POWER_ON";
    case EEG_READER_EVENT_POWER_OFF:    return "EEG_READER_EVENT_POWER_OFF";
    case EEG_READER_EVENT_START:        return "EEG_READER_EVENT_START";
    case EEG_READER_EVENT_STOP:         return "EEG_READER_EVENT_STOP";
    case EEG_READER_EVENT_SET_ADS_GAIN: return "EEG_READER_EVENT_SET_ADS_GAIN";
    case EEG_READER_EVENT_GET_GAIN:     return "EEG_READER_EVENT_GET_GAIN";
    case EEG_READER_EVENT_DRDY:         return "EEG_READER_EVENT_DRDY";
    case EEG_READER_EVENT_TEMP_SAMPLE:  return "EEG_READER_EVENT_TEMP_SAMPLE";
    default:
      break;
  }
  return "EEG_READER_EVENT UNKNOWN";
}

void
eeg_reader_event_get_info(void)
{
  eeg_reader_event_t event = {.type = EEG_READER_EVENT_GET_INFO, {0}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
eeg_reader_event_power_on(void)
{
  eeg_reader_event_t event = {.type = EEG_READER_EVENT_POWER_ON, {0}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
eeg_reader_event_power_off(void)
{
  eeg_reader_event_t event = {.type = EEG_READER_EVENT_POWER_OFF, {0}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
eeg_reader_event_start(void)
{
  eeg_reader_event_t event = {.type = EEG_READER_EVENT_START, {0}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void
eeg_reader_event_stop(void)
{
  eeg_reader_event_t event = {.type = EEG_READER_EVENT_STOP, {0}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

void eeg_reader_event_set_ads_gain(uint8_t ads_gain){
  // stuff the gain into the user data
  eeg_reader_event_t event = {.type = EEG_READER_EVENT_SET_ADS_GAIN, .data = {.ads_gain = ads_gain}};
  xQueueSend(g_event_queue, &event, portMAX_DELAY);
}

//void eeg_reader_event_get_gain(){
//  // stuff the gain into the user data
//  eeg_reader_event_t event = {.type = EEG_READER_EVENT_GET_GAIN, {0}};
//  xQueueSend(g_event_queue, &event, portMAX_DELAY);
//}

float eeg_reader_get_total_gain(){
  // TODO: Should we protect g_eeg_reader_context.ads_gain with a mutex?
  float gain = g_eeg_reader_context.ads_gain*EEG_PREAMP_GAIN;
  return gain;
}

static void
log_event(eeg_reader_event_t *event)
{
  switch (event->type) {
    case EEG_READER_EVENT_DRDY:
    case EEG_READER_EVENT_TEMP_SAMPLE:
      // do nothing -- suppress printing EEG MSG events
      break;
    default:
      LOGV(TAG, "[%s] Event: %s", eeg_reader_state_name(g_eeg_reader_context.state), eeg_reader_event_type_name(event->type));
      break;
  }
}

static void
log_event_ignored(eeg_reader_event_t *event)
{
  switch(event->type){
    case EEG_READER_EVENT_DRDY:
    case EEG_READER_EVENT_TEMP_SAMPLE:
      // do nothing -- suppress printing EEG MSG events
      break;
    default:
      LOGD(TAG, "[%s] Ignored Event: %s", eeg_reader_state_name(g_eeg_reader_context.state), eeg_reader_event_type_name(event->type));
      break;
  }
}

static void
set_state(eeg_reader_state_t state)
{
  LOGD(TAG, "[%s] -> [%s]", eeg_reader_state_name(g_eeg_reader_context.state), eeg_reader_state_name(state));

  g_eeg_reader_context.state = state;

  // Immediately process an ENTER_STATE event, before any other pending events.
  // This allows the app to do state-specific init/setup when changing states.
  eeg_reader_event_t event = { EEG_READER_EVENT_ENTER_STATE, /* (void *) state*/ {0} };
  handle_event(&event);
}


//
// Event handlers for the various application states:
//

static void
handle_state_off(eeg_reader_event_t *event)
{
  switch (event->type) {
    case EEG_READER_EVENT_ENTER_STATE:
      ads_off( &(g_eeg_reader_context.ads) );
      break;

    case EEG_READER_EVENT_POWER_ON:
      set_state(EEG_READER_STATE_STANDBY);
      break;

    case EEG_READER_EVENT_SET_ADS_GAIN:
      // record gain to context
      g_eeg_reader_context.ads_gain = event->data.ads_gain;
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void
handle_state_standby(eeg_reader_event_t *event)
{

  switch (event->type) {
    case EEG_READER_EVENT_ENTER_STATE:
      ads_on( &(g_eeg_reader_context.ads) );
      // set gain
      ads_set_gain( &(g_eeg_reader_context.ads), g_eeg_reader_context.ads_gain );
      break;

    case EEG_READER_EVENT_POWER_OFF:
      set_state(EEG_READER_STATE_OFF);
      break;

    case EEG_READER_EVENT_START:
      set_state(EEG_READER_STATE_SAMPLING);
      break;

    case EEG_READER_EVENT_GET_INFO:
      ads_info_command( &(g_eeg_reader_context.ads) );
      break;

    case EEG_READER_EVENT_SET_ADS_GAIN:
      // set gain
      ads_set_gain( &(g_eeg_reader_context.ads), event->data.ads_gain );
      // record gain to context
      g_eeg_reader_context.ads_gain = event->data.ads_gain;
      break;

    default:
      log_event_ignored(event);
      break;
  }
}

static void stop_spi_dma(){
  // Disable eeg_drdy_pint_isr callback
  PINT_DisableCallbackByIndex(PINT_PERIPHERAL, kPINT_PinInt4);

  vTaskDelay(pdMS_TO_TICKS(8)); // delay twice the period of the 250Hz sampling rate.
}

static void start_spi_dma(){
  // Enable eeg_drdy_pint_isr callback
  PINT_EnableCallbackByIndex(PINT_PERIPHERAL, kPINT_PinInt4);
}


static void
handle_state_sampling(eeg_reader_event_t *event)
{
  switch (event->type) {
    case EEG_READER_EVENT_ENTER_STATE:
      // Stop continuous sampling mode of ADS1298.
      stop_spi_dma();
      ads_sdatac_command( &(g_eeg_reader_context.ads) );

      // reset sample number
      taskENTER_CRITICAL();
      g_sample_number_union.sample_number = 0;
      taskEXIT_CRITICAL();

      // initialize eeg processing
      eeg_processor_init();

      // Start continuous sampling mode of ADS1298.
      ads_rdatac_command( &(g_eeg_reader_context.ads) );
      start_spi_dma();
      break;

    case EEG_READER_EVENT_POWER_OFF:
      // Stop continuous sampling mode of ADS1298.
      stop_spi_dma();
      ads_sdatac_command( &(g_eeg_reader_context.ads) );

      set_state(EEG_READER_STATE_OFF);
      break;

    case EEG_READER_EVENT_STOP:
      // Stop continuous sampling mode of ADS1298.
      stop_spi_dma();
      ads_sdatac_command( &(g_eeg_reader_context.ads) );

      set_state(EEG_READER_STATE_STANDBY);
      break;

#if 0
    case EEG_READER_EVENT_SET_ADS_GAIN:
      // Stop continuous sampling mode of ADS1298.
      stop_spi_dma();
      ads_sdatac_command( &(g_eeg_reader_context.ads) );

      // set gain
      ads_set_gain( &(g_eeg_reader_context.ads), event->data.ads_gain );
      // record gain to context
      g_eeg_reader_context.ads_gain = event->data.ads_gain;

      // Resume continuous sampling mode of ADS1298
      ads_rdatac_command( &(g_eeg_reader_context.ads) );
      start_spi_dma();
      break;
#endif

    default:
      log_event_ignored(event);
      break;
  }
}


static void
handle_event(eeg_reader_event_t *event)
{
  // handle stateless events
  switch (event->type) {
#if (defined(USE_EEG_INTERRUPT_INITIATED_DMA) && (USE_EEG_INTERRUPT_INITIATED_DMA > 0U))
#else
  case EEG_READER_EVENT_DRDY:
    handle_eeg_drdy_pint_isr();
    return;
#endif

  case EEG_READER_EVENT_TEMP_SAMPLE:
    handle_eeg_temp_sample(event->data.temp);
    return;

  default:
    break;
  }

  switch (g_eeg_reader_context.state) {
    case EEG_READER_STATE_OFF:
      handle_state_off(event);
      break;

    case EEG_READER_STATE_STANDBY:
      handle_state_standby(event);
      break;

    case EEG_READER_STATE_SAMPLING:
      handle_state_sampling(event);
      break;

    default:
      // (We should never get here.)
      LOGE(TAG, "Unknown eeg_reader state: %d", (int) g_eeg_reader_context.state);
      break;
  }
}

void
eeg_reader_pretask_init(void)
{
  // Any pre-scheduler init goes here.

  // Create the event queue before the scheduler starts. Avoids race conditions.
  g_event_queue = xQueueCreateStatic(EEG_READER_EVENT_QUEUE_SIZE,sizeof(eeg_reader_event_t),g_event_queue_array,&g_event_queue_struct);
  vQueueAddToRegistry(g_event_queue, "eeg_reader_event_queue");

#if (defined(USE_EEG_INTERRUPT_INITIATED_DMA) && (USE_EEG_INTERRUPT_INITIATED_DMA > 0U))
#else
  status_t status = SPI_MasterTransferCreateHandle(SPI_EEG_BASE,
      &g_spi_handle,
      eeg_spi_rx_complete_isr,
      NULL);
  if(status != kStatus_Success){
    LOGV(TAG,"Failed to initialize SPIMasterTransferHandle");
  }
#endif
}


static void
task_init()
{
  // Any post-scheduler init goes here.
  g_eeg_reader_task_handle = xTaskGetCurrentTaskHandle();

  // Initialize ADS1298 charge pump and LDO.
  ads_init(&(g_eeg_reader_context.ads));
  g_eeg_reader_context.ads_gain = ADS_GAIN_DEFAULT; // TODO get this gain from the device

  set_state(EEG_READER_STATE_OFF);

  // disable eeg data ready interrupt, which is automatically enabled in peripherals.c
  PINT_DisableCallbackByIndex(PINT_PERIPHERAL, kPINT_PinInt4);

  // power up (high on power down pin)
  eeg_reader_event_power_on();

  LOGV(TAG, "Task launched. Entering event loop.");
}

void
eeg_reader_task(void *ignored)
{

  task_init();

  eeg_reader_event_t event;

  while (1) {

    // Get the next event.
    xQueueReceive(g_event_queue, &event, portMAX_DELAY);

    log_event(&event);

    handle_event(&event);
  }
}


#if (defined(USE_EEG_INTERRUPT_INITIATED_DMA) && (USE_EEG_INTERRUPT_INITIATED_DMA > 0U))

void eeg_drdy_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status){
  ads129x *ads = &(g_eeg_reader_context.ads);

//  g_timestamp_union.timestamp = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
//  g_spi_bytes[0] = g_timestamp_union.timestamp_bytes[0];
//  g_spi_bytes[1] = g_timestamp_union.timestamp_bytes[1];
//  g_spi_bytes[2] = g_timestamp_union.timestamp_bytes[2];
//  g_spi_bytes[3] = g_timestamp_union.timestamp_bytes[3];
  g_spi_bytes[4] = g_sample_number_union.sample_number_bytes[0];
  g_spi_bytes[5] = g_sample_number_union.sample_number_bytes[1];
  g_spi_bytes[6] = g_sample_number_union.sample_number_bytes[2];
  g_spi_bytes[7] = g_sample_number_union.sample_number_bytes[3];
  g_sample_number_union.sample_number++;

  // Prepare the DMA SPI Transfer
  g_eeg_spi_transfer.txData   = NULL;
  g_eeg_spi_transfer.dataSize = ads->num_spi_bytes;
  g_eeg_spi_transfer.rxData   = g_spi_bytes + TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES;
  g_eeg_spi_transfer.configFlags |= kSPI_FrameAssert;

  // NOTE: g_eeg_spi_transfer must have already been init'd.
  // We can do that in task_init() because the values never change.
//  taskENTER_CRITICAL(); // Calling CRITICAL section in ISR is okay on Cortex M processors
  portDISABLE_INTERRUPTS();
  status_t status = SPI_MasterTransferDMA(SPI_EEG_BASE, &SPI_EEG_DMA_HANDLE, &g_eeg_spi_transfer);
  portENABLE_INTERRUPTS();
//  taskEXIT_CRITICAL(); // Calling CRITICAL section in ISR is okay on Cortex M processors
  if(status == kStatus_InvalidArgument){
//    LOGV(TAG,"Failed to initiate eeg dma: kStatus_InvalidArgument.");
  }else if(status ==  kStatus_SPI_Busy){
//    LOGV(TAG,"Failed to initiate eeg dma: kStatus_SPI_Busy.");
  }
}


void eeg_dma_rx_complete_isr(SPI_Type *base, spi_dma_handle_t *handle, status_t status, void *userData)
{
  TRACEALYZER_ISR_EEG_BEGIN( EEG_DRDY_PINT_ISR_TRACE );
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  arrange_and_send_eeg_channels_from_isr(&xHigherPriorityTaskWoken);

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  TRACEALYZER_ISR_EEG_END( xHigherPriorityTaskWoken );
}


#else // (defined(USE_EEG_INTERRUPT_INITIATED_DMA) && (USE_EEG_INTERRUPT_INITIATED_DMA > 0U))

// Immediately kick off a DMA read of the SPI bytes:
void eeg_drdy_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status)
{
    TRACEALYZER_ISR_EEG_BEGIN( EEG_DRDY_PINT_ISR_TRACE );
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    eeg_reader_event_t event = {.type = EEG_READER_EVENT_DRDY, {0}};
    xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    TRACEALYZER_ISR_EEG_END( xHigherPriorityTaskWoken );
}

static void handle_eeg_drdy_pint_isr(){
  ads129x *ads = &(g_eeg_reader_context.ads);

//  g_timestamp_union.timestamp = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
//  g_spi_bytes[0] = g_timestamp_union.timestamp_bytes[0];
//  g_spi_bytes[1] = g_timestamp_union.timestamp_bytes[1];
//  g_spi_bytes[2] = g_timestamp_union.timestamp_bytes[2];
//  g_spi_bytes[3] = g_timestamp_union.timestamp_bytes[3];
  g_spi_bytes[4] = g_sample_number_union.sample_number_bytes[0];
  g_spi_bytes[5] = g_sample_number_union.sample_number_bytes[1];
  g_spi_bytes[6] = g_sample_number_union.sample_number_bytes[2];
  g_spi_bytes[7] = g_sample_number_union.sample_number_bytes[3];
  g_sample_number_union.sample_number++;

  // Prepare the DMA SPI Transfer
  g_eeg_spi_transfer.txData   = NULL;
  g_eeg_spi_transfer.dataSize = ads->num_spi_bytes;
  g_eeg_spi_transfer.rxData   = g_spi_bytes + TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES;
  g_eeg_spi_transfer.configFlags |= kSPI_FrameAssert;

  // NOTE: g_eeg_spi_transfer must have already been init'd.
  // We can do that in task_init() because the values never change.
  status_t status = SPI_MasterTransferNonBlocking(SPI_EEG_BASE, &g_spi_handle, &g_eeg_spi_transfer);
  if(status == kStatus_InvalidArgument){
    LOGV(TAG,"Failed to initiate eeg dma: kStatus_InvalidArgument.");
  }else if(status ==  kStatus_SPI_Busy){
    LOGV(TAG,"Failed to initiate eeg dma: kStatus_SPI_Busy.");
  }
}

static void eeg_spi_rx_complete_isr(SPI_Type *base, spi_master_handle_t *handle, status_t status, void *userData){
  TRACEALYZER_ISR_EEG_BEGIN(EEG_DMA_RX_COMPLETE_ISR_TRACE);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if(status == kStatus_SPI_Idle){
    arrange_and_send_eeg_channels_from_isr(&xHigherPriorityTaskWoken);
  }else{
    debug_uart_puts((char*)"missed eeg_spi_rx_complete_isr");
  }

  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
  TRACEALYZER_ISR_EEG_END( xHigherPriorityTaskWoken );
}

// We still need to provide a stub for this function, as it is referenced in the MCUXpresso config tools.
void eeg_dma_rx_complete_isr(SPI_Type *base, spi_dma_handle_t *handle, status_t status, void *userData){}


#endif // (defined(USE_EEG_INTERRUPT_INITIATED_DMA) && (USE_EEG_INTERRUPT_INITIATED_DMA > 0U))

static void arrange_and_send_eeg_channels_from_isr(BaseType_t *pxHigherPriorityTaskWoken){
  BaseType_t xHigherPriorityTaskWoken1 = pdFALSE;
  BaseType_t xHigherPriorityTaskWoken2 = pdFALSE;

  // decode
  uint8_t* buffer = eeg_processor_send_eeg_data_open_from_isr(EEG_MSG_LEN, &xHigherPriorityTaskWoken1);
  if(buffer != NULL){
    size_t src_offset = 0;
    size_t dst_offset = 0;
    // skip timestamp
    src_offset += 4;
    // copy off sample number
    memcpy(buffer+dst_offset,g_spi_bytes+src_offset,SAMPLE_NUMBER_SIZE_IN_BYTES);
    src_offset += 4;
    dst_offset += 4;
    // skip status
    src_offset += 3;
#if defined(VARIANT_FF2)
    // copy off FP1
    memcpy(buffer+dst_offset,g_spi_bytes+src_offset+EEG_CH2*3,3); // 2
    dst_offset += 3;
    // copy off FPZ
    memcpy(buffer+dst_offset,g_spi_bytes+src_offset+EEG_CH4*3,3); // 4
    dst_offset += 3;
    // copy off FP2
    memcpy(buffer+dst_offset,g_spi_bytes+src_offset+EEG_CH1*3,3); // 1
    dst_offset += 3;
#elif defined(VARIANT_FF3)
    // copy off FP1
    memcpy(buffer+dst_offset,g_spi_bytes+src_offset+EEG_CH2*3,3);
    dst_offset += 3;
    // copy off FPZ
    memcpy(buffer+dst_offset,g_spi_bytes+src_offset+EEG_CH1*3,3);
    dst_offset += 3;
    // copy off FP2
    memcpy(buffer+dst_offset,g_spi_bytes+src_offset+EEG_CH3*3,3);
    dst_offset += 3;
#endif

    eeg_processor_send_eeg_data_close_from_isr( buffer , EEG_MSG_LEN , &xHigherPriorityTaskWoken2 );
  }

#ifdef VARIANT_FF3
  static uint32_t temp_sample_rate = 0;
  
  if (temp_sample_rate++ > TEMP_SENSOR_SAMPLE_RATE) {
    // Save the skin temp sensor data.
    int32_t temp;
    uint8_t* temp_bytes = &g_spi_bytes[4 /* timestamp */ + 4 /* sample number */ + 3 /* status */ + TEMP_SENSOR_CHANNEL*3];
    temp_sample_rate = 0;

    // Convert 24bit 2's complement to 32 bit 2's complement.
    temp = (temp_bytes[0] << 24
	    | temp_bytes[1] << 16
	    | temp_bytes[2] << 8) >> 8;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    eeg_reader_event_t event = {.type = EEG_READER_EVENT_TEMP_SAMPLE, 0};
    event.data.temp = temp;
    xQueueSendFromISR(g_event_queue, &event, &xHigherPriorityTaskWoken);
  }
#endif

  *pxHigherPriorityTaskWoken = xHigherPriorityTaskWoken1 || xHigherPriorityTaskWoken2;
}

void handle_eeg_temp_sample(int32_t temp) {
  data_log_temp(temp);
}

#else

void eeg_reader_pretask_init(void){}
void eeg_reader_task(void *ignored){}
void eeg_reader_event_get_info(void){}
void eeg_reader_event_power_on(void){}
void eeg_reader_event_power_off(void){}
void eeg_reader_event_start(void){}
void eeg_reader_event_stop(void){}
void eeg_reader_event_set_gain(uint8_t ads_gain){}

void eeg_drdy_pint_isr(pint_pin_int_t pintr, uint32_t pmatch_status){}
void eeg_dma_rx_complete_isr(SPI_Type *base, spi_dma_handle_t *handle, status_t status, void *userData){}

#endif



