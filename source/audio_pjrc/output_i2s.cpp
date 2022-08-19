/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// This code is based on the Arduino library above.
// See basic Elemind Audio documentation here:
// https://docs.google.com/document/d/1uTERHHox20vDXZfLd5PRKSXwrYdiXV4cV4suHQzFgoU

//#include <Arduino.h>
//#include "memcpy_audio.h"
#include "AudioCompat.h"
#include "output_i2s.h"
#include "peripherals.h"
#include "loglevels.h"
#include "critical_section.h"

#if defined(__GNUC__) /* GNU Compiler */
#ifndef __ALIGN_END
#define __ALIGN_END __attribute__((aligned(4)))
#endif
#ifndef __ALIGN_BEGIN
#define __ALIGN_BEGIN
#endif
#else
#ifndef __ALIGN_END
#define __ALIGN_END
#endif
#ifndef __ALIGN_BEGIN
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) /* ARM Compiler */
#define __ALIGN_BEGIN __attribute__((aligned(4)))
#elif defined(__ICCARM__) /* IAR Compiler */
#define __ALIGN_BEGIN
#endif
#endif
#endif

audio_block_t * AudioOutputI2S::block_left_1st = NULL;
audio_block_t * AudioOutputI2S::block_right_1st = NULL;
audio_block_t * AudioOutputI2S::block_left_2nd = NULL;
audio_block_t * AudioOutputI2S::block_right_2nd = NULL;
uint16_t AudioOutputI2S::block_left_offset = 0;
uint16_t AudioOutputI2S::block_right_offset = 0;
bool AudioOutputI2S::update_responsibility = false;
//DMAChannel AudioOutputI2S::dma(false);
//DMAMEM __attribute__((aligned(32))) static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
__ALIGN_BEGIN uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES] __ALIGN_END;
static i2s_transfer_t s_TxTransfer1;
static i2s_transfer_t s_TxTransfer2;
static volatile bool toggle_i2s_transfer = false;
//static volatile unsigned long transfer_counter = 0;

// Interrupt service routine for I2S:
void audio_i2s_isr(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData);

#if defined(__IMXRT1062__)
#include "utility/imxrt_hw.h"
#endif

void AudioOutputI2S::init(void)
{
	block_left_1st = NULL;
	block_right_1st = NULL;
	block_left_2nd = NULL;
	block_right_2nd = NULL;
	block_left_offset = 0;
	block_right_offset = 0;
	toggle_i2s_transfer = false;

	update_responsibility = update_setup();

	memset(i2s_tx_buffer,0,sizeof(i2s_tx_buffer));

	// Set up the I2S output buffers.
	// Divide the buffer into two for ping-pong DMA to keep the pipe filled.
	s_TxTransfer1.data     = (uint8_t*) &i2s_tx_buffer[0];
	s_TxTransfer1.dataSize = sizeof(i2s_tx_buffer)/2;
	s_TxTransfer2.data     = (uint8_t*) &i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
	s_TxTransfer2.dataSize = sizeof(i2s_tx_buffer)/2;

	I2S_TxTransferCreateHandleDMA(
		AUDIO_I2S_BASE, 
		&AUDIO_I2S_DMA_TX_HANDLE, 
		&AUDIO_I2S_TX_HANDLE, 
		audio_i2s_isr, 
		(void *)&toggle_i2s_transfer);
}

void AudioOutputI2S::begin(void)
{
	// Kick off transmission, which currently runs continuously until end().
	I2S_TxTransferSendDMA(AUDIO_I2S_BASE, &AUDIO_I2S_DMA_TX_HANDLE, s_TxTransfer1);
	I2S_TxTransferSendDMA(AUDIO_I2S_BASE, &AUDIO_I2S_DMA_TX_HANDLE, s_TxTransfer2);
}

void AudioOutputI2S::end(void)
{
	I2S_TransferAbortDMA(AUDIO_I2S_BASE, &AUDIO_I2S_DMA_TX_HANDLE);
}

void AudioOutputI2S::handle_audio_i2s_event()
{
	const int16_t *src, *end;
	int16_t *dest;
	audio_block_t *block;
//	uint32_t saddr;
	uint32_t offset;

//	saddr = (uint32_t)(dma.CFG->SAR);
//	dma.clearInterrupt();
//	if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
	if (toggle_i2s_transfer) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)&i2s_tx_buffer[0];
		end = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
	}

	// Does the left block have data to send?
	block = AudioOutputI2S::block_left_1st;
	if (block) {
		// Fill the output buffer, in interleaved fashion.
		offset = AudioOutputI2S::block_left_offset;
		src = &block->data[offset];
		do {
			*dest = *src++;
			dest += 2;
		} while (dest < end);

		// Advance the position in the source buffer
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			// Continue at offset within source block.
			AudioOutputI2S::block_left_offset = offset;
		} else {
			// Advance to next source block.
			AudioOutputI2S::block_left_offset = 0;
			AudioStream::release(block);
			AudioOutputI2S::block_left_1st = AudioOutputI2S::block_left_2nd;
			AudioOutputI2S::block_left_2nd = NULL;
		}
	} else {
		// Nothing to send. Fill buffer with 0s.
		do {
			*dest = 0;
			dest += 2;
		} while (dest < end);
	}

	// Rewind the destination pointer to the beginning of the buffer, except
	// off by 1 so that the right samples are interleaved.
	dest -= AUDIO_BLOCK_SAMPLES - 1;

	// Does the right block have data to send?
	block = AudioOutputI2S::block_right_1st;
	if (block) {
		// Fill the output buffer, in interleaved fashion.
		offset = AudioOutputI2S::block_right_offset;
		src = &block->data[offset];
		do {
			*dest = *src++;
			dest += 2;
		} while (dest < end);

		// Advance the position in the source buffer
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			// Continue at offset within source block.
			AudioOutputI2S::block_right_offset = offset;
		} else {
			// Advance to next source block.
			AudioOutputI2S::block_right_offset = 0;
			AudioStream::release(block);
			AudioOutputI2S::block_right_1st = AudioOutputI2S::block_right_2nd;
			AudioOutputI2S::block_right_2nd = NULL;
		}
	} else {
		// Nothing to send. Fill buffer with 0s.
		do {
			*dest = 0;
			dest += 2;
		} while (dest < end);
	}

	// We have a very short time to get the next I2S buffer out via DMA.
	// A SysTick (or any other) interrupt here can cause an audio glitch.

	taskENTER_CRITICAL();

	status_t status;
	if (toggle_i2s_transfer) {
		// debug_uart_puts((char*)"audio_i2s_isr tx2");
		status = I2S_TxTransferSendDMA(AUDIO_I2S_BASE, &AUDIO_I2S_DMA_TX_HANDLE, s_TxTransfer2);
	} else {
		// debug_uart_puts((char*)"audio_i2s_isr tx1");
		status = I2S_TxTransferSendDMA(AUDIO_I2S_BASE, &AUDIO_I2S_DMA_TX_HANDLE, s_TxTransfer1);
	}

	taskEXIT_CRITICAL();

	if (status == kStatus_I2S_Busy) {
		LOGV("output_i2s","Failed to initiate audio dma: kStatus_SPI_Busy.");
	}

	if (toggle_i2s_transfer) {
		// Now that the next chunk of DMA has been shipped off, invoke update_all():
		if (AudioOutputI2S::update_responsibility) AudioStream::update_all_streams();
	}

	toggle_i2s_transfer = !toggle_i2s_transfer;
}

void AudioOutputI2S::update(void)
{
	// null audio device: discard all incoming data
	//if (!active) return;
	//audio_block_t *block = receiveReadOnly();
	//if (block) release(block);

	// debug_uart_puts((char*)"audio update");

	audio_block_t *block;

	// Does the left channel have any data to send?
	block = receiveReadOnly(0); // input 0 = left channel
	if (block) {
		// debug_uart_puts((char*)"L");

		AUDIO_ENTER_CRITICAL();
		if (block_left_1st == NULL) {
			// First block empty, use it for the new output.
			block_left_1st = block;
			block_left_offset = 0;
			AUDIO_EXIT_CRITICAL();
		} else if (block_left_2nd == NULL) {
			// Second block empty.
			block_left_2nd = block;
			AUDIO_EXIT_CRITICAL();
		} else {
			// Both blocks in use, shift down and append the new block.
			audio_block_t *tmp = block_left_1st;
			block_left_1st = block_left_2nd;
			block_left_2nd = block;
			block_left_offset = 0;
			AUDIO_EXIT_CRITICAL();
			release(tmp);
		}
	}

	// Does the left channel have any data to send?
	block = receiveReadOnly(1); // input 1 = right channel
	if (block) {
		// debug_uart_puts((char*)"R");
		AUDIO_ENTER_CRITICAL();
		if (block_right_1st == NULL) {
			// First block empty, use it for the new output.
			block_right_1st = block;
			block_right_offset = 0;
			AUDIO_EXIT_CRITICAL();
		} else if (block_right_2nd == NULL) {
			// Second block empty.
			block_right_2nd = block;
			AUDIO_EXIT_CRITICAL();
		} else {
			// Both blocks in use, shift down and append the new block.
			audio_block_t *tmp = block_right_1st;
			block_right_1st = block_right_2nd;
			block_right_2nd = block;
			block_right_offset = 0;
			AUDIO_EXIT_CRITICAL();
			release(tmp);
		}
	}
}

bool AudioOutputI2S::is_idle(void)
{
  // This block has no audio source of its own. It is always ok to sleep.
  return true;
}



/*****************************************************************************/
// Interrupt Service Routine


#if (defined(AUDIO_ENABLE_I2S_OUTPUT) && (AUDIO_ENABLE_I2S_OUTPUT > 0U))

void audio_i2s_isr(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	audio_event_update_streams_from_isr(completionStatus);
}

#else

void audio_i2s_isr(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
}

#endif
