#ifndef SYSTEM_WATCHDOG_H
#define SYSTEM_WATCHDOG_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_WATCHDOG_PET_MS 5000
void system_watchdog_init();
void system_watchdog_pet();

#ifdef __cplusplus
}
#endif

#endif //SYSTEM_WATCHDOG_H
