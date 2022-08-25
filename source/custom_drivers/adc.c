#include "config.h"
#include "fsl_lpadc.h"

// Match trigger + commandId with hardware adc channels
typedef struct {
  int trigger;
  int channel;
} channel_t;

// TODO: this should come from the board config
#define ADC_CH_MICROPHONE   (0)
#define ADC_CH_VBAT         (3)
static const int channels[] = {ADC_CH_MICROPHONE, ADC_CH_VBAT};

#include "fsl_power.h"
void
adc_init() {
   lpadc_conv_command_config_t lpadc_config_conv;

   // Disable LDOGPADC power down
   // TODO: Sort out if this draws more power
  // POWER_DisablePD(kPDRUNCFG_PD_LDOGPADC);

   for(int i = 0; i < (sizeof(channels) / sizeof(int)); i++) {
     // Configure a single set of conversion params
     LPADC_GetDefaultConvCommandConfig(&lpadc_config_conv);

     // Note: The ADC really works best with a 6Mhz clock (as per
     // included temp sensor example).  The PLL+ADC clock divider
     // currently set the clock to 12Mhz, so increase the sample time
     // to compensate.
     lpadc_config_conv.sampleTimeMode = kLPADC_SampleTimeADCK131;
     lpadc_config_conv.channelNumber =  channels[i];
     // Apply config
     // Commands are 1-indexed, so add 1.
     LPADC_SetConvCommandConfig(ADC0, i + 1, &lpadc_config_conv);

     // Configure the software trigger
     lpadc_conv_trigger_config_t trigger_config;
     LPADC_GetDefaultConvTriggerConfig(&trigger_config);
     trigger_config.targetCommandId = i + 1;
     trigger_config.enableHardwareTrigger = false;
     LPADC_SetConvTriggerConfig(ADC0, i, &trigger_config);
   }

   LPADC_Enable(ADC0, true);
}

// Returns q16.15
int32_t
adc_read(int channel) {
  lpadc_conv_result_t result;
  int32_t reference_voltage = 58982; // 1.8f * (1 << 15) 

  // Trigger command
  LPADC_DoSoftwareTrigger(ADC0, 1 << channel);
  while(!LPADC_GetConvResult(ADC0, &result));
  
  // result is already a q15.  Multiply by the reference voltage and return.
  return (result.convValue * reference_voltage) >> 15;
}
