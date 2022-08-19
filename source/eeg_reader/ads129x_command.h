/**
 * send and receive commands from TI ADS129x chips.
 *
 * Copyright (c) 2013 by Adam Feuer <adam@adamfeuer.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*
 * Original Name: adsCommand.h
 * New Name:      ads129x_command.h
 *
 * Downloaded from: https://github.com/starcat-io/hackeeg-driver-arduino/
 * Downloaded on: February, 2020
 *
 * Modified:     Jul, 2020
 * Modified by:  Gansheng Ou, David Wang
 *
 * Description: SPI command for ADS129x
 */

#ifndef _ADS129X_COMMAND_H
#define _ADS129X_COMMAND_H

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * ADS status type.
 * Used to indicate the return status of methods.
 */
typedef uint8_t ads_status;
#define ADS_STATUS_SUCCESS 0
#define ADS_STATUS_FAIL 1

ads_status ads_wreg(uint8_t reg, uint8_t val);
ads_status ads_send_command(uint8_t cmd);
#if 0
ads_status ads_send_command_leave_cs_active(uint8_t cmd);
#endif
ads_status ads_rreg(uint8_t reg, uint8_t *val);

#if defined(__cplusplus)
}
#endif

#endif // _ADS129X_COMMAND_H
