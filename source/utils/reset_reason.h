#ifndef RESET_REASON_H
#define RESET_REASON_H

#include "stdint.h"  // uint32_t, etc.

#ifdef __cplusplus
extern "C" {
#endif

void
save_reset_reason(void);
 
uint32_t
get_reset_reason(void);

#ifdef __cplusplus
}
#endif

#endif  // RESET_REASON_H
