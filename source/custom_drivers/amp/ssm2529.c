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
status_t SSM2529_WriteReg(char regAddr, unsigned char data)
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

  return status;
}

/*****************************************************************************
* @brief Read from an internal SSM2529 Register.
*
* @param regAddr - register address.
*
* @return rxBuffer[0] - data read from device.
******************************************************************************/
status_t SSM2529_ReadReg(char regAddr, unsigned char *data)
{
  i2c_master_transfer_t masterXfer;
  status_t status;

  masterXfer.slaveAddress   = SSM2529_I2C_ADDR;
  masterXfer.direction      = kI2C_Read;
  masterXfer.subaddress     = regAddr;
  masterXfer.subaddressSize = 1;
  masterXfer.data           = data;
  masterXfer.dataSize       = 1;
  masterXfer.flags          = kI2C_TransferDefaultFlag;

  status = I2C_RTOS_Transfer(g_i2c_handle, &masterXfer);
  if (status != kStatus_Success) {
    // TODO: Print Error Message
  }

  return status;
}

/*****************************************************************************
* @brief Initialize SSM2529
*
* @param i2c_rtos_handle_t *i2c_handle
*
* @return None.
******************************************************************************/
void SSM2529_Init(i2c_rtos_handle_t *i2c_handle){
	g_i2c_handle = i2c_handle;
}

/*****************************************************************************
* @brief Configure SSM2529
*
* @param None.
*
* @return None.
******************************************************************************/
void SSM2529_Config()
{
  // Initializing SSM2529 via I2C.

  // Software Reset and Master Software Power Down Control Register (RegAddr 0):
  // 01100010
  // 0******* - Software reset: Normal operation (do not reset)
  // *1****** - Auto power-down mode: digital and analog
  // **1***** - Auto power-down enable: enable
  // ***0**** - Low power mode: Normal operation
  // ****001* - Master clock rate selection: (not needed?, enable auto sample rate)
  // *******0 - Software master power-down: Normal operation (do not power-down)
  SSM2529_WriteReg(SSM2529_PWR_CTRL, 0x62);

  // Edge Speed and Clocking Control Register (RegAddr 1):
  // 00110000
  // 0******* - PDM input enable: Disable
  // *0****** - PDM input sample rate: 3 MHz
  // **1***** - ADC power down: Power on
  // ***1**** - BCLK cycles per channel frame: 16 cycles per channel
  // ****0*** - Generate BCLK internally: Disabled
  // *****00* - Edge rate control: Normal Operation
  // *******0 - Auto sample rate: Automatic sample rate detection
  SSM2529_WriteReg(SSM2529_SYS_CTRL, 0x30);

  // Serial Audio Interface and Sample Rate Control Register (RegAddr 2):
  // 00000001
  // 00****** - Serial data format: I2S Left Justified //I2S BCLK delay by 1
  // **000*** - Serial audio interface format: Stereo I2S, left justified, right justified
  // *****001 - Sample rate selection
  SSM2529_WriteReg(SSM2529_SAI_FMT1, 0x01);

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
  // 0******* - Clock loss detect enable (enabling this seems to ruin DPLL lock)
  // *RRR**** - Auto detected sample rate: Read only
  // ****0*** - Reserved
  // *****0** - PDP volume fade enable: Soft
  // ******0* - DIG volume fade enable: Soft
  // *******0 - Analog gain control: 3.6V gain
  SSM2529_WriteReg(SSM2529_Volume_and_mute_control, 0x00);

  /*
   * apll_output_clk = bclk * (2^DPLL_N) * (APLL_R + (APLL_N/APLL_M)) / APLL_X
   * where:
   *   8000000 <= bclk * (2^DPLL_N) <= 27000000
   *   44.1kHz * 1024 <= apll_output_clk <= 48kHz * 1024
   *
   * ADI RECOMMENDED PLL CONFIG SOLUTION:
   * 45,158,400 Hz {APLL_R=4, APLL_N=0, APLL_M=0, APLL_X=1, DPLL_N=4}
   */

  // DPLL_CTRL Register (RegAddr 8)
  // 00010111
  // X******* - Reserved
  // *001**** - DPLL source clock selection: BCLK as DPLL reference clock
  // ****0111 - DPLL output clock frequency: 2^DPLL_N = 16
  SSM2529_WriteReg(SSM2529_DPLL_CTRL, 0x17);

  // APLL_CTRL1 Register (RegAddr 9)
  // 00000000
  // 00000000 - Denominator (M) of the fractional APLL upper byte
  SSM2529_WriteReg(SSM2529_APLL_CTRL1, 0x00);

  // APLL_CTRL2 Register (RegAddr 10)
  // 00000000
  // 00000000 - Denominator (M) of the fractional APLL lower byte
  SSM2529_WriteReg(SSM2529_APLL_CTRL2, 0x00);

  // APLL_CTRL3 Register (RegAddr 11)
  // 00000000
  // 00000000 - Numerator (N) of the fractional APLL upper byte
  SSM2529_WriteReg(SSM2529_APLL_CTRL3, 0x00);

  // APLL_CTRL4 Register (RegAddr 12)
  // 00000000
  // 00000000 - Numerator (N) of the fractional APLL lower byte
  SSM2529_WriteReg(SSM2529_APLL_CTRL4, 0x00);

  // APLL_CTRL5 Register (RegAddr 13)
  // 00100000
  // X0000000 - Reserved
  // *0100*** - Integer part of APLL (R): 4
  // *****00* - APLL input clock divider (X): 1
  // *******0 - APLL operation mode: fractional
  SSM2529_WriteReg(SSM2529_APLL_CTRL5, 0x20);

  // APLL CTRL 6 Register (RegAddr 14):
  // 00000011
  // 00****** - FSYS_DPLL, Analog OSC Clock Rate: ??
  // **0***** - DPLL_BYPASS: Enable DPLL
  // ***0**** - APLL_BYPASS: Enable APLL
  // ****R*** - DPLL_LOCK
  // *****R** - APLL_LOCK
  // ******1* - PLLEN internal PLL enable: disabled
  // *******1 - COREN, core clock enable: enable
  SSM2529_WriteReg(SSM2529_APLL_CTRL6, 0x03); // TODO: CHECK CHECK

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

#if 1
	// ToDo: Verify with real hardware. Do we need both registers?
	// ToDo: Do something with the returned status registers.
	if(mute)
	{
		// Save volume in case of un mute, and set volume to 0xFF to mute
		SSM2529_ReadReg(SSM2529_VOL_BF_FDSP, &last_volume_bf);
		SSM2529_ReadReg(SSM2529_VOL_AF_FDSP, &last_volume_af);

		SSM2529_WriteReg(SSM2529_VOL_BF_FDSP, 0xFF);
		SSM2529_WriteReg(SSM2529_VOL_AF_FDSP, 0xFF);
	}
	else
	{
		// Only update volume if currently muted
		uint8_t temp_volume_bf = 0;
		SSM2529_ReadReg(SSM2529_VOL_BF_FDSP, &temp_volume_bf);
		if(temp_volume_bf == 0xFF)
		{
			SSM2529_WriteReg(SSM2529_VOL_BF_FDSP, last_volume_bf);
		}

		uint8_t temp_volume_af = 0;
		SSM2529_ReadReg(SSM2529_VOL_AF_FDSP, &temp_volume_af);
		if(temp_volume_af == 0xFF)
		{
			SSM2529_WriteReg(SSM2529_VOL_AF_FDSP, last_volume_af);
		}
	}
#endif

}

/* Output detailed SSM2529 status. */
int SSM2529_print_detailed_status()
{
  int status;

  // Get volume and mute register
  ssm2529_vol_and_mute_ctrl_t ssm2529_vol_and_mute_ctrl;
  status = SSM2529_ReadReg(SSM2529_Volume_and_mute_control, &ssm2529_vol_and_mute_ctrl.init_all);
  if (status == kStatus_Success) {
    printf("  Volume and mute register (%02x): 0x%x\n",
    		SSM2529_Volume_and_mute_control, ssm2529_vol_and_mute_ctrl.init_all);
    printf("    ana_gain: %d\n", ssm2529_vol_and_mute_ctrl.ana_gain);
    printf("    dig_vol_force: %d\n", ssm2529_vol_and_mute_ctrl.dig_vol_force);
    printf("    pdp_vol_force: %d\n", ssm2529_vol_and_mute_ctrl.pdp_vol_force);
    printf("    sr_auto: %d\n", ssm2529_vol_and_mute_ctrl.sr_auto);
    printf("    clk_loss_det: %d\n", ssm2529_vol_and_mute_ctrl.clk_loss_det);
  }
  else {
    printf("Error reading volume and mute register: %d\n", status);
  }

  // Get analog pll control 6 register
  ssm2529_apll_ctrl6_t ssm2529_apll_ctrl6;
  status = SSM2529_ReadReg(SSM2529_APLL_CTRL6, &ssm2529_apll_ctrl6.init_all);
  if (status == kStatus_Success) {
    printf("  Analog PLL control 6 register (%02x): 0x%x\n",
    		SSM2529_APLL_CTRL6, ssm2529_apll_ctrl6.init_all);
    printf("    coren: %d\n", ssm2529_apll_ctrl6.coren);
    printf("    pllen: %d\n", ssm2529_apll_ctrl6.pllen);
    printf("    apll_lock: %d\n", ssm2529_apll_ctrl6.apll_lock);
    printf("    dpll_lock: %d\n", ssm2529_apll_ctrl6.dpll_lock);
    printf("    apll_bypass: %d\n", ssm2529_apll_ctrl6.apll_bypass);
    printf("    dpll_bypass: %d\n", ssm2529_apll_ctrl6.dpll_bypass);
    printf("    fsys_dpll: %d\n", ssm2529_apll_ctrl6.fsys_dpll);
  }
  else {
    printf("Error reading analog pll control 6 register: %d\n", status);
  }

  // Get fault control 1 register
  ssm2529_fault_ctrl1_t ssm2529_fault_ctrl1;
  status = SSM2529_ReadReg(SSM2529_FAULT_CTRL1, &ssm2529_fault_ctrl1.init_all);
  if (status == kStatus_Success) {
    printf("  Fault control 1 register (%02x): 0x%x\n",
    		SSM2529_FAULT_CTRL1, ssm2529_fault_ctrl1.init_all);
    printf("    ot: %d\n", ssm2529_fault_ctrl1.ot);
    printf("    oc: %d\n", ssm2529_fault_ctrl1.oc);
    printf("    clk_loss: %d\n", ssm2529_fault_ctrl1.clk_loss);
    printf("    pdb_zc: %d\n", ssm2529_fault_ctrl1.pdb_zc);
    printf("    pdb_line: %d\n", ssm2529_fault_ctrl1.pdb_line);
  }
  else {
    printf("Error reading fault control 1 register: %d\n", status);
  }

  return status;
}

