#ifndef SYSTEM_WATCHDOG_H
#define SYSTEM_WATCHDOG_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void system_watchdog_init();
void system_watchdog_feed();
uint32_t system_watchdog_reset_ms();

#ifdef __cplusplus
}
#endif

#endif //SYSTEM_WATCHDOG_H
