/*
 * max86140.c
 *
 *  Created on: Jun 9, 2023
 *      Author: tyler
 */
#include "max86140.h"

static const char *TAG = "hrm";  // Logging prefix for this module


static void rreg(uint8_t reg, uint8_t* val)
{
	 /* Setup TX/RX buffers */
	  uint8_t tx_cmd[3] = {0};  // 3 bytes
	  tx_cmd[0] = reg; 			// register address to read
	  tx_cmd[1] = 0x80;         // Read command
	  tx_cmd[2] = 0;            // transmit 1 byte to receive 1 byte.
	  uint8_t rx_cmd[3] = {0};  // expect only 1 meaningful rx byte

	  /* Setup master transfer */
	  status_t status;
	  spi_transfer_t masterXfer = {0};
	  masterXfer.txData   = tx_cmd;
	  masterXfer.dataSize = 3;
	  masterXfer.rxData   = rx_cmd;
	  masterXfer.configFlags |= kSPI_FrameAssert;

	  /* Start master transfer */
	  status = SPI_MasterTransferBlocking(FC2_HRM_SPI_PERIPHERAL, &masterXfer);
	  if (status != kStatus_Success)
	  {
	    LOGV(TAG, "SPI master: error during transfer.");
	    return;
	  }

	  *val = rx_cmd[2];
	  return;
}

static void wreg(uint8_t reg, uint8_t val)
{
	/* Setup TX/RX buffers */
	uint8_t tx_cmd[3] = {0}; // 3 bytes
	tx_cmd[0] = reg; // register address to written
	tx_cmd[1] = 0x00;          // Write command?
	tx_cmd[2] = val;          // transmit 1 byte to receive 1 byte.
	uint8_t rx_cmd[3] = {0}; // dummy buffer

	/* Setup master transfer */
	status_t status;
	spi_transfer_t masterXfer = {0};
	masterXfer.txData   = tx_cmd;
	masterXfer.dataSize = 3;
	masterXfer.rxData   = rx_cmd;
	masterXfer.configFlags |= kSPI_FrameAssert;

	/* Start master transfer */
	status = SPI_MasterTransferBlocking(FC2_HRM_SPI_PERIPHERAL, &masterXfer);
	if (status != kStatus_Success)
	{
	LOGV(TAG, "SPI master: error during transfer.");
	return;
	}
	else
	{
		printf("spi transaction worked\r\n"); // ToDo remove for final merge in
	}

	return;

}

void max86140_init(void)
{

}

void max86140_test(void)
{
	uint8_t val;

	rreg(MAX86140_REG_INT_STATUS_1,&val);
	LOGV(TAG, "MAX86140_REG_INT_STATUS_1 = 0x%2X ",val);                 
	rreg(MAX86140_REG_INT_STATUS_2,&val);
	LOGV(TAG, "MAX86140_REG_INT_STATUS_2 = 0x%2X ",val);                 
	rreg(MAX86140_REG_INT_ENABLE_1,&val);
	LOGV(TAG, "MAX86140_REG_INT_ENABLE_1 = 0x%2X ",val);                 
	rreg(MAX86140_REG_INT_ENABLE_2,&val);
	LOGV(TAG, "MAX86140_REG_INT_ENABLE_2 = 0x%2X ",val);                 
	rreg(MAX86140_REG_FIFO_WRITE_POINTER,&val);
	LOGV(TAG, "MAX86140_REG_FIFO_WRITE_POINTER = 0x%2X ",val);           
	rreg(MAX86140_REG_FIFO_READ_POINTER,&val);
	LOGV(TAG, "MAX86140_REG_FIFO_READ_POINTER = 0x%2X ",val);            
	rreg(MAX86140_REG_OVERFLOW_COUNTER,&val);
	LOGV(TAG, "MAX86140_REG_OVERFLOW_COUNTER = 0x%2X ",val);             
	rreg(MAX86140_REG_FIFO_DATA_COUNTER,&val);
	LOGV(TAG, "MAX86140_REG_FIFO_DATA_COUNTER = 0x%2X ",val);            
	rreg(MAX86140_REG_FIFO_DATA_REGISTER,&val);
	LOGV(TAG, "MAX86140_REG_FIFO_DATA_REGISTER = 0x%2X ",val);           
	rreg(MAX86140_REG_FIFO_CONFIGURATION_1,&val);
	LOGV(TAG, "MAX86140_REG_FIFO_CONFIGURATION_1 = %2X",val);         
	rreg(MAX86140_REG_FIFO_CONFIGURATION_2,&val);
	LOGV(TAG, "MAX86140_REG_FIFO_CONFIGURATION_2 = %2X",val);         
	rreg(MAX86140_REG_SYSTEM_CONTROL,&val);
	LOGV(TAG, "MAX86140_REG_SYSTEM_CONTROL = %2X",val);               
	rreg(MAX86140_REG_PPG_SYNC_CONTROL,&val);
	LOGV(TAG, "MAX86140_REG_PPG_SYNC_CONTROL = %2X",val);             
	rreg(MAX86140_REG_PPG_CONFIGURATION_1,&val);
	LOGV(TAG, "MAX86140_REG_PPG_CONFIGURATION_1 = %2X",val);          
	rreg(MAX86140_REG_PPG_CONFIGURATION_2,&val);
	LOGV(TAG, "MAX86140_REG_PPG_CONFIGURATION_2 = %2X",val);          
	rreg(MAX86140_REG_PPG_CONFIGURATION_3,&val);
	LOGV(TAG, "MAX86140_REG_PPG_CONFIGURATION_3 = %2X",val);          
	rreg(MAX86140_REG_PROX_INTERRUPT_THRESHOLD,&val);
	LOGV(TAG, "MAX86140_REG_PROX_INTERRUPT_THRESHOLD = %2X",val);     
	rreg(MAX86140_REG_PHOTO_DIODE_BIAS,&val);
	LOGV(TAG, "MAX86140_REG_PHOTO_DIODE_BIAS = %2X",val);             
	rreg(MAX86140_REG_PICKET_FENCE,&val);
	LOGV(TAG, "MAX86140_REG_PICKET_FENCE = %2X",val);                 
	rreg(MAX86140_REG_LED_SEQUENCE_REGISTER_1,&val);
	LOGV(TAG, "MAX86140_REG_LED_SEQUENCE_REGISTER_1 = %2X",val);      
	rreg(MAX86140_REG_LED_SEQUENCE_REGISTER_2,&val);
	LOGV(TAG, "MAX86140_REG_LED_SEQUENCE_REGISTER_2 = %2X",val);      
	rreg(MAX86140_REG_LED_SEQUENCE_REGISTER_3,&val);
	LOGV(TAG, "MAX86140_REG_LED_SEQUENCE_REGISTER_3 = %2X",val);      
	rreg(MAX86140_REG_LED1_PA,&val);
	LOGV(TAG, "MAX86140_REG_LED1_PA = %2X",val);                      
	rreg(MAX86140_REG_LED2_PA,&val);
	LOGV(TAG, "MAX86140_REG_LED2_PA = %2X",val);                      
	rreg(MAX86140_REG_LED3_PA,&val);
	LOGV(TAG, "MAX86140_REG_LED3_PA = %2X",val);                      
	rreg(MAX86140_REG_LED4_PA,&val);
	LOGV(TAG, "MAX86140_REG_LED4_PA = %2X",val);                      
	rreg(MAX86140_REG_LED5_PA,&val);
	LOGV(TAG, "MAX86140_REG_LED5_PA = %2X",val);                      
	rreg(MAX86140_REG_LED6_PA,&val);
	LOGV(TAG, "MAX86140_REG_LED6_PA = %2X",val);                      
	rreg(MAX86140_REG_LED_PILOT_PA,&val);
	LOGV(TAG, "MAX86140_REG_LED_PILOT_PA = %2X",val);                 
	rreg(MAX86140_REG_LED_RANGE_1,&val);
	LOGV(TAG, "MAX86140_REG_LED_RANGE_1 = %2X",val);                  
	rreg(MAX86140_REG_LED_RANGE_2,&val);
	LOGV(TAG, "MAX86140_REG_LED_RANGE_2 = %2X",val);                  
	rreg(MAX86140_REG_S1_HI_RES_DAC1,&val);
	LOGV(TAG, "MAX86140_REG_S1_HI_RES_DAC1 = %2X",val);               
	rreg(MAX86140_REG_S2_HI_RES_DAC1,&val);
	LOGV(TAG, "MAX86140_REG_S2_HI_RES_DAC1 = %2X",val);               
	rreg(MAX86140_REG_S3_HI_RES_DAC1,&val);
	LOGV(TAG, "MAX86140_REG_S3_HI_RES_DAC1 = %2X",val);               
	rreg(MAX86140_REG_S4_HI_RES_DAC1,&val);
	LOGV(TAG, "MAX86140_REG_S4_HI_RES_DAC1 = %2X",val);               
	rreg(MAX86140_REG_S5_HI_RES_DAC1,&val);
	LOGV(TAG, "MAX86140_REG_S5_HI_RES_DAC1 = %2X",val);               
	rreg(MAX86140_REG_S6_HI_RES_DAC1,&val);
	LOGV(TAG, "MAX86140_REG_S6_HI_RES_DAC1 = %2X",val);               
	rreg(MAX86140_REG_S1_HI_RES_DAC2,&val);
	LOGV(TAG, "MAX86140_REG_S1_HI_RES_DAC2 = %2X",val);               
	rreg(MAX86140_REG_S2_HI_RES_DAC2,&val);
	LOGV(TAG, "MAX86140_REG_S2_HI_RES_DAC2 = %2X",val);               
	rreg(MAX86140_REG_S3_HI_RES_DAC2,&val);
	LOGV(TAG, "MAX86140_REG_S3_HI_RES_DAC2 = %2X",val);               
	rreg(MAX86140_REG_S4_HI_RES_DAC2,&val);
	LOGV(TAG, "MAX86140_REG_S4_HI_RES_DAC2 = %2X",val);               
	rreg(MAX86140_REG_S5_HI_RES_DAC2,&val);
	LOGV(TAG, "MAX86140_REG_S5_HI_RES_DAC2 = %2X",val);               
	rreg(MAX86140_REG_S6_HI_RES_DAC2,&val);
	LOGV(TAG, "MAX86140_REG_S6_HI_RES_DAC2 = %2X",val);               
	rreg(MAX86140_REG_DIE_TEMPERATURE_CONFIGURATION,&val);
	LOGV(TAG, "MAX86140_REG_DIE_TEMPERATURE_CONFIGURATION = %2X",val);
	rreg(MAX86140_REG_DIE_TEMPERATURE_INTEGER,&val);
	LOGV(TAG, "MAX86140_REG_DIE_TEMPERATURE_INTEGER = %2X",val);      
	rreg(MAX86140_REG_DIE_TEMPERATURE_FRACTION,&val);
	LOGV(TAG, "MAX86140_REG_DIE_TEMPERATURE_FRACTION = %2X",val);     
	rreg(MAX86140_REG_SHA_COMMAND,&val);
	LOGV(TAG, "MAX86140_REG_SHA_COMMAND = %2X",val);                  
	rreg(MAX86140_REG_SHA_CONFIGURATION,&val);
	LOGV(TAG, "MAX86140_REG_SHA_CONFIGURATION = %2X",val);            
	rreg(MAX86140_REG_MEMORY_CONTROL,&val);
	LOGV(TAG, "MAX86140_REG_MEMORY_CONTROL = %2X",val);               
	rreg(MAX86140_REG_MEMORY_INDEX,&val);
	LOGV(TAG, "MAX86140_REG_MEMORY_INDEX = %2X",val);                 
	rreg(MAX86140_REG_MEMORY_DATA,&val);
	LOGV(TAG, "MAX86140_REG_MEMORY_DATA = %2X",val);                  
	rreg(MAX86140_REG_PART_ID,&val);
	LOGV(TAG, "MAX86140_REG_PART_ID = %2X",val);                      
}

void max86140_test_read(uint8_t regAdd)
{
	uint8_t val;
	rreg(regAdd, &val);

	LOGV(TAG, "Reg %2X = %2X", regAdd, val);
}

void max86140_test_write(uint8_t regAdd, uint8_t val)
{
	wreg(regAdd, val);


}
