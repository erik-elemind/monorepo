
#ifndef ADC_H
#define ADC_H



#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define ADC_MICROPHONE 0
#define ADC_VBAT 1

void adc_init();
// Returns q16.15
int32_t adc_read(int channel);

#ifdef __cplusplus
}
#endif

#endif  // ADC_H
