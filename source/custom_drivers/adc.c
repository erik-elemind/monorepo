#include "config.h"
#include "fsl_lpadc.h"
#include "fsl_clock.h"
#include "fsl_power.h"
#include <stdio.h>

#define DEMO_LPADC_BASE             ADC0
#define DEMO_LPADC_USER_CHANNEL     1U
#define DEMO_LPADC_USER_CMDID       15U

#define ADC_COMMAND_ID 15U

void
adc_init() {

	lpadc_config_t mLpadcConfigStruct;
	lpadc_conv_trigger_config_t mLpadcTriggerConfigStruct;
	lpadc_conv_command_config_t mLpadcCommandConfigStruct;

	SYSCTL0->PDRUNCFG0_CLR = SYSCTL0_PDRUNCFG0_ADC_PD_MASK;
	SYSCTL0->PDRUNCFG0_CLR = SYSCTL0_PDRUNCFG0_ADC_LP_MASK;
	RESET_PeripheralReset(kADC0_RST_SHIFT_RSTn);

	LPADC_GetDefaultConfig(&mLpadcConfigStruct);
	mLpadcConfigStruct.enableAnalogPreliminary = true;
	LPADC_Init(DEMO_LPADC_BASE, &mLpadcConfigStruct);

	// Set battery voltage measurement
	/* Set conversion CMD configuration. */
	LPADC_GetDefaultConvCommandConfig(&mLpadcCommandConfigStruct);
	mLpadcCommandConfigStruct.sampleChannelMode = kLPADC_SampleChannelSingleEndSideA;
	mLpadcCommandConfigStruct.channelNumber = DEMO_LPADC_USER_CHANNEL;
	LPADC_SetConvCommandConfig(DEMO_LPADC_BASE, DEMO_LPADC_USER_CMDID, &mLpadcCommandConfigStruct);

	/* Set trigger configuration. */
	LPADC_GetDefaultConvTriggerConfig(&mLpadcTriggerConfigStruct);
	mLpadcTriggerConfigStruct.targetCommandId       = DEMO_LPADC_USER_CMDID;
	mLpadcTriggerConfigStruct.enableHardwareTrigger = false;
	LPADC_SetConvTriggerConfig(DEMO_LPADC_BASE, 0U, &mLpadcTriggerConfigStruct); /* Configurate the trigger0. */

	// Set NTC0 measurement

	// Set NTC1 measurement

}


int32_t adc_read(int channel) {

	lpadc_conv_result_t mLpadcResultConfigStruct;

	LPADC_DoSoftwareTrigger(DEMO_LPADC_BASE, 1U); /* 1U is trigger0 mask. */

	while (!LPADC_GetConvResult(DEMO_LPADC_BASE, &mLpadcResultConfigStruct)){}

	printf("ADC value: %d\r\n", ((mLpadcResultConfigStruct.convValue) >> 3U));

	return 0;
}
