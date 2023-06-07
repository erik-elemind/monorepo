#include "config.h"
#include "fsl_lpadc.h"
#include "fsl_clock.h"
#include "fsl_power.h"
#include <stdio.h>

#define ADC_COMMAND_ID 15U

void
adc_init() {
	lpadc_config_t lpAdcConfigStruct;

	CLOCK_AttachClk(kSFRO_to_ADC_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivAdcClk, 1);

	printf("ADC INIT\r\n"); // todo remove
	LPADC_GetDefaultConfig(&lpAdcConfigStruct);
	lpAdcConfigStruct.enableAnalogPreliminary = true;
	LPADC_Init(ADC0, &lpAdcConfigStruct);



//	/* Set conversion CMD configuration. */
//	LPADC_GetDefaultConvCommandConfig(&lpAdcCommandConfigStruct);
//	lpAdcCommandConfigStruct.channelNumber = 2;
//	LPADC_SetConvCommandConfig(ADC0, ADC_COMMAND_ID, &lpAdcCommandConfigStruct);
//
//
//	/* Set trigger configuration. */
//	LPADC_GetDefaultConvTriggerConfig(&lpAdcTriggerConfigStruct);
//	lpAdcTriggerConfigStruct.targetCommandId       = ADC_COMMAND_ID;
//	LPADC_SetConvTriggerConfig(ADC0, 0U, &lpAdcTriggerConfigStruct); /* Configurate the trigger0. */
//
//
//    printf("ADC Full Range: %d\r\n", 4096U);
//    if (kLPADC_SampleFullScale == lpAdcCommandConfigStruct.sampleScaleMode)
//    {
//    	printf("Full channel scale (Factor of 1).\r\n");
//    }
//    else if (kLPADC_SamplePartScale == lpAdcCommandConfigStruct.sampleScaleMode)
//    {
//    	printf("Divided input voltage signal. (Factor of 30/64).\r\n");
//    }
//
//    while (!LPADC_GetConvResult(ADC0, &lpAdcResultConfigStruct)){}
//    printf("ADC value: %d\r\n", ((lpAdcResultConfigStruct.convValue) >> 3U));



   // Disable LDOGPADC power down
   // TODO: Sort out if this draws more power
  // POWER_DisablePD(kPDRUNCFG_PD_LDOGPADC);

//   for(int i = 0; i < (sizeof(channels) / sizeof(int)); i++) {
//     // Configure a single set of conversion params
//     LPADC_GetDefaultConvCommandConfig(&mLpadcCommandConfigStruct);
//
//     // Note: The ADC really works best with a 6Mhz clock (as per
//     // included temp sensor example).  The PLL+ADC clock divider
//     // currently set the clock to 12Mhz, so increase the sample time
//     // to compensate.
//     mLpadcCommandConfigStruct.sampleTimeMode = kLPADC_SampleTimeADCK131;
//     mLpadcCommandConfigStruct.channelNumber =  channels[i];
//     // Apply config
//     // Commands are 1-indexed, so add 1.
//     LPADC_SetConvCommandConfig(ADC0, i + 1, &lpadc_config_conv);
//
//     // Configure the software trigger
//     lpadc_conv_trigger_config_t trigger_config;
//     LPADC_GetDefaultConvTriggerConfig(&trigger_config);
//     trigger_config.targetCommandId = i + 1;
//     trigger_config.enableHardwareTrigger = false;
//     LPADC_SetConvTriggerConfig(ADC0, i, &trigger_config);
//   }
//
//   LPADC_Enable(ADC0, true);
}

// Returns q16.15
int32_t
adc_read(int channel) {
//  lpadc_conv_result_t result;
//  int32_t reference_voltage = 58982; // 1.8f * (1 << 15)
//
//  // Trigger command
//  LPADC_DoSoftwareTrigger(ADC0, 1 << channel);
//  while(!LPADC_GetConvResult(ADC0, &result));
//
//  // result is already a q15.  Multiply by the reference voltage and return.
//  return (result.convValue * reference_voltage) >> 15;

	lpadc_conv_command_config_t lpAdcCommandConfigStruct;
	lpadc_conv_trigger_config_t lpAdcTriggerConfigStruct;
	lpadc_conv_result_t lpAdcResultConfigStruct;

	/* Set conversion CMD configuration. */
	LPADC_GetDefaultConvCommandConfig(&lpAdcCommandConfigStruct);
	lpAdcCommandConfigStruct.channelNumber = 2;
	LPADC_SetConvCommandConfig(ADC0, ADC_COMMAND_ID, &lpAdcCommandConfigStruct);


	/* Set trigger configuration. */
	LPADC_GetDefaultConvTriggerConfig(&lpAdcTriggerConfigStruct);
	lpAdcTriggerConfigStruct.targetCommandId       = ADC_COMMAND_ID;
	LPADC_SetConvTriggerConfig(ADC0, 0U, &lpAdcTriggerConfigStruct); /* Configurate the trigger0. */


	printf("ADC Full Range: %d\r\n", 4096U);
	if (kLPADC_SampleFullScale == lpAdcCommandConfigStruct.sampleScaleMode)
	{
		printf("Full channel scale (Factor of 1).\r\n");
	}
	else if (kLPADC_SamplePartScale == lpAdcCommandConfigStruct.sampleScaleMode)
	{
		printf("Divided input voltage signal. (Factor of 30/64).\r\n");
	}

	while (!LPADC_GetConvResult(ADC0, &lpAdcResultConfigStruct)){}
	printf("ADC value: %d\r\n", ((lpAdcResultConfigStruct.convValue) >> 3U));

	return 0;
}
