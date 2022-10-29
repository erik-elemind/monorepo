/*
   Driver for TI ADS129x
   for Arduino Due and Arduino Mega2560

   Copyright (c) 2013-2019 by Adam Feuer <adam@adamfeuer.com>

   This library is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/
/*
 * Original Name: hackeeg_driver.ino
 * New Name:      ads129x.c
 *
 * Downloaded from: https://github.com/starcat-io/hackeeg-driver-arduino/
 * Downloaded on: February, 2020
 *
 * Modified:     Jul, 2020
 * Modified by:  Gansheng Ou, David Wang
 *
 * Description: SPI command for ADS129x
 */

#include <stdlib.h>

#include "pin_mux.h"
#include "fsl_gpio.h"
#include "peripherals.h"
#include "config.h"
#include "loglevels.h"

#include "ads129x_command.h"
#include "ads129x_regs.h"
#include "ads129x.h"

static const char *TAG = "eeg";  // Logging prefix for this module

static ads_status detect_active_channels(ads129x* ads);
static ads_status ads_setup(ads129x* ads);

#if 0
void encode_int32(int32_t *output, char *input, int input_len)
{
  int output_index = 0;
  int offset = 3 + TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES;
  for (register int i = offset; i < input_len; i+=3) {
    // convert from 3 bytes in 24bit 2's complement to 32bit 2's complement.
    // Note that:
    // * The original over-range code in 24bit 2's complement was:  7FFFFFh
    // * The same over-range code in 32bit 2's complement is:       007FFFFFh
    // * The original under-range code in 24bit 2's complement was: 800000h
    // * The same under-range code in 32bit 2's complement is:      FF800000h
    output[output_index++] =  (
          (input[i] << 24)
      |   (input[i+1] << 16)
      |   (input[i+2] << 8)
      ) >> 8;
  }
}

void get_sample(ads129x* ads, int32_t *output){
  encode_int32(output, (char *) ads->spi_bytes, ads->num_timestamped_spi_bytes);
}
#endif


void send_response(int status_code, const char *status_text)
{
  char response[128];
  sprintf(response, "%d %s", status_code, status_text);
  LOGV(TAG,"%s\r\n",response);
}

void ads_info_command(ads129x* ads)
{
  detect_active_channels(ads);

  LOGV(TAG,"Hardware type: %s", ads->hardware_type);
  LOGV(TAG,"Max channels: %d",  ads->max_channels);
  LOGV(TAG,"Number of active channels: %d", ads->num_active_channels);
}

ads_status ads_wakeup_command()
{
  return ads_send_command(WAKEUP);
}

ads_status ads_standby_command()
{
  return ads_send_command(STANDBY);
}

ads_status ads_reset_command(ads129x* ads)
{
  ads_status status;
  status = ads_send_command(RESET);
  if(status != ADS_STATUS_SUCCESS){
    return status;
  }
  status = ads_setup(ads);
  return status;
}

ads_status ads_start_command()
{
  ads_status status = ads_send_command(START);
  if(status != ADS_STATUS_SUCCESS){
    return status;
  }
//  sample_number_union.sample_number = 0;
  return ADS_STATUS_SUCCESS;
}

ads_status ads_stop_command()
{
  return ads_send_command(STOP);
}

ads_status ads_rdatac_command(ads129x* ads)
{
  ads_status status = detect_active_channels(ads);
  if(status != ADS_STATUS_SUCCESS){
    return status;
  }
  if (ads->num_active_channels > 0) {

    ads->is_rdatac = true;
    status = ads_send_command(RDATAC);
    if(status != ADS_STATUS_SUCCESS){
      return status;
    }
    return ADS_STATUS_SUCCESS;
  } else {
    return ADS_STATUS_FAIL;
  }
}

ads_status ads_sdatac_command(ads129x* ads)
{
  ads->is_rdatac = false;
  return ads_send_command(SDATAC);
}

//set device into RDATAC (continous) mode -it will stream data
static ads_status detect_active_channels(ads129x* ads)
{
  ads_status status;
  if ((ads->is_rdatac) || (ads->max_channels < 1)){
    return ADS_STATUS_FAIL; //we can not read registers when in RDATAC mode
  }

  ads->num_active_channels = 0;
  for (int i = 1; i <= (ads->max_channels); i++) {

    uint8_t chSet;
    status = ads_rreg(CHnSET + i, &chSet);
    if(status != ADS_STATUS_SUCCESS){
      return status;
    }
    ads->active_channels[i] = ((chSet & 7) != SHORTED);
    if ((chSet & 7) != SHORTED) (ads->num_active_channels)++;
  }
  return ADS_STATUS_SUCCESS;
}

#if 0
//static inline
ads_status ads_receive_sample(ads129x* ads, bool from_isr)
{
  /* Setup TX/RX buffers */
  uint8_t tx_cmd[ads->num_spi_bytes];
  memset(tx_cmd, 0, sizeof(tx_cmd));

  memset(ads->spi_bytes, 0, sizeof(ads->spi_bytes));

  //timestamp_union.timestamp = g_systick_inCounter;
  ads->timestamp_union.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
  ads->spi_bytes[0] = ads->timestamp_union.timestamp_bytes[0];
  ads->spi_bytes[1] = ads->timestamp_union.timestamp_bytes[1];
  ads->spi_bytes[2] = ads->timestamp_union.timestamp_bytes[2];
  ads->spi_bytes[3] = ads->timestamp_union.timestamp_bytes[3];
  ads->spi_bytes[4] = ads->sample_number_union.sample_number_bytes[0];
  ads->spi_bytes[5] = ads->sample_number_union.sample_number_bytes[1];
  ads->spi_bytes[6] = ads->sample_number_union.sample_number_bytes[2];
  ads->spi_bytes[7] = ads->sample_number_union.sample_number_bytes[3];

  /* Setup master transfer */
  status_t status;
  spi_transfer_t masterXfer = {0};
  masterXfer.txData   = tx_cmd;
  masterXfer.dataSize = ads->num_spi_bytes;
  masterXfer.rxData   = ads->spi_bytes + TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES;
  masterXfer.configFlags |= kSPI_FrameAssert;

#if 0    // Old RTOS driver code. TODO: Remove
  /* Start master transfer */
  // TODO: This should probably never be called from an ISR context
  if(from_isr){
    status = SPI_MasterTransferBlocking(EEG_SPI_RTOS_HANDLE.base, &masterXfer);
    if (status != kStatus_Success) {
      LOGV(TAG, "SPI master: error during transfer.");
      return ADS_STATUS_FAIL;
    }
  }else{
    status = SPI_RTOS_Transfer(&EEG_SPI_RTOS_HANDLE, &masterXfer);
    if (status != kStatus_Success) {
      LOGV(TAG, "SPI master: error during transfer.");
      return ADS_STATUS_FAIL;
    }
  }
#endif
  status = SPI_MasterTransferBlocking(SPI_EEG_BASE, &masterXfer);
  if (status != kStatus_Success) {
    LOGV(TAG, "SPI master: error during transfer.");
    return ADS_STATUS_FAIL;
  }

  ads->sample_number_union.sample_number++;
  return ADS_STATUS_SUCCESS;
}
#endif


static ads_status ads_setup(ads129x* ads)
{ //default settings for ADS129x
  // Send SDATAC Command (Stop Read Data Continuously mode)
  ads_send_command(SDATAC);

  //pause to provide ads129n enough time to boot up...
  vTaskDelay(pdMS_TO_TICKS(1));

  ads_status status;
  uint8_t val;
  status = ads_rreg(ID, &val);
  if (status != ADS_STATUS_SUCCESS) {
    return status;
  }

  switch (val & 0b00011111) {
  case 0b10000:
    ads->hardware_type = "ADS1294";
    ads->max_channels = 4;
    break;
  case 0b10001:
    ads->hardware_type = "ADS1296";
    ads->max_channels = 6;
    break;
  case 0b10010:
    ads->hardware_type = "ADS1298";
    ads->max_channels = 8;
    break;
  case 0b11110:
    ads->hardware_type = "ADS1299";
    ads->max_channels = 8;
    break;
  case 0b11100:
    ads->hardware_type = "ADS1299-4";
    ads->max_channels = 4;
    break;
  case 0b11101:
    ads->hardware_type = "ADS1299-6";
    ads->max_channels = 6;
    break;
  default:
    ads->max_channels = 0;
  }

  ads->num_spi_bytes = (3 * (ads->max_channels + 1)); //24-bits header plus 24-bits per channel
  ads->num_timestamped_spi_bytes = ads->num_spi_bytes + TIMESTAMP_SIZE_IN_BYTES + SAMPLE_NUMBER_SIZE_IN_BYTES;
  if (ads->max_channels == 0) { //error mode
    return ADS_STATUS_FAIL;
  } //error mode

  // All GPIO set to output 0x0000: (floating CMOS inputs can flicker on and off, creating noise)
  ads_wreg(adsGPIO, 0);
  ads_wreg(CONFIG3,PD_REFBUF | CONFIG3_const);

  // Set to max gain
  ads_set_gain(ads, 12);

  // Pull the start pin high
  GPIO_PinWrite(EEG_START_GPIO, EEG_START_PORT, EEG_START_PIN, true);

  return ADS_STATUS_SUCCESS;
}

void ads_set_gain(ads129x* ads, uint8_t gain) {
  uint8_t gain_reg_val = 0;
  switch (gain){
  case 1:
    gain_reg_val = ADS1298_GAIN_1X;
    break;
  case 2:
    gain_reg_val = ADS1298_GAIN_2X;
    break;
  case 3:
    gain_reg_val = ADS1298_GAIN_3X;
    break;
  case 4:
    gain_reg_val = ADS1298_GAIN_4X;
    break;
  case 6:
    gain_reg_val = ADS1298_GAIN_6X;
    break;
  case 8:
    gain_reg_val = ADS1298_GAIN_8X;
    break;
  case 12:
    gain_reg_val = ADS1298_GAIN_12X;
    break;
  default:
    // ToDo: Do something if gain is not recognized
    return;
  }

#if defined(VARIANT_FF2)
  // set eeg gain
  ads_wreg(CH1SET, gain_reg_val);
  ads_wreg(CH2SET, gain_reg_val);
  ads_wreg(CH3SET, gain_reg_val);
  ads_wreg(CH4SET, gain_reg_val);
  ads_wreg(CH5SET, gain_reg_val);
  ads_wreg(CH6SET, gain_reg_val);
  ads_wreg(CH7SET, gain_reg_val);
  ads_wreg(CH8SET, gain_reg_val);
#elif defined(VARIANT_FF3) || defined(VARIANT_FF4)
  // set eeg gain
  ads_wreg(CH1SET, gain_reg_val);
  ads_wreg(CH2SET, gain_reg_val);
  ads_wreg(CH3SET, gain_reg_val);
  // Set gain on skin temp sensor channel to 1.
  ads_wreg(CH4SET, ADS1298_GAIN_1X);
#endif

}

void ads_init(ads129x* ads){
#if defined(EEG_CP_CONTROL) && (EEG_CP_CONTROL > 0)
  /* Configure EEG negative charge pump */
  gpio_pin_config_t EEG_CP_EN_config = {
      .pinDirection = kGPIO_DigitalOutput,
      .outputLogic = 0U
  };
  /* Initialize GPIO functionality on pin PIO1_13 (pin 2)  */
#if defined(VARIANT_FF2) || defined(VARIANT_FF3)
  GPIO_PinInit(BOARD_INITPINS_EEG_CP_EN_GPIO, BOARD_INITPINS_EEG_CP_EN_PORT, BOARD_INITPINS_EEG_CP_EN_PIN, &EEG_CP_EN_config);
#endif
#endif

#if defined(EEG_LDO_CONTROL) && (EEG_LDO_CONTROL > 0)
  /* Configure EEG +2.5/-2.5V LDO */
  gpio_pin_config_t EEG_LDO_EN_config = {
      .pinDirection = kGPIO_DigitalOutput,
      .outputLogic = 0U
  };
  /* Initialize GPIO functionality on pin PIO1_19 (pin 58)  */
  GPIO_PinInit(BOARD_INITPINS_EEG_LDO_EN_GPIO, BOARD_INITPINS_EEG_LDO_EN_PORT, BOARD_INITPINS_EEG_LDO_EN_PIN, &EEG_LDO_EN_config);
#endif
}

ads_status ads_on(ads129x* ads) {
#if defined(EEG_CP_CONTROL) && (EEG_CP_CONTROL > 0)
  // Power ON EEG negative charge pump
#if defined(VARIANT_FF2) || defined(VARIANT_FF3)
  GPIO_PinWrite(BOARD_INITPINS_EEG_CP_EN_GPIO, BOARD_INITPINS_EEG_CP_EN_PORT, BOARD_INITPINS_EEG_CP_EN_PIN, EEG_CP_ENABLE_LEVEL);
#endif
#endif

#if defined(EEG_LDO_CONTROL) && (EEG_LDO_CONTROL > 0)
  // Power ON EEG +2.5/-2.5V LDO
  GPIO_PinWrite(BOARD_INITPINS_EEG_LDO_EN_GPIO, BOARD_INITPINS_EEG_LDO_EN_PORT, BOARD_INITPINS_EEG_LDO_EN_PIN, EEG_LDO_ENABLE_LEVEL);
#endif

  // Power ON the ADS129x
  GPIO_PinWrite(EEG_PWDN_GPIO, EEG_PWDN_PORT, EEG_PWDN_PIN, true);

  //Start ADS129x
  //wait for the ads129n to be ready - it can take a while to charge caps
  vTaskDelay(pdMS_TO_TICKS(500));

  GPIO_PinWrite(EEG_RESET_GPIO, EEG_RESET_PORT, EEG_RESET_PIN, true);

  GPIO_PinWrite(EEG_RESET_GPIO, EEG_RESET_PORT, EEG_RESET_PIN, false);
  //  Wait for 18 tCLKs AKA 9 microseconds
  vTaskDelay(pdMS_TO_TICKS(1));

  GPIO_PinWrite(EEG_RESET_GPIO, EEG_RESET_PORT, EEG_RESET_PIN, true);

  return ads_setup(ads);
}

void ads_off(ads129x* ads){
  // Power OFF the ADS129x
  GPIO_PinWrite(EEG_PWDN_GPIO, EEG_PWDN_PORT, EEG_PWDN_PIN, false);

#if defined(EEG_LDO_CONTROL) && (EEG_LDO_CONTROL>0)
  // Power OFF EEG +2.5/-2.5V LDO
  GPIO_PinWrite(BOARD_INITPINS_EEG_LDO_EN_GPIO, BOARD_INITPINS_EEG_LDO_EN_PORT, BOARD_INITPINS_EEG_LDO_EN_PIN, !EEG_LDO_ENABLE_LEVEL);
#endif

#if defined(EEG_CP_CONTROL) && (EEG_CP_CONTROL>0)
  // Power OFF EEG negative charge pump
  GPIO_PinWrite(BOARD_INITPINS_EEG_CP_EN_GPIO, BOARD_INITPINS_EEG_CP_EN_PORT, BOARD_INITPINS_EEG_CP_EN_PIN, !EEG_CP_ENABLE_LEVEL);
#endif

  ads->hardware_type = "unknown";
  ads->max_channels = 0;
  ads->num_active_channels = 0;
}

/*
static void ads_log_eeg_channels(ads129x* ads){
    LOGV(TAG,"%02X%02X%02X %02X%02X%02X %02X%02X%02X %02X%02X%02X %02X%02X%02X %02X%02X%02X %02X%02X%02X %02X%02X%02X",
        ads->spi_bytes[11], ads->spi_bytes[12], ads->spi_bytes[13],
        ads->spi_bytes[14], ads->spi_bytes[15], ads->spi_bytes[16],
        ads->spi_bytes[17], ads->spi_bytes[18], ads->spi_bytes[19],
        ads->spi_bytes[20], ads->spi_bytes[21], ads->spi_bytes[22],
        ads->spi_bytes[23], ads->spi_bytes[24], ads->spi_bytes[25],
        ads->spi_bytes[26], ads->spi_bytes[27], ads->spi_bytes[28],
        ads->spi_bytes[29], ads->spi_bytes[30], ads->spi_bytes[31],
        ads->spi_bytes[32], ads->spi_bytes[33], ads->spi_bytes[34]
    );
}
*/
