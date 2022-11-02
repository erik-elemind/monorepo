/*
 * nand_platform_rt685_flexspi.c
 *
 *  Created on: Oct 4, 2022
 *      Author: Tyler Gage
 */

#include "nand.h"
#include "nand_platform.h"
#include "util_delay.h"

// HAL
#include "fsl_flexspi.h"
#include "fsl_flexspi_dma.h"

// FreeRTOS
#include "FreeRTOS.h"

// Create a reference struct to use for the chipnfo.
// This is externed via nand.h, but could also be returned by a getter().
const nand_chipinfo_t nand_chipinfo = {
  .block_addr_offset = NAND_BLOCK_ADDR_OFFSET,
  .layout_size_bytes = NAND_PAGE_PLUS_SPARE_SIZE
};

// TODO PGA: characterize and re-enable this
// Threshold, in bytes, at which DMA is utilized for SPI transactions
#define NAND_DMA_MIN_XFER_SIZE_BYTES 0

/*
 * Enable semaphore to free up processor time while the DMA is transferring data.
 * - There is a small time penalty (~5%) for using the semaphore vs not (polling).
 * - But, when doing continuous reads, 30% of the processor cycles can be saved.
 */
#ifndef NAND_SPI_USE_SEMAPHORE
#define NAND_SPI_USE_SEMAPHORE 1
#endif

/// Logging prefix
//static const char* TAG = "nand_platform_lpc_spi";

typedef struct nand_platform_handle {
  SPI_Type* base;
  spi_dma_handle_t* spi_handle;
  SemaphoreHandle_t mutex;
  SemaphoreHandle_t event;
} nand_platform_handle_t;

static nand_platform_handle_t g_nand_handle = {
  .base = SPI_FLASH_BASE,
  .spi_handle = &NAND_FLEXSPI_DMA_Handle
};

#if !(defined(NAND_SPI_USE_SEMAPHORE) && (NAND_SPI_USE_SEMAPHORE > 0U))
  static volatile bool g_spi_dma_rx_is_complete = false;
#endif
static volatile status_t g_spi_transfer_status;

static status_t
SPI_MasterTransferDMA_Blocking(nand_platform_handle_t *handle, const spi_transfer_t *xfer)
{
  // If we are called before the scheduler is started (in main()), just use
  // the busy-looping vendor HAL function. This is needed because FreeRTOS
  // semaphore functions don't work before the scheduler is started.
  if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
    return SPI_MasterTransferBlocking(handle->base, (spi_transfer_t*) xfer);
  }

  if (xSemaphoreTake(handle->mutex, portMAX_DELAY) != pdTRUE)
  {
      return kStatus_SPI_Busy;
  }

  uint8_t *remain_txData = xfer->txData;
  uint8_t *remain_rxData = xfer->rxData;
  size_t remain_dataSize = xfer->dataSize;
  size_t data_size = 0;

  while(remain_dataSize > 0){

    // Ensure the transfer size is always less than DMA allowed max.
    data_size = (remain_dataSize > DMA_MAX_TRANSFER_COUNT) ? DMA_MAX_TRANSFER_COUNT : remain_dataSize;
    // Setup new SPI xfer object
    spi_transfer_t xfer_split;
    xfer_split.dataSize = data_size;
    xfer_split.txData = remain_txData;
    xfer_split.rxData = remain_rxData;
    xfer_split.configFlags = xfer->configFlags;
    if((xfer->configFlags & kSPI_FrameAssert) != 0){
      if(remain_dataSize-data_size == 0){
        // if this is the last transfer, use frame assert.
        xfer_split.configFlags = xfer->configFlags;
      }else{
        xfer_split.configFlags = xfer->configFlags & ~kSPI_FrameAssert;
      }
    }

    // Do the SPI DMA Transfer
#if !(defined(NAND_SPI_USE_SEMAPHORE) && (NAND_SPI_USE_SEMAPHORE > 0U))
    g_spi_dma_rx_is_complete = false;
#endif

    status_t status = SPI_MasterTransferDMA(handle->base, handle->spi_handle, &xfer_split);
    if (kStatus_Success != status)
    {
      xSemaphoreGive(handle->mutex);
      return status;
    }

#if (defined(NAND_SPI_USE_SEMAPHORE) && (NAND_SPI_USE_SEMAPHORE > 0U))
    /* Wait for transfer to finish */
    if (xSemaphoreTake(handle->event, portMAX_DELAY) != pdTRUE)
    {
      xSemaphoreGive(handle->mutex);
      return kStatus_SPI_Error;
    }
#else
      // Wait for the SPI DMA transfer to complete
      while(!g_spi_dma_rx_is_complete){
        // wait
      }
#endif

    if(kStatus_Success != g_spi_transfer_status){
      xSemaphoreGive(handle->mutex);
      return g_spi_transfer_status;
    }

    // Transfer is complete, setup for the next transfer
    remain_dataSize -= data_size;
    remain_txData = (xfer->txData == NULL) ? NULL : remain_txData+data_size;
    remain_rxData = (xfer->rxData == NULL) ? NULL : remain_rxData+data_size;
  }

  xSemaphoreGive(handle->mutex);
  return kStatus_Success;
}

#if 0
// This routine is declared in peripherals.h
void nand_spi_isr(FLEXSPI_Type *base, flexspi_dma_handle_t *handle, status_t status, void *userData){
  TRACEALYZER_ISR_FLASH_BEGIN( FLASH_SPI_ISR_TRACE );

  // Capture most recent transfer status for use outside this ISR
  g_spi_transfer_status = status;

#if (defined(NAND_SPI_USE_SEMAPHORE) && (NAND_SPI_USE_SEMAPHORE > 0U))
  BaseType_t reschedule;
  xSemaphoreGiveFromISR(g_nand_handle.event, &reschedule);
  // This yield is important to prevent breaks in the eeg sampling and audio playback.
  portYIELD_FROM_ISR(reschedule);
  TRACEALYZER_ISR_FLASH_END( reschedule );
#else
  g_spi_dma_rx_is_complete = true;
  TRACEALYZER_ISR_FLASH_END(0);
#endif
}

#else


volatile bool g_flexspi_done = false;

void nand_flexspi_isr(FLEXSPI_Type *base, flexspi_dma_handle_t *handle, status_t status, void *userData){
	g_flexspi_done = true;
}


status_t
FLEXSPI_MasterTransferDMA_Blocking(nand_platform_handle_t *handle, const flexspi_transfer_t *xfer)
{
	g_flexspi_done = false;
	status_t status;

//	FLEXSPI_TransferUpdateSizeDMA(NAND_FLEXSPI_PERIPHERAL, &NAND_FLEXSPI_DMA_Handle, kFLEXPSI_DMAnSize1Bytes);

	do{
		status = FLEXSPI_TransferDMA(NAND_FLEXSPI_PERIPHERAL, &NAND_FLEXSPI_DMA_Handle, xfer);
		printf("DMAstatus = %d\r\n", status);
	}while(status != kStatus_Success);

	while(!g_flexspi_done){

	}
	g_flexspi_done = false;


	printf("DMAstatus = %d\r\n", status);
//	printf("\n\rTransfer Done\n\r");

	return status;
}

status_t
FLEXSPI_MasterTransferDMA_Blocking2(const flexspi_transfer_t *xfer)
{
	return FLEXSPI_MasterTransferDMA_Blocking(&g_nand_handle, xfer);
}


#endif



#if 0
void nand_spi_isr(SPI_Type *base, spi_dma_handle_t *handle, status_t status, void *userData){
  TRACEALYZER_ISR_FLASH_BEGIN( FLASH_SPI_ISR_TRACE );

  // Capture most recent transfer status for use outside this ISR
  g_spi_transfer_status = status;

#if (defined(NAND_SPI_USE_SEMAPHORE) && (NAND_SPI_USE_SEMAPHORE > 0U))
  BaseType_t reschedule;
  xSemaphoreGiveFromISR(g_nand_handle.event, &reschedule);
  // This yield is important to prevent breaks in the eeg sampling and audio playback.
  portYIELD_FROM_ISR(reschedule);
  TRACEALYZER_ISR_FLASH_END( reschedule );
#else
  g_spi_dma_rx_is_complete = true;
  TRACEALYZER_ISR_FLASH_END(0);
#endif
}
#endif

static status_t spi_flash_transfer(nand_platform_handle_t *handle, spi_transfer_t *xfer){
  // TODO: Use the FreeRTOS driver here; need to modify handle?
  if (xfer->dataSize < NAND_DMA_MIN_XFER_SIZE_BYTES) {
    // Use blocking non-DMA operation
    return SPI_MasterTransferBlocking(handle->base, xfer);
  }
  // Use blocking DMA
  return SPI_MasterTransferDMA_Blocking(handle, xfer);
}


// Delay for delay_ms (delay_ms may be zero for a simple thread yield).
void nand_platform_yield_delay(int delay_ms) {
	util_delay_ms(delay_ms);
}


// Return 0 if command and response completed succesfully, or <0 error code
int nand_platform_command_response(
  uint8_t* p_command,
  uint8_t command_len,
  uint32_t* p_data,
  uint32_t data_len
  )
{
	status_t status;

//////////////////////////////////////////////////////// FF3 version
//  	 printf("\r\n");
//  spi_transfer_t spi_transfer = {0};
//
//  /* Start transfer for command TX (don't de-assert CS after). */
//  spi_transfer.txData   = (uint8_t*)p_command;
//  spi_transfer.dataSize = command_len;
//  spi_transfer.rxData   = NULL;
//  if ((p_data == NULL) || (data_len == 0)) {
//      spi_transfer.configFlags |= kSPI_FrameAssert;
//  }
//  else {
//    spi_transfer.configFlags = 0;
//  }
//
//  status = spi_flash_transfer(&g_nand_handle, &spi_transfer);
//
//  if (status == kStatus_Success) {
//    if ((p_data != NULL) && (data_len > 0)) {
//      /* Start transfer for data RX (de-assert CS after to indicate end of
//         transaction).  */
//      spi_transfer.txData   = NULL;
//      spi_transfer.dataSize = data_len;
//      spi_transfer.rxData   = (uint8_t*)p_data;
//      spi_transfer.configFlags |= kSPI_FrameAssert;
//
//      status = spi_flash_transfer(&g_nand_handle, &spi_transfer);
//    }
//  }
  return status;
}

int nand_platform_command_with_data(
  uint8_t* p_command,
  uint8_t command_len,
  uint8_t* p_data,
  uint16_t data_len
  )
{
  status_t status;


//  spi_transfer_t spi_transfer = {0};
//
//  /* Start transfer for command TX (don't de-assert CS after). */
//  spi_transfer.txData   = (uint8_t*)p_command;
//  spi_transfer.dataSize = command_len;
//  spi_transfer.rxData   = NULL;
//  if ((p_data == NULL) || (data_len == 0)) {
//      spi_transfer.configFlags |= kSPI_FrameAssert;
//  }
//  else {
//    spi_transfer.configFlags = 0;
//  }
//
//  status = spi_flash_transfer(&g_nand_handle, &spi_transfer);
//
//  if (status == kStatus_Success) {
//    if ((p_data != NULL) && (data_len > 0)) {
//      /* Start transfer for data TX (de-assert CS after to indicate end of
//         transaction).  */
//      spi_transfer.txData   = (uint8_t*)p_data;
//      spi_transfer.dataSize = data_len;
//      spi_transfer.rxData   = NULL;
//      spi_transfer.configFlags |= kSPI_FrameAssert;
//
//      status = spi_flash_transfer(&g_nand_handle, &spi_transfer);
//    }
//  }


  return status;
}

int nand_platform_init(void) {
  // Initialize RTOS related functions
  g_nand_handle.mutex = xSemaphoreCreateMutex();
  g_nand_handle.event = xSemaphoreCreateBinary();

  if (NULL == g_nand_handle.mutex ||
      NULL == g_nand_handle.event) {
    return -1;
  }

  vQueueAddToRegistry(g_nand_handle.mutex, "nand_spi_mutex_sem");
  vQueueAddToRegistry(g_nand_handle.event, "nand_spi_event_sem");
  return 0;
}
