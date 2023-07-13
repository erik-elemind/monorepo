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
 * New Name:      ads129x.h
 *
 * Downloaded from: https://github.com/starcat-io/hackeeg-driver-arduino/
 * Downloaded on: February, 2020
 *
 * Modified:     Jul, 2020
 * Modified by:  Gansheng Ou, David Wang
 *
 * Description: SPI command for ADS129x
 */

#ifndef _1298APP_H
#define _1298APP_H

#include "fsl_pint.h"
#include "ads129x_command.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define SPI_BUFFER_SIZE 200

// microseconds timestamp
#define TIMESTAMP_SIZE_IN_BYTES 4
typedef union{
  char timestamp_bytes[TIMESTAMP_SIZE_IN_BYTES];
  unsigned long timestamp;
} timestamp_union_t;

// sample number counter
#define SAMPLE_NUMBER_SIZE_IN_BYTES 4
typedef union{
  char sample_number_bytes[SAMPLE_NUMBER_SIZE_IN_BYTES];
  unsigned long sample_number;
} sample_number_union_t;


typedef struct {
  char *hardware_type;
  int max_channels;
  int num_active_channels;
  bool active_channels[9]; // reports whether channels 1..9 are active
  int num_spi_bytes;
  int num_timestamped_spi_bytes;
  bool is_rdatac;
} ads129x;

void ads_info_command(ads129x* ads);
ads_status ads_wakeup_command();
ads_status ads_standby_command();
ads_status ads_reset_command(ads129x* ads);
ads_status ads_start_command();
ads_status ads_stop_command();
ads_status ads_rdatac_command(ads129x* ads);
ads_status ads_sdatac_command(ads129x* ads);
ads_status ads_receive_sample(ads129x* ads, bool from_isr);
ads_status ads_turn_on_leadoff_detection(void);
ads_status ads_turn_off_leadoff_detection(void);
ads_status ads_get_leadoff_stat(uint8_t* stat);

void ads_init(ads129x *ads);
ads_status ads_on(ads129x* ads);
void ads_off(ads129x* ads);
void ads_set_gain(ads129x* ads, uint8_t gain);

void ads_set_gain(ads129x* ads, uint8_t gain);

#if defined(__cplusplus)
}
#endif

#endif //_1298APP_H

