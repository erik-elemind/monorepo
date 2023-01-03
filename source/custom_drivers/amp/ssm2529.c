/*
 * Modified:     Jan, 2023
 * Modified by:  Tyler Gage
 */

#include "ssm2529.h"

static i2c_rtos_handle_t* g_i2c_handle;
/*****************************************************************************
* @brief Write to an internal SSM2529 Register.
*
* @param regAddr - register address.
* @param data - data to be written.
*
* @return None.
******************************************************************************/
void SSM2529_WriteReg(char regAddr, unsigned char data)
{
  i2c_master_transfer_t masterXfer;
  status_t status;

  memset(&masterXfer, 0, sizeof(masterXfer));
  masterXfer.slaveAddress   = SSM2529_I2C_ADDR;
  masterXfer.direction      = kI2C_Write;
  masterXfer.subaddress     = regAddr;
  masterXfer.subaddressSize = 1;
  masterXfer.data           = &data;
  masterXfer.dataSize       = 1;
  masterXfer.flags          = kI2C_TransferDefaultFlag;

  status = I2C_RTOS_Transfer(g_i2c_handle, &masterXfer);
  if (status != kStatus_Success) {
    // TODO: Print Error Message
  }
}

/*****************************************************************************
* @brief Read from an internal SSM2529 Register.
*
* @param regAddr - register address.
*
* @return rxBuffer[0] - data read from device.
******************************************************************************/
unsigned char SSM2529_ReadReg(char regAddr)
{
  unsigned char data;

  i2c_master_transfer_t masterXfer;
  status_t status;

  masterXfer.slaveAddress   = SSM2529_I2C_ADDR;
  masterXfer.direction      = kI2C_Read;
  masterXfer.subaddress     = regAddr;
  masterXfer.subaddressSize = 1;
  masterXfer.data           = &data;
  masterXfer.dataSize       = 1;
  masterXfer.flags          = kI2C_TransferDefaultFlag;

  status = I2C_RTOS_Transfer(g_i2c_handle, &masterXfer);
  if (status != kStatus_Success) {
    // TODO: Print Error Message
  }

  return data;
}

/*****************************************************************************
* @brief Initialize SSM2529
*
* @param None.
*
* @return None.
******************************************************************************/
void SSM2529_Init(i2c_rtos_handle_t *i2c_handle)
{
  // Initializing SSM2529 via I2C.

  // Software Reset and Master Software Power Down Control Register (RegAddr 0):
  // 00000000
  // 0******* - Software reset: Normal operation (do not reset)
  // *0****** - Auto power-down mode: Only digital
  // **0***** - Auto power-down enable: Auto power-down disabled
  // ***0000* - Master clock rate selection: ToDo: Figure this out
  // *******0 - Software master p0ower-down: Normal operation (do not power-down)
  SSM2529_WriteReg(SSM2529_PWR_CTRL, 0x00);

  // Edge Speed and Clocking Control Register (RegAddr 1):
  // 00000001
  // 0******* - PDM input enable: Disable
  // *0****** - PDM input sample rate: 3 MHz
  // **0***** - ADC power down: Power down
  // ***0**** - BCLK cycles per channel frame: 32 cycles per channel
  // ****0*** - Generate BCLK internally: Disabled
  // *****00* - Edge rate control: Normal Operation
  // *******1 - Auto sample rate: Automatic sample rate detection
  SSM2529_WriteReg(SSM2529_SYS_CTRL, 0x01);

  // Serial Audio Interface and Sample Rate Control Register (RegAddr 2):
  // 00000000
  // 00****** - Serial data format: I2S BCLK delay by 1
  // **000*** - Serial audio interface format: Stereo I2S, left justified, right justified
  // *****000 - Sample rate selection: ToDo: Evaluate once MCS from Reg 0 is figured out
  SSM2529_WriteReg(SSM2529_SAI_FMT1, 0x00);

  // Serial Audio Interface Control Register (RegAddr 3):
  // 00000000
  // 0******* - Small power stage enable: Disabled
  // *00***** - L/R channel selector: Select Left Channel
  // ***0**** - LRCLK mode selection for TDM operation: 50% duty cycle LRCLK
  // ****0*** - LRCLK polarity control: Normal LRCLK operation
  // *****0** - SDATA bit stream order: MSB first
  // ******0* - BCLK cycles per frame in TDM modes select: 32 BCLK cycles per slot
  // *******0 - BCLK active edge select: Rising BCLK edge
  SSM2529_WriteReg(SSM2529_SAI_FMT2, 0x00);

  // Channel Mapping Control Register (RegAddr 4):
  // 00010000
  // 0001**** - Right channel mapping select: Channel 1 from SAI to right output
  // ****0000 - Left channel mapping select: Channel 0 from SAI to left output
  SSM2529_WriteReg(SSM2529_Channel_mapping_control, 0x10);

  // Volume Control Before FDSP (RegAddr 5):
  // 01000000
  // 01000000 - Volume control before FDSP: 0dB
  SSM2529_WriteReg(SSM2529_VOL_BF_FDSP, 0x40);

  // Volume Control After FDSP (RegAddr 6):
  // 01000000
  // 01000000 - Volume control after FDSP: 0dB
  SSM2529_WriteReg(SSM2529_VOL_AF_FDSP, 0x40);

  // Volume and Mute Control Register (RegAddr 7):
  // 00000000
  // 0******* - Clock loss detect enable
  // *000**** - Auto detected sample rate: Read only
  // ****0*** - Reserved
  // *****0** - PDP volume fade enable: Soft
  // ******0* - DIG volume fade enable: Soft
  // *******0 - Analog gain control: 3.6V gain
  SSM2529_WriteReg(SSM2529_Volume_and_mute_control, 0x00);

  // Fault Control 1 Register (RegAddr 15):
  // 00000000
  // 000***** - Reserved
  // ***0**** - Single end lineout enable: Disabled
  // ****0*** - Lineout calibration enable: Disabled
  // *****0** - Clock for DAC and Class-D lost: Read only
  // ******0* - Right channel overcurrent fault: Read only
  // *******0 - Overtemperture fault status: Read only
  SSM2529_WriteReg(SSM2529_FAULT_CTRL1, 0x00);

  // Fault Control 1 Register (RegAddr 16):
  // 01001100
  // 0******* - Reserved
  // *10***** - Auto recovery time: 40 ms
  // ***0**** - Manual fault recovery: Normal operation
  // ****11** - Maximum fault recovery attempts:  Unlimited
  // ******00 - Auto fault recovery control: Auto fault recovery for overtemperature and overcurrent faults
  SSM2529_WriteReg(SSM2529_FAULT_CTRL2, 0x4C);
}

void SSM2529_SetVolume(uint8_t volume)
{
	// ToDo: Verify with real hardware. Do we need both registers?
	SSM2529_WriteReg(SSM2529_VOL_BF_FDSP, volume);
	SSM2529_WriteReg(SSM2529_VOL_AF_FDSP, volume);
}

void SSM2529_Mute(bool mute)
{
	static uint8_t last_volume_bf = 0;
	static uint8_t last_volume_af = 0;

	// ToDo: Verify with real hardware. Do we need both registers?
	if(mute)
	{
		// Save volume in case of un mute, and set volume to 0xFF to mute
		last_volume_bf = SSM2529_ReadReg(SSM2529_VOL_BF_FDSP);
		last_volume_af = SSM2529_ReadReg(SSM2529_VOL_AF_FDSP);

		SSM2529_WriteReg(SSM2529_VOL_BF_FDSP, 0xFF);
		SSM2529_WriteReg(SSM2529_VOL_AF_FDSP, 0xFF);
	}
	else
	{
		// Only update volume if currently muted
		if(SSM2529_ReadReg(SSM2529_VOL_BF_FDSP) == 0xFF)
		{
			SSM2529_WriteReg(SSM2529_VOL_BF_FDSP, last_volume_bf);
		}

		if(SSM2529_ReadReg(SSM2529_VOL_AF_FDSP) == 0xFF)
		{
			SSM2529_WriteReg(SSM2529_VOL_AF_FDSP, last_volume_af);
		}
	}
}

