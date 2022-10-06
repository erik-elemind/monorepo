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

// FreeRTOS
#include "FreeRTOS.h"

/* LUT for the W25N04KW NAND Flash*/


#define NAND_FLEXSPI_LUT_LENGTH 64


#define NAND_CMD_LUT_SEQ_IDX_READID 8
#define NAND_CMD_LUT_SEQ_IDX_READSTATUS 1

const uint32_t NAND_FLEXSPI_LUT[NAND_FLEXSPI_LUT_LENGTH] = {
	/* Read Status */
	[4 * NAND_CMD_LUT_SEQ_IDX_READSTATUS] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x0F, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x08),
	[4 * NAND_CMD_LUT_SEQ_IDX_READSTATUS + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	/* Read ID */
	[4 * NAND_CMD_LUT_SEQ_IDX_READID] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x9F, kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x08),
	[4 * NAND_CMD_LUT_SEQ_IDX_READID + 1] =
	FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x08, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
};

/* FLEXSPI_IRQn interrupt handler */
//void NAND_FLEXSPI_IRQHANDLER(void) {
//  /*  Place your code here */
//  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
//     Store immediate overlapping exception return operation might vector to incorrect interrupt. */
//  #if defined __CORTEX_M && (__CORTEX_M == 4U)
//    __DSB();
//  #endif
//}


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
  .spi_handle = NULL
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

// This routine is declared in peripherals.h
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
void * myuserdatapointer;

static int done = 0;

void mycallback(FLEXSPI_Type *base, flexspi_handle_t *handle, status_t status, void *userData)
{
	printf("callback???\r\n");
	done = 1;
}

uint32_t buff[10] = {0};

// Return 0 if command and response completed succesfully, or <0 error code
int nand_platform_command_response(
  uint8_t* p_command,
  uint8_t command_len,
  uint32_t* p_data,
  uint32_t data_len
  )
{
  status_t status;
  flexspi_transfer_t flashXfer;



		flashXfer.deviceAddress = 0;
		flashXfer.port          = kFLEXSPI_PortA1;
		flashXfer.cmdType       = kFLEXSPI_Read;
		flashXfer.SeqNumber     = 1;
		flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_READID;
		flashXfer.data     = buff;
		flashXfer.dataSize = 4;

		status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

		printf("status = %d\r\n", status);
		printf("Vendor ID: %04X", buff[0]);


		flashXfer.deviceAddress = 0xA0;
		flashXfer.port          = kFLEXSPI_PortA1;
		flashXfer.cmdType       = kFLEXSPI_Read;
		flashXfer.SeqNumber     = 1;
		flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_READSTATUS;
		flashXfer.data     = buff;
		flashXfer.dataSize = 8;

		status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

		printf("status = %d\r\n", status);
		printf("Status Reg 1: %04X\r\n", buff[0]);

		flashXfer.deviceAddress = 0xB0;
		flashXfer.port          = kFLEXSPI_PortA1;
		flashXfer.cmdType       = kFLEXSPI_Read;
		flashXfer.SeqNumber     = 1;
		flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_READSTATUS;
		flashXfer.data     = buff;
		flashXfer.dataSize = 8;

		status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

		printf("status = %d\r\n", status);
		printf("Status Reg 2: %04X\r\n", buff[0]);

		flashXfer.deviceAddress = 0xC0;
		flashXfer.port          = kFLEXSPI_PortA1;
		flashXfer.cmdType       = kFLEXSPI_Read;
		flashXfer.SeqNumber     = 1;
		flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_READSTATUS;
		flashXfer.data     = buff;
		flashXfer.dataSize = 8;

		status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);

		printf("status = %d\r\n", status);
		printf("Status Reg 3: %04X\r\n", buff[0]);

//		// Lets try non blocking?
//
//		flashXfer.deviceAddress = 0;
//		flashXfer.port          = kFLEXSPI_PortA1;
//		flashXfer.cmdType       = kFLEXSPI_Read;
//		flashXfer.SeqNumber     = 1;
//		flashXfer.seqIndex = NAND_CMD_LUT_SEQ_IDX_READID;
//		flashXfer.data     = buff;
//		flashXfer.dataSize = 4;
//
//		status = FLEXSPI_TransferNonBlocking(FLEXSPI, &NAND_FLEXSPI_handle, &flashXfer);
//
//		printf("status = %d\r\n", status);
//
//		size_t count=0;
//
//		while(done == 0){}
//
//		printf("Vendor ID 2 :count = %d,  %04X",count, buff[0]);

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


  // Initialize FlexSPI peripheral configuration
  flexspi_device_config_t deviceconfig = {
  	    .flexspiRootClk       = 6000000,
  		.isSck2Enabled        = false,
  	    .flashSize            = 0x80000,
  	    .CSIntervalUnit       = kFLEXSPI_CsIntervalUnit1SckCycle,
  	    .CSInterval           = 2,
  	    .CSHoldTime           = 3,
  	    .CSSetupTime          = 3,
  	    .dataValidTime        = 2,
  	    .columnspace          = 0,
  	    .enableWordAddress    = 0,
  	    .AWRSeqIndex          = 0,
  	    .AWRSeqNumber         = 0,
  	    .ARDSeqIndex          = 0,
  	    .ARDSeqNumber         = 0,
  	    .AHBWriteWaitUnit     = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
  	    .AHBWriteWaitInterval = 0,
  		.enableWriteMask      = false
  	};
  FLEXSPI_SetFlashConfig(FLEXSPI, &deviceconfig, kFLEXSPI_PortA1);

  // Update LUT
  FLEXSPI_UpdateLUT(FLEXSPI, 0, NAND_FLEXSPI_LUT, 64);

  FLEXSPI_SoftwareReset(FLEXSPI);

  return 0;
}
