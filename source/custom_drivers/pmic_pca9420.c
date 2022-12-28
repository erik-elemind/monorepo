/*
 * pmic_pca9420.c
 *
 *  Created on: Dec 16, 2022
 *      Author: DavidWang
 */

#include <stdio.h>
#include "pmic_pca9420.h"
#include "peripherals.h"

static pca9420_handle_t pca9420Handle;

/*
 * Sends txBuff to PCA9420 over I2C
 *
 * The following function is based on functions
 * BOARD_PMIC_I2C_Send() and BOARD_I2C_Send()
 * from NXP SDK example "evkmimxrt685_pca9420".
 */
static status_t pmic_i2c_send(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, const uint8_t *txBuff, uint8_t txBuffSize)
{
    i2c_master_transfer_t masterXfer;

    /* Prepare transfer structure. */
    masterXfer.slaveAddress   = deviceAddress;
    masterXfer.direction      = kI2C_Write;
    masterXfer.subaddress     = subAddress;
    masterXfer.subaddressSize = subAddressSize;
    masterXfer.data           = (uint8_t *)txBuff;
    masterXfer.dataSize       = txBuffSize;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    return I2C_MasterTransferBlocking(FC15_PMIC_PERIPHERAL, &masterXfer);
}

/*
 * Receives rxBuff from PCA9420 over I2C
 *
 * The following function is based on functions
 * BOARD_PMIC_I2C_Receive() and BOARD_I2C_Receive()
 * from NXP SDK example "evkmimxrt685_pca9420".
 */
static status_t pmic_i2c_receive(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, uint8_t *rxBuff, uint8_t rxBuffSize)
{
    i2c_master_transfer_t masterXfer;

    /* Prepare transfer structure. */
    masterXfer.slaveAddress   = deviceAddress;
    masterXfer.subaddress     = subAddress;
    masterXfer.subaddressSize = subAddressSize;
    masterXfer.data           = rxBuff;
    masterXfer.dataSize       = rxBuffSize;
    masterXfer.direction      = kI2C_Read;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    return I2C_MasterTransferBlocking(FC15_PMIC_PERIPHERAL, &masterXfer);
}

/*
 * Prints PMIC modes.
 *
 * The following function is based on function
 * DEMO_DumpModes()
 * from NXP SDK example "evkmimxrt685_pca9420".
 */
void pmic_dump_modes(void)
{
    pca9420_modecfg_t modeCfgs[4];
    uint32_t i;
    pca9420_regulator_mv_t regVolt;
    pca9420_mode_t currentMode;
    bool result;

    result = PCA9420_GetCurrentMode(&pca9420Handle, &currentMode);
    if (!result)
    {
        printf("Get current mode failed\r\n");
        return;
    }

    PCA9420_ReadModeConfigs(&pca9420Handle, kPCA9420_Mode0, modeCfgs, ARRAY_SIZE(modeCfgs));
    for (i = 0; i < ARRAY_SIZE(modeCfgs); i++)
    {
        PCA9420_GetRegulatorVolt(&modeCfgs[i], &regVolt);
        printf("--------------------- Mode %d%s -----------------\r\n", (int)(i), ((uint8_t)currentMode) == i ? "(*)" : "");
        printf("Mode controlled by [%s]\r\n", modeCfgs[i].modeSel == kPCA9420_ModeSelPin ? "Pin" : "I2C");
        printf("Watch dog timer    [%s]\r\n",
               modeCfgs[i].wdogTimerCfg == kPCA9420_WdTimerDisabled ? "Disabled" : "Enabled");
        if (modeCfgs[i].wdogTimerCfg != kPCA9420_WdTimerDisabled)
        {
            printf("Watch dog timeout  [%d sec]\r\n", (1U << ((((uint32_t)(modeCfgs[i].wdogTimerCfg)) >> 6U) + 3)));
        }
        /* SW1 voltage */
        printf("SW1 voltage        [%d.%03dV][%s]\r\n", (int)(regVolt.mVoltSw1 / 1000), (int)(regVolt.mVoltSw1 % 1000),
               modeCfgs[i].enableSw1Out ? "ON" : "OFF");
        /* SW2 voltage */
        printf("SW2 voltage        [%d.%03dV][%s]\r\n", (int)(regVolt.mVoltSw2 / 1000), (int)(regVolt.mVoltSw2 % 1000),
               modeCfgs[i].enableSw2Out ? "ON" : "OFF");
        /* LDO1 voltage */
        printf("LDO1 voltage       [%d.%03dV][%s]\r\n", (int)(regVolt.mVoltLdo1 / 1000), (int)(regVolt.mVoltLdo1 % 1000),
               modeCfgs[i].enableLdo1Out ? "ON" : "OFF");
        /* LDO2 voltage */
        printf("LDO2 voltage       [%d.%03dV][%s]\r\n", (int)(regVolt.mVoltLdo2 / 1000), (int)(regVolt.mVoltLdo2 % 1000),
               modeCfgs[i].enableLdo2Out ? "ON" : "OFF");
    }
    printf("----------------------- End -------------------\r\n");
}

/*
 * initializes the an array of PCA9420 modes to their default values:
 * All 4 power rails are ON.
 *
 * The following function is based on code copied
 * from NXP SDK example "evkmimxrt685_pca9420",
 * "pca9420.c", main() function.
 */
static void pmic_config_default_modes(pca9420_modecfg_t *cfg, uint32_t num){
	uint32_t i;
	for (i = 0; i < num; i++)
    {
        PCA9420_GetDefaultModeConfig(&cfg[i]);
    }
}

/*
 * The following function is based on functions
 * BOARD_ConfigPMICModes()
 * from NXP SDK example "evkmimxrt685_pca9420".
 */
static void pmic_config_modes(pca9420_modecfg_t *cfg, uint32_t num)
{
	// TODO: Update this function to encode the modes we care about.
    assert(cfg);

    if (num >= 2)
    {
        /* Mode 1: High drive. */
        cfg[1].sw1OutVolt  = kPCA9420_Sw1OutVolt1V100;
        cfg[1].sw2OutVolt  = kPCA9420_Sw2OutVolt1V900;
        cfg[1].ldo1OutVolt = kPCA9420_Ldo1OutVolt1V900;
        cfg[1].ldo2OutVolt = kPCA9420_Ldo2OutVolt3V300;
    }
    if (num >= 3)
    {
        /* Mode 2: Low drive. */
        cfg[2].sw1OutVolt  = kPCA9420_Sw1OutVolt0V975;
        cfg[2].sw2OutVolt  = kPCA9420_Sw2OutVolt1V800;
        cfg[2].ldo1OutVolt = kPCA9420_Ldo1OutVolt1V800;
        cfg[2].ldo2OutVolt = kPCA9420_Ldo2OutVolt3V300;
    }
    if (num >= 4)
    {
        /* Mode 3: VDDIO off, watchdog enabled. */
        cfg[3].enableLdo2Out = false;
        cfg[3].wdogTimerCfg  = kPCA9420_WdTimer16s;
    }
}


void pmic_init(){

    // Init PMIC
	pca9420_config_t pca9420Config;
	pca9420_modecfg_t pca9420ModeCfg[4];

    /* Init PCA9420 Component. */
    PCA9420_GetDefaultConfig(&pca9420Config);
    pca9420Config.I2C_SendFunc    = pmic_i2c_send;
    pca9420Config.I2C_ReceiveFunc = pmic_i2c_receive;
    PCA9420_Init(&pca9420Handle, &pca9420Config);

    /* Configure PMIC modes. */
    pmic_config_default_modes(pca9420ModeCfg, ARRAY_SIZE(pca9420ModeCfg));
    pmic_config_modes(pca9420ModeCfg, ARRAY_SIZE(pca9420ModeCfg));
    PCA9420_WriteModeConfigs(&pca9420Handle, kPCA9420_Mode0, &pca9420ModeCfg[0], ARRAY_SIZE(pca9420ModeCfg));

    /* Enable PMIC pad interrupts */
    /* Clear flags first. */
    POWER_ClearEventFlags(kPMC_FLAGS_INTNPADF);
    POWER_EnableInterrupts(kPMC_INT_INTRPAD);
    /* Enable PMIC interrupts. */
    PCA9420_EnableInterrupts(&pca9420Handle, kPCA9420_IntSrcSysAll | kPCA9420_IntSrcRegulatorAll);

}

