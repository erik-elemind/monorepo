#include "battery.h"
#include "adc.h"

#include "utils.h"

// 0% to 95% in 5% increments.  q16.15
// Taken from https://drive.google.com/drive/folders/12jffEA6UYURV18Y0dWxxYziG3PS0u3yp
static const int32_t battery_level[] = {
		       98304,  //  0%, < 3v
		      114524,  //  5%, < 3.495v
		      117571,  // 10%, < 3.588v
		      119046,  // 15%, < 3.633v
		      120160,  // 20%, < 3.667v
		      120979,  // 25%, < 3.692v
		      121667,  // 30%, < 3.713v
		      122355,  // 35%, < 3.734v
		      123011,  // 40%, < 3.754v
		      123797,  // 45%, < 3.778v
		      123418,  // 50%, < 3.800v
		      125206,  // 55%, < 3.821v
		      125862,  // 60%, < 3.841v
		      126845,  // 65%, < 3.871v
		      128450,  // 70%, < 3.920v
		      129531,  // 75%, < 3.953v
		      130515,  // 80%, < 3.983v
		      131367,  // 85%, < 4.009v
		      132645,  // 90%, < 4.048v
		      134349,  // 95%, < 4.100v
};

uint32_t battery_get_percent() {
  /* battery voltage is on a 3:1 divider */;
  int32_t raw = adc_read(ADC_VBAT) * 3;
  uint32_t level = 0;
  for(int i = 0; i < ARRAY_SIZE(battery_level); i++) {
    if (battery_level[i] >= raw) {
      break;
    }
    level += 5;
  }

  return level;
}
