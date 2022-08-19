#ifndef EEG_SQW
#define EEG_SQW

#include <stdint.h>
#include <stdbool.h>

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

// freq = 1-100 hz, in 1hz increments
// Duty cycle = percent. 0 = 0%, 100 = 100%, 50 = 50%, etc
int set_eeg_sqw(uint8_t freq, uint8_t duty_cycle);
void eeg_sqw_init(void);
  
  // End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif // EEG_SQW

