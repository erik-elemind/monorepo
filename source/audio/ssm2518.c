/***************************************************************************//**
*   @file   ssm2518.c
*   @brief  Driver File for SSM2518 Driver.
*   @author ATofan (alexandru.tofan@analog.com)
********************************************************************************
* Copyright 2012(c) Analog Devices, Inc.
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*  - Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  - Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
*  - Neither the name of Analog Devices, Inc. nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*  - The use of this software may or may not infringe the patent rights
*    of one or more patent holders.  This license does not release you
*    from the requirement that you obtain separate licenses from these
*    patent holders to use this software.
*  - Use of the software either in source or binary form, must be run
*    on or directly connected to an Analog Devices Inc. component.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
********************************************************************************
*   SVN Revision: $WCREV$
*******************************************************************************/
/*
 * Modified:     Jul, 2020
 * Modified by:  David Wang
 *
 * Downloaded from: https://github.com/analogdevicesinc/no-OS/blob/master/Pmods/PmodAMP3/
 * Downloaded on: June 18th, 2020
 */

#include "ssm2518.h"

volatile int rxData = 0x00;

/*****************************************************************************
* @brief Write to an internal SSM2518 Register.
*
* @param regAddr - register address.
* @param data - data to be written.
*
* @return None.
******************************************************************************/
void SSM2518_WriteReg(i2c_rtos_handle_t *i2c_handle, char regAddr, unsigned char data)
{
  i2c_master_transfer_t masterXfer;
  status_t status;

  memset(&masterXfer, 0, sizeof(masterXfer));
  masterXfer.slaveAddress   = SSM2518_I2C_ADDR;
  masterXfer.direction      = kI2C_Write;
  masterXfer.subaddress     = regAddr;
  masterXfer.subaddressSize = 1;
  masterXfer.data           = &data;
  masterXfer.dataSize       = 1;
  masterXfer.flags          = kI2C_TransferDefaultFlag;

  status = I2C_RTOS_Transfer(i2c_handle, &masterXfer);
  if (status != kStatus_Success) {
    // TODO: Print Error Message
  }
}

/*****************************************************************************
* @brief Read from an internal SSM2518 Register.
*
* @param regAddr - register address.
*
* @return rxBuffer[0] - data read from device.
******************************************************************************/
unsigned char SSM2518_ReadReg(i2c_rtos_handle_t *i2c_handle, char regAddr)
{
  unsigned char data;

//  I2C_Read_axi(I2C_BASEADDR, SSM2518_I2C_ADDR, regAddr, 1, rxBuffer);

  i2c_master_transfer_t masterXfer;
  status_t status;

  masterXfer.slaveAddress   = SSM2518_I2C_ADDR;
  masterXfer.direction      = kI2C_Read;
  masterXfer.subaddress     = regAddr;
  masterXfer.subaddressSize = 1;
  masterXfer.data           = &data;
  masterXfer.dataSize       = 1;
  masterXfer.flags          = kI2C_TransferDefaultFlag;

  status = I2C_RTOS_Transfer(i2c_handle, &masterXfer);
  if (status != kStatus_Success) {
    // TODO: Print Error Message
  }

  return data;
}

/*****************************************************************************
* @brief Initialize SSM2518
*
* @param None.
*
* @return None.
******************************************************************************/
void SSM2518_Init(i2c_rtos_handle_t *i2c_handle)
{
#if 0
  // Initializing SSM2518 via I2C.

  // Reset Power Control Register (RegAddr 0):
  // 00100000
  // 0******* - Software Reset            , Normal operation (do not reset)
  // *0****** - Reserved
  // **1***** - Bit Clock Source Selection, MCLK pin used as bit clock source
  // ***0000* - 64 * Fs
  // *******0 - Software Master Power-Down, Normal operation (do not power-down)
  SSM2518_WriteReg(i2c_handle, SSM2518_Reset_Power_Control,                  0x20);
  // Edge Speed And Clocking Control Register (RegAddr 1):
  // 00000000
  // 00000*** - Reserved
  // *****00* - Edge Rate Control         , No edge rate control
  // *******0 - Automatic Sample Rate Detection, Automatic detection enabled
  SSM2518_WriteReg(i2c_handle, SSM2518_Edge_Clock_Control,                   0x00);
  // Serial Audio Interface And Sample Rate Control Register (RegAddr 2)
  // 00001011
  // 0******* - Reserved
  // *00***** - Serial Data Format, I2S standard. Data is delayed by one BCLK cycle
  // ***010** - Serial Audio Interface Format, 4-slot TDM
  // ******11 - Manual Sample Rate Selection, 64 to 96 kHz
  // Note: Manual Sample Rate Selection is NOT used because Edge Clock Control Register
  //       has Automatic Sample Rate Detection enabled. (50kHz is what is actually used)
  SSM2518_WriteReg(i2c_handle, SSM2518_Serial_Interface_Sample_Rate_Control, 0x0B);//0x03
  // Serial Audio Interface Control Register (RegAddr 3):
  // 00001000
  // 0******* - Internal BCLK Generator Enable, Bit clock from BCLK pin is used.
  // *0****** - LRCLK Shape Selection         , 50% Duty Cycle
  // **0***** - LRCLK Polarity                , Rising edge (normal)
  // ***0**** - Serial Data Bit Order         , MSB first
  // ****10** - TDM Slot Width                , 16 BCLK cycles per slot
  // ******0* - BCLK Active Edge              , Rising BCLK edge used
  // *******0 - Reserved
  SSM2518_WriteReg(i2c_handle, SSM2518_Serial_Interface_Control,             0x08);
  // Channel Mapping Control Register (RegAddr 4):
  // 00100000
  // 0010**** - Right Channel Select          , Channel 2
  // ****0000 - Left Channel Select           , Channel 0
  SSM2518_WriteReg(i2c_handle, SSM2518_Channel_Mapping_Control,              0x20);
  // Left Channel Volume Control Register (RegAddr 5):
  // 11000000
  // 11000000 - Left Channel Volume Control   , 0dB  (smaller number is louder)
  SSM2518_WriteReg(i2c_handle, SSM2518_Left_Volume_Control,                  0x80);
  // Right Channel Volume Control Register (RegAddr 6):
  // 11000000
  // 11000000 - Right Channel Volume Control  , 0dB  (smaller number is louder)
  SSM2518_WriteReg(i2c_handle, SSM2518_Right_Volume_Control,                 0x80);
  // Volume and Mute Control Register (RegAddr 7):
  // 10000000
  // 1******* - Automatic Mute Enable       , Auto mute disabled
  // *0****** - Reserved
  // **0***** - Analog Gain                 , Matched to 3.6V supply (other option is 5V supply)
  // ***0**** - Digital De-Emphasis Filter  , De-emphasis disabled (normal operation)
  // ****0*** - Volume Link                 , Normal operation (L and R channels have separate vol cntrl)
  // *****0** - Right Channel Soft Mute     , Normal operation (R not muted)
  // ******0* - Left Channel Soft Mute      , Normal operation (L not muted)
  // *******0 - Master Mute Control         , Normal operation (R and L not muted)
  SSM2518_WriteReg(i2c_handle, SSM2518_Volume_Mute_Control,                  0x80);
  // Fault Control Register 1 (RegAddr 8):
  // 00001100
  // 0******* - Left Channel Overcurrent Fault  (read only), Normal operation
  // *0****** - Right Channel Overcurrent Fault (read only), Normal operation
  // **0***** - Overtemperature Fault Status    (read only), Normal operation
  // ***0**** - Manual Fault Recovery           (read only), Normal operation
  // ****11** - Max Fault Recovery Attempts                , Unlimited attempts
  // ******00 - Auto Fault Recovery         , Auto recovery from overtemp and overcurrent.
  SSM2518_WriteReg(i2c_handle, SSM2518_Fault_Control_1,                      0x0C);
  // Power and Fault Control Register (RegAddr 9):
  // 10000000
  // 10****** - Auto Recovery Delay Time   , 40 ms autorecovery delay
  // **0***** - Reserved
  // ***0**** - Class-D Amplifier Low Power Mode, High performance operation
  // ****0*** - DAC Low Power Mode         , Normal operation (not low power operation)
  // *****0** - Right Channel Powerdown    , Normal operation (R not powered down)
  // ******0* - Left Channel Powerdown     , Normal operation (L not powered down)
  // *******0 - Automatic Power-Down Enable, auto power-down disabled
  SSM2518_WriteReg(i2c_handle, SSM2518_Power_Fault_Control,                  0x80);
#else
  // Initializing SSM2518 via I2C.

  // Reset Power Control Register (RegAddr 0):
  // 00100000
  // 0******* - Software Reset            , Normal operation (do not reset)
  // *0****** - Reserved
  // **1***** - Bit Clock Source Selection, MCLK pin used as bit clock source
  // ***0001* - 128 * Fs
  // *******0 - Software Master Power-Down, Normal operation (do not power-down)
  SSM2518_WriteReg(i2c_handle, SSM2518_Reset_Power_Control,                  0x22);
  // Edge Speed And Clocking Control Register (RegAddr 1):
  // 00000000
  // 00000*** - Reserved
  // *****00* - Edge Rate Control         , No edge rate control
  // *******0 - Automatic Sample Rate Detection, Automatic detection enabled
  SSM2518_WriteReg(i2c_handle, SSM2518_Edge_Clock_Control,                   0x00);
  // Serial Audio Interface And Sample Rate Control Register (RegAddr 2)
  // 00001011
  // 0******* - Reserved
  // *00***** - Serial Data Format, I2S standard. Data is delayed by one BCLK cycle
  // ***000** - Serial Audio Interface Format, I2S
  // ******01 - Manual Sample Rate Selection, 16 to 24 kHz
  // Note: Manual Sample Rate Selection is NOT used because Edge Clock Control Register
  //       has Automatic Sample Rate Detection enabled. (50kHz is what is actually used)
  SSM2518_WriteReg(i2c_handle, SSM2518_Serial_Interface_Sample_Rate_Control, 0x01);//0x03
  // Serial Audio Interface Control Register (RegAddr 3):
  // 00001000
  // 0******* - Internal BCLK Generator Enable, Bit clock from BCLK pin is used.
  // *0****** - LRCLK Shape Selection         , 50% Duty Cycle
  // **0***** - LRCLK Polarity                , Rising edge (normal)
  // ***0**** - Serial Data Bit Order         , MSB first
  // ****00** - TDM Slot Width                , 32 BCLK (used inly in TDM mode)
  // ******0* - BCLK Active Edge              , Rising BCLK edge used
  // *******0 - Reserved
  SSM2518_WriteReg(i2c_handle, SSM2518_Serial_Interface_Control,             0x00);
  // Channel Mapping Control Register (RegAddr 4):
  // 00100000
  // 0001**** - Right Channel Select          , Channel 2
  // ****0000 - Left Channel Select           , Channel 0
  SSM2518_WriteReg(i2c_handle, SSM2518_Channel_Mapping_Control,              0x10);
  // Left Channel Volume Control Register (RegAddr 5):
  // 11000000
  // 11000000 - Left Channel Volume Control   , 0dB  (smaller number is louder)
  SSM2518_WriteReg(i2c_handle, SSM2518_Left_Volume_Control,                  0x80);
  // Right Channel Volume Control Register (RegAddr 6):
  // 11000000
  // 11000000 - Right Channel Volume Control  , 0dB  (smaller number is louder)
  SSM2518_WriteReg(i2c_handle, SSM2518_Right_Volume_Control,                 0x80);
  // Volume and Mute Control Register (RegAddr 7):
  // 10000000
  // 1******* - Automatic Mute Enable       , Auto mute disabled
  // *0****** - Reserved
  // **0***** - Analog Gain                 , Matched to 3.6V supply (other option is 5V supply)
  // ***0**** - Digital De-Emphasis Filter  , De-emphasis disabled (normal operation)
  // ****0*** - Volume Link                 , Normal operation (L and R channels have separate vol cntrl)
  // *****0** - Right Channel Soft Mute     , Normal operation (R not muted)
  // ******0* - Left Channel Soft Mute      , Normal operation (L not muted)
  // *******0 - Master Mute Control         , Normal operation (R and L not muted)
  SSM2518_WriteReg(i2c_handle, SSM2518_Volume_Mute_Control,                  0x80);
  // Fault Control Register 1 (RegAddr 8):
  // 00001100
  // 0******* - Left Channel Overcurrent Fault  (read only), Normal operation
  // *0****** - Right Channel Overcurrent Fault (read only), Normal operation
  // **0***** - Overtemperature Fault Status    (read only), Normal operation
  // ***0**** - Manual Fault Recovery           (read only), Normal operation
  // ****11** - Max Fault Recovery Attempts                , Unlimited attempts
  // ******00 - Auto Fault Recovery         , Auto recovery from overtemp and overcurrent.
  SSM2518_WriteReg(i2c_handle, SSM2518_Fault_Control_1,                      0x0C);
  // Power and Fault Control Register (RegAddr 9):
  // 10000000
  // 10****** - Auto Recovery Delay Time   , 40 ms autorecovery delay
  // **0***** - Reserved
  // ***0**** - Class-D Amplifier Low Power Mode, High performance operation
  // ****0*** - DAC Low Power Mode         , Normal operation (not low power operation)
  // *****0** - Right Channel Powerdown    , Normal operation (R not powered down)
  // ******0* - Left Channel Powerdown     , Normal operation (L not powered down)
  // *******0 - Automatic Power-Down Enable, auto power-down disabled
  SSM2518_WriteReg(i2c_handle, SSM2518_Power_Fault_Control,                  0x80);
#endif
}
