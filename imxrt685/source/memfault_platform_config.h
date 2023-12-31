#pragma once

//! @file
//!
//! Copyright 2023 Memfault, Inc
//!
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//!     http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!
//! @brief
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// For example, decide if you want to use the Gnu Build ID.
#define MEMFAULT_USE_GNU_BUILD_ID 1
#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS 3600 // TODO: default 1 hr, change as needed
#define MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN 128 // TODO: adjust to bottleneck of end-to-end (BLE MTU: 251 to be safe, likely ok to do 253 need to look at BLE SDK)
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH 1
#define MEMFAULT_EXC_HANDLER_WATCHDOG  WDT0_IRQHandler
//#define MEMFAULT_FREERTOS_PORT_HEAP_STATS_ENABLE 1
//#define MEMFAULT_COREDUMP_HEAP_STATS_LOCK_ENABLE 0
//#define MEMFAULT_COREDUMP_COLLECT_HEAP_STATS 1

#ifdef __cplusplus
}
#endif
