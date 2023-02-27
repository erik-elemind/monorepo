/*
 * board_config.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: June, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Morpheus board configuration.
 */
#include "prebuild.h"
#include "board_config.h"
#include "fsl_sctimer.h"
#include "fsl_debug_console.h"
#include "syscalls.h"  // For Nanolib shell _read()/_write()

// Declare global HAL handles here.
//
// These globals are initialized below in board_init().
// These globals are exported to modules for use as extern in board_config.h.

AT_QUICKACCESS_SECTION_DATA(static uint32_t s_ispPin[3]);
AT_QUICKACCESS_SECTION_DATA(static uint32_t s_flexspiPin[10]);
/******************************************************************************
 * POWER
 *****************************************************************************/
void BOARD_SetDeepSleepPinConfig(void)
{
    /* Backup Pin configuration. */
    s_ispPin[0]     = IOPCTL->PIO[1][15];
    s_ispPin[1]     = IOPCTL->PIO[1][16];
    s_ispPin[2]     = IOPCTL->PIO[1][17];
    s_flexspiPin[0] = IOPCTL->PIO[1][29];
    s_flexspiPin[1] = IOPCTL->PIO[2][19];
    s_flexspiPin[2] = IOPCTL->PIO[1][11];
    s_flexspiPin[3] = IOPCTL->PIO[1][12];
    s_flexspiPin[4] = IOPCTL->PIO[1][13];
    s_flexspiPin[5] = IOPCTL->PIO[1][14];
    s_flexspiPin[6] = IOPCTL->PIO[2][17];
    s_flexspiPin[7] = IOPCTL->PIO[2][18];
    s_flexspiPin[8] = IOPCTL->PIO[2][22];
    s_flexspiPin[9] = IOPCTL->PIO[2][23];

    /* Disable ISP Pin pull-ups and input buffers to avoid current leakage */
    IOPCTL->PIO[1][15] = 0;
    IOPCTL->PIO[1][16] = 0;
    IOPCTL->PIO[1][17] = 0;

    /* Disable unnecessary input buffers */
    IOPCTL->PIO[1][29] &= ~IOPCTL_PIO_IBENA_MASK;
    IOPCTL->PIO[2][19] &= ~IOPCTL_PIO_IBENA_MASK;

    /* Enable pull-ups floating FlexSPI0 pins */
    IOPCTL->PIO[1][11] |= IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK;
    IOPCTL->PIO[1][12] |= IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK;
    IOPCTL->PIO[1][13] |= IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK;
    IOPCTL->PIO[1][14] |= IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK;
    IOPCTL->PIO[2][17] |= IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK;
    IOPCTL->PIO[2][18] |= IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK;
    IOPCTL->PIO[2][22] |= IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK;
    IOPCTL->PIO[2][23] |= IOPCTL_PIO_PUPDENA_MASK | IOPCTL_PIO_PUPDSEL_MASK;
}

void BOARD_RestoreDeepSleepPinConfig(void)
{
    /* Restore the Pin configuration. */
    IOPCTL->PIO[1][15] = s_ispPin[0];
    IOPCTL->PIO[1][16] = s_ispPin[1];
    IOPCTL->PIO[1][17] = s_ispPin[2];

    IOPCTL->PIO[1][29] = s_flexspiPin[0];
    IOPCTL->PIO[2][19] = s_flexspiPin[1];
    IOPCTL->PIO[1][11] = s_flexspiPin[2];
    IOPCTL->PIO[1][12] = s_flexspiPin[3];
    IOPCTL->PIO[1][13] = s_flexspiPin[4];
    IOPCTL->PIO[1][14] = s_flexspiPin[5];
    IOPCTL->PIO[2][17] = s_flexspiPin[6];
    IOPCTL->PIO[2][18] = s_flexspiPin[7];
    IOPCTL->PIO[2][22] = s_flexspiPin[8];
    IOPCTL->PIO[2][23] = s_flexspiPin[9];
}

void BOARD_EnterDeepSleep(const uint32_t exclude_from_pd[4])
{
    BOARD_SetDeepSleepPinConfig();
    POWER_EnterDeepSleep(exclude_from_pd);
    BOARD_RestoreDeepSleepPinConfig();
}

void BOARD_EnterDeepPowerDown(const uint32_t exclude_from_pd[4])
{
    BOARD_SetDeepSleepPinConfig();
    POWER_EnterDeepPowerDown(exclude_from_pd);
    /* After deep power down wakeup, the code will restart and cannot reach here. */
    BOARD_RestoreDeepSleepPinConfig();
}

/******************************************************************************
 * BLUETOOTH
 *****************************************************************************/


int
BOARD_InitBLE()
{
  //
  // USART_BLE init (uses the FreeRTOS driver)
  //
  NVIC_SetPriority(USART_BLE_IRQn, USART_BLE_NVIC_PRIORITY);
  EnableIRQ(USART_BLE_IRQn);

  //RESET_ClearPeripheralReset(USART_BLE_RST);  // TODO: Is this necessary?
  return kStatus_Success;
}



/******************************************************************************
 * DEBUG UART
 *****************************************************************************/
int
BOARD_InitDebugConsole()
{
#ifndef CONFIG_USE_SEMIHOSTING
  // Initialize the Nanolib C system calls (_read(), _write(), _open(), etc.)
  syscalls_pretask_init();
#endif

  NVIC_SetPriority(USART_DEBUG_IRQn, USART_DEBUG_NVIC_PRIORITY);
  EnableIRQ(USART_DEBUG_IRQn);

  return kStatus_Success;
}

/******************************************************************************
 * SPI Busses
 *****************************************************************************/

void
BOARD_InitEEGSPI()
{
}

void
BOARD_InitFlashSPI()
{
}


/******************************************************************************
 * I2C on FLEXCOMM 4
 *****************************************************************************/

/******************************************************************************
 * I2C on FLEXCOMM 5
 *****************************************************************************/

/******************************************************************************
 * DEBUG LED
 *****************************************************************************/

void
BOARD_ToggleDebugLED(void) {
  GPIO_PortToggle(DEBUG_LED_GPIO, DEBUG_LED_PORT, DEBUG_LED_PIN_MASK);
}

/******************************************************************************
 * BUTTONS
 *****************************************************************************/
