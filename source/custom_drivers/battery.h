#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t battery_get_percent();
#ifdef __cplusplus
}
#endif

#endif  // BATTERY_H
