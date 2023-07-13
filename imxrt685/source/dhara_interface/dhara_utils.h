/*
 * dhara_utils.h
 *
 * Copyright (C) 2021 Igor Institute, Inc.
 *
 * Created: Feb, 2021
 * Author:  Derek Simkowiak
 *
 * Description: Dhara Flash Translation Layer utilities (buffer, mount, etc.)
 *
 */
#ifndef DHARA_UTILS_H
#define DHARA_UTILS_H

#include "dhara_nand.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif
	
void dhara_pretask_init(void);

struct dhara_map *dhara_get_my_map(void);

uint16_t dhara_map_sector_size_bytes(struct dhara_map *map);

// Pre-emptive recovery hint.
//
// This is a polled event variable with two operations:
//   - set: set the event variable's state to signalled
//   - take: set the event variable's state to unsignalled and return
//       whether or not it was signalled prior to the call
//
// The take operation must have a single consumer thread, but put may be
// called from multiple threads.
void dhara_recovery_hint_set(void);
int dhara_recovery_hint_take(void);

#ifdef __cplusplus
}
#endif

#endif  // DHARA_UTILS_H
