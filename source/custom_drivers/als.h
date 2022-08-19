#ifndef ALS
#define ALS

#include <stdint.h>
#include <stdbool.h>

// Start: Tell C++ compiler to include this C header.
#ifdef __cplusplus
extern "C" {
#endif

void als_init(void);
void als_start(void);
void als_wait(void);
void als_stop(void);
int als_wait_time(void);
int als_get_lux(float* lux);
  

  // End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif // ALS

