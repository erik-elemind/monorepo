/*
 * pmic_pca9420.c
 *
 *  Created on: Dec 16, 2022
 *      Author: DavidWang
 */

#include <stdio.h>
#include "pmic_pca9420.h"
#include "peripherals.h"
#include "loglevels.h"

#define TAG "pmic"

#define REG_ADDR_MODECFG_0_0 0x22

static pca9420_handle_t pca9420Handle;
static pca9420_modecfg_t pca9420CurrModeCfg;
static pca9420_mode_t pca9420CurrMode;
static bool pmicVoltChangedForDeepSleep;
static const pca9420_sw1_out_t pca9420VoltLevel[5] = {
    kPCA9420_Sw1OutVolt1V150, kPCA9420_Sw1OutVolt1V000, kPCA9420_Sw1OutVolt0V900,
    kPCA9420_Sw1OutVolt0V800, kPCA9420_Sw1OutVolt0V700,
};

#define PMIC_DECREASE_LVD_LEVEL_IF_HIGHER_THAN(currVolt, targetVolt) \
    do                                                               \
    {                                                                \
        if ((uint32_t)(currVolt) > (uint32_t)(targetVolt))           \
        {                                                            \
            POWER_SetLvdFallingTripVoltage(kLvdFallingTripVol_720);  \
        }                                                            \
    } while (0)

static uint32_t BOARD_CalcVoltLevel(const uint32_t *freqLevels, uint32_t num, uint32_t freq)
{
    uint32_t i;
    uint32_t volt;

    for (i = 0; i < num; i++)
    {
        if (freq > freqLevels[i])
        {
            break;
        }
    }

    if (i == 0) /* Frequency exceed max supported */
    {
        volt = POWER_INVALID_VOLT_LEVEL;
    }
    else
    {
        volt = pca9420VoltLevel[i + ARRAY_SIZE(pca9420VoltLevel) - num - 1];
    }

    return volt;
}

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

void pmic_test(void)
{
	uint8_t val;
	if(PCA9420_ReadRegs(&pca9420Handle, 0x00, &val, 1) == true)
	{
		printf("PMIC ID: 0x%2X\r\n", val);
	}
	else
	{
		printf("Read failed\r\n");
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

    /* Configuration PMIC mode to align with power lib like below:
     *  0b00    run mode, no special.
     *  0b01    deep sleep mode, vddcore 0.7V.
     *  0b10    deep powerdown mode, vddcore off.
     *  0b11    full deep powerdown mode vdd1v8 and vddcore off. */

    /* Mode 1: VDDCORE 0.7V. */
    cfg[1].sw1OutVolt = kPCA9420_Sw1OutVolt0V700;

    /* Mode 2: VDDCORE off. */
    cfg[2].enableSw1Out = false;

    /* Mode 3: VDDCORE, VDD1V8 and VDDIO off. */
    cfg[3].enableSw1Out  = false;
    cfg[3].enableSw2Out  = false;
    cfg[3].enableLdo2Out = false;

    // if (num >= 2)
    // {
    //     /* Mode 1: High drive. */
    //     cfg[1].sw1OutVolt  = kPCA9420_Sw1OutVolt1V100;
    //     cfg[1].sw2OutVolt  = kPCA9420_Sw2OutVolt1V900;
    //     cfg[1].ldo1OutVolt = kPCA9420_Ldo1OutVolt1V900;
    //     cfg[1].ldo2OutVolt = kPCA9420_Ldo2OutVolt3V300;
    // }
    // if (num >= 3)
    // {
    //     /* Mode 2: Low drive. */
    //     cfg[2].sw1OutVolt  = kPCA9420_Sw1OutVolt0V975;
    //     cfg[2].sw2OutVolt  = kPCA9420_Sw2OutVolt1V800;
    //     cfg[2].ldo1OutVolt = kPCA9420_Ldo1OutVolt1V800;
    //     cfg[2].ldo2OutVolt = kPCA9420_Ldo2OutVolt3V300;
    // }
    // if (num >= 4)
    // {
    //     /* Mode 3: VDDIO off, watchdog enabled. */
    //     cfg[3].enableLdo2Out = false;
    //     cfg[3].wdogTimerCfg  = kPCA9420_WdTimer16s;
    // }
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

void pmic_enter_ship_mode(void)
{
	uint8_t val;

	// Read mode 0 config 0 register
	if(PCA9420_ReadRegs(&pca9420Handle, REG_ADDR_MODECFG_0_0, &val, 1) == true)
	{
		printf("PMIC Mode 0 control: 0x%2X\r\n", val);
	}
	else
	{
		printf("Read failed\r\n");
	}

	// Set ship enable bit, MSB of register
	val |= 0x80;

	// This will put device into the ship mode
	PCA9420_WriteRegs(&pca9420Handle, REG_ADDR_MODECFG_0_0, &val, 1);
}

bool BOARD_SetPmicVoltageForFreq(power_part_temp_range_t tempRange,
                                 power_volt_op_range_t voltOpRange,
                                 uint32_t cm33Freq,
                                 uint32_t dspFreq)
{
    power_lvd_falling_trip_vol_val_t lvdVolt;
    uint32_t idx = (uint32_t)tempRange;
    uint32_t cm33Volt, dspVolt, volt;
    bool ret;

    PCA9420_GetCurrentMode(&pca9420Handle, &pca9420CurrMode);
    PCA9420_ReadModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);

    lvdVolt = POWER_GetLvdFallingTripVoltage();

    /* Enter FBB mode first */
    if (POWER_GetBodyBiasMode(kCfg_Run) != kPmu_Fbb)
    {
        POWER_EnterFbb();
    }

    if (voltOpRange == kVoltOpLowRange)
    {
        cm33Volt = BOARD_CalcVoltLevel(&powerLowCm33FreqLevel[idx][0], 3U, cm33Freq);
        dspVolt  = BOARD_CalcVoltLevel(&powerLowDspFreqLevel[idx][0], 3U, dspFreq);
    }
    else
    {
        cm33Volt = BOARD_CalcVoltLevel(&powerFullCm33FreqLevel[idx][0], 5U, cm33Freq);
        dspVolt  = BOARD_CalcVoltLevel(&powerFullDspFreqLevel[idx][0], 5U, dspFreq);
    }

    volt = MAX(cm33Volt, dspVolt);
    ret  = volt != POWER_INVALID_VOLT_LEVEL;
    assert(ret);

    if (ret)
    {
        if (volt < kPCA9420_Sw1OutVolt0V800)
        {
            POWER_DisableLVD();
        }
        else
        {
            if (volt < kPCA9420_Sw1OutVolt0V900)
            {
                PMIC_DECREASE_LVD_LEVEL_IF_HIGHER_THAN(lvdVolt, kLvdFallingTripVol_795);
            }
            else if (volt < kPCA9420_Sw1OutVolt1V000)
            {
                PMIC_DECREASE_LVD_LEVEL_IF_HIGHER_THAN(lvdVolt, kLvdFallingTripVol_885);
            }
            else
            {
            }
        }

        /* Configure vddcore voltage value */
        pca9420CurrModeCfg.sw1OutVolt = (pca9420_sw1_out_t)volt;
        PCA9420_WriteModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);

        if (volt >= kPCA9420_Sw1OutVolt0V800)
        {
            POWER_RestoreLVD();
        }
    }

    return ret;
}

void BOARD_SetPmicVoltageBeforeDeepSleep(void)
{
    PCA9420_GetCurrentMode(&pca9420Handle, &pca9420CurrMode);
    PCA9420_ReadModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);

    if (pca9420CurrModeCfg.sw1OutVolt == kPCA9420_Sw1OutVolt0V700)
    {
        pmicVoltChangedForDeepSleep = true;
        /* On resume from deep sleep with external PMIC, LVD is always used even if we have already disabled it.
         * Here we need to set up a safe threshold to avoid LVD reset and interrupt. */
        POWER_SetLvdFallingTripVoltage(kLvdFallingTripVol_720);
        pca9420CurrModeCfg.sw1OutVolt = kPCA9420_Sw1OutVolt0V750;
        PCA9420_WriteModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);
    }
    else
    {
    }
}

void BOARD_RestorePmicVoltageAfterDeepSleep(void)
{
    if (pmicVoltChangedForDeepSleep)
    {
        PCA9420_GetCurrentMode(&pca9420Handle, &pca9420CurrMode);
        PCA9420_ReadModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);
        pca9420CurrModeCfg.sw1OutVolt = kPCA9420_Sw1OutVolt0V700;
        PCA9420_WriteModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);
        pmicVoltChangedForDeepSleep = false;
    }
    else
    {
    }
}

void BOARD_SetPmicVoltageBeforeDeepPowerDown(void)
{
    PCA9420_GetCurrentMode(&pca9420Handle, &pca9420CurrMode);
    PCA9420_ReadModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);

    /* Wakeup from deep power down is same as POR, and need VDDCORE >= 1.0V. Otherwise
       0.9V LVD reset value may cause wakeup failure. */
    if (pca9420CurrModeCfg.sw1OutVolt < kPCA9420_Sw1OutVolt1V000)
    {
        pca9420CurrModeCfg.sw1OutVolt = kPCA9420_Sw1OutVolt1V000;
        PCA9420_WriteModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);
    }
    else
    {
    }
}

// Global function definitions
void battery_charger_init(void)
{
  LOGV(TAG, "battery_charger_init");
}

battery_charger_status_t battery_charger_get_status(void)
{
	uint8_t charger_status_0=0;
	uint8_t charger_status_2=0;
	uint8_t batt_charge_status=0;

	if(PCA9420_ReadRegs(&pca9420Handle, PCA9420_REG_CHG_STATUS_0, &charger_status_0, 1) != true)
	{
		LOGE(TAG, "Error reading charger status 0 register");
		return BATTERY_CHARGER_STATUS_FAULT;
	}
	if(PCA9420_ReadRegs(&pca9420Handle, PCA9420_REG_CHG_STATUS_2, &charger_status_2, 1) != true)
	{
		LOGE(TAG, "Error reading charger status 2 register");
		return BATTERY_CHARGER_STATUS_FAULT;
	}

	batt_charge_status = (charger_status_2 & 0x07);

	if(charger_status_0 == 0xD0 && charger_status_2 == 0x70) // Check if on battery
	{
		return BATTERY_CHARGER_STATUS_ON_BATTERY;
	}
	else if(charger_status_0 == 0xF0)
	{
		if((batt_charge_status == 5) || (batt_charge_status == 6))
		{
			return BATTERY_CHARGER_STATUS_CHARGE_COMPLETE;
		}
		else if((batt_charge_status >= 1) && (batt_charge_status < 5))
		{
			return BATTERY_CHARGER_STATUS_CHARGING;
		}
	}

	// If other cases are not valid, treat it as a fault
	return BATTERY_CHARGER_STATUS_FAULT;
}

status_t battery_charger_print_detailed_status(void)
{
	uint8_t charger_regs[4];
	if(PCA9420_ReadRegs(&pca9420Handle, 0x18, charger_regs, 4) != true)
	{
		LOGE(TAG, "Error reading charger status registers");
		return kStatus_Fail;
	}

	printf("ONREG:\r\n");
	for(uint8_t i=0;i<4;i++)
	{
		printf("Charger Status %d: %02X\r\n", i, charger_regs[i]);
	}

//	printf("ONBATT:\r\n");
//	for(uint8_t i=0;i<4;i++)
//	{
//		printf("Charger Status %d: %02X\r\n", i, charger_off_batt[i]);
//	}

	return kStatus_Success;
}

