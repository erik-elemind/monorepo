/**
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

#ifdef BOARD_FF4

#define BUTTONS_NUMBER 0
#define BUTTON_PULL    NRF_GPIO_PIN_NOPULL

/** Pin definitions for Elemind Morpheus FF4 board. */

// UART pins
#define RX_PIN_NUMBER  9
#define TX_PIN_NUMBER  10
#define CTS_PIN_NUMBER 12
#define RTS_PIN_NUMBER 6

// LPC/NXP pins
#define LPC_RESETN_PIN 20
#define LPC_ISPN_PIN 30

#define ISP0N_PIN 27
#define ISP1N_PIN 29
#define ISP2N_PIN 31

// SPI config for external flash
#define SPIM0_SCK_PIN   (16)
#define SPIM0_MOSI_PIN  (14)
#define SPIM0_MISO_PIN  (15)
#define SPIM0_CSN_PIN   (24)
#define SPI_FLASH_RSTN  (5)

#endif  // BOARD_FF4

#ifdef BOARD_FF3
// Pins are the same for FF1 and FF3.

#define BUTTONS_NUMBER 0
#define BUTTON_PULL    NRF_GPIO_PIN_NOPULL

/** Pin definitions for Elemind Morpheus FF3 board. */

// UART pins are reversed from FF1/FF2
#define RX_PIN_NUMBER  9
#define TX_PIN_NUMBER  10
#define CTS_PIN_NUMBER 12
#define RTS_PIN_NUMBER 6

#define LPC_RESETN_PIN 20
#define LPC_ISPN_PIN 30

#define ISP0N_PIN 27
#define ISP1N_PIN 29
#define ISP2N_PIN 31

// SPI config for external flash
#define SPIM0_SCK_PIN   (16)
#define SPIM0_MOSI_PIN  (14)
#define SPIM0_MISO_PIN  (15)
#define SPIM0_CSN_PIN   (25)
#define SPI_FLASH_RSTN  (5)

#endif  // BOARD_FF3

#ifdef BOARD_FF1

#define BUTTONS_NUMBER 0
#define BUTTON_PULL    NRF_GPIO_PIN_NOPULL

/** Pin definitions for Elemind Morpheus FF1 board. */
#define RX_PIN_NUMBER  10
#define TX_PIN_NUMBER  9
#define CTS_PIN_NUMBER 6
#define RTS_PIN_NUMBER 12

#define LPC_RESETN_PIN 20
#define LPC_ISPN_PIN 30

// SPI config for external flash
#define SPIM0_SCK_PIN   (16)
#define SPIM0_MOSI_PIN  (14)
#define SPIM0_MISO_PIN  (15)
#define SPIM0_CSN_PIN   (25)
#define SPI_FLASH_RSTN  (5)

#endif  // BOARD_FF1

#ifdef __cplusplus
}
#endif

#endif // CUSTOM_BOARD_H
