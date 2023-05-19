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
//! Glue layer between the Memfault SDK and the underlying platform
//!
//! TODO: Fill in FIXMEs below for your platform

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"
#include "memfault/ports/freertos.h"

#include <stdbool.h>

#include <stdio.h>

#include "fsl_debug_console.h"

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt,
                           ...) {
  const char *lvl_str;
  switch (level) {
    case kMemfaultPlatformLogLevel_Debug:
      lvl_str = "D";
      break;

    case kMemfaultPlatformLogLevel_Info:
      lvl_str = "I";
      break;

    case kMemfaultPlatformLogLevel_Warning:
      lvl_str = "W";
      break;

    case kMemfaultPlatformLogLevel_Error:
      lvl_str = "E";
      break;

    default:
      return;
      break;
  }

  va_list args;
  va_start(args, fmt);

  char log_buf[128];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  printf("[%s] MFLT: %s\n", lvl_str, log_buf);

  va_end(args);
}

MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_tracking")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

MEMFAULT_WEAK void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = {0};
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  const uint32_t reset_cause = RSTCTL0->SYSRSTSTAT;
  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  // TODO: Convert MCU specific reboot reason to memfault enum
  if (reset_cause & (RSTCTL0_SYSRSTSTAT_VDD_POR_MASK | RSTCTL0_SYSRSTSTAT_PAD_RESET_MASK | RSTCTL0_SYSRSTSTAT_ARM_APD_RESET_MASK))
  {
	  reset_reason = kMfltRebootReason_SoftwareReset;
  }
  else if (reset_cause & (RSTCTL0_SYSRSTSTAT_VDD_POR_MASK | RSTCTL0_SYSRSTSTAT_PAD_RESET_MASK))
  {
	  reset_reason = kMfltRebootReason_PinReset;
  }
  else if (reset_cause & RSTCTL0_SYSRSTSTAT_VDD_POR_MASK)
  {
	  reset_reason = kMfltRebootReason_PowerOnReset;
  }
  else
  {
	  reset_reason = kMfltRebootReason_Unknown;
  }

  *info = (sResetBootupInfo){
      .reset_reason_reg = reset_cause,
      .reset_reason = reset_reason,
  };
}

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  // !FIXME: Populate with platform device information

  // IMPORTANT: All strings returned in info must be constant
  // or static as they will be used _after_ the function returns

  // See https://mflt.io/version-nomenclature for more context
  *info = (sMemfaultDeviceInfo) {
    // An ID that uniquely identifies the device in your fleet
    // (i.e serial number, mac addr, chip id, etc)
    // Regular expression defining valid device serials: ^[-a-zA-Z0-9_]+$
    .device_serial = "ERIK_B07",
     // A name to represent the firmware running on the MCU.
    // (i.e "ble-fw", "main-fw", or a codename for your project)
    .software_type = "imxrt685-fw",
    // The version of the "software_type" currently running.
    // "software_type" + "software_version" must uniquely represent
    // a single binary
    .software_version = "0.13.0",
    // The revision of hardware for the device. This value must remain
    // the same for a unique device.
    // (i.e evt, dvt, pvt, or rev1, rev2, etc)
    // Regular expression defining valid hardware versions: ^[-a-zA-Z0-9_\.\+]+$
    .hardware_version = "alpha1",
  };
}

//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void memfault_platform_reboot(void) {
  // !FIXME: Perform any final system cleanup here
  size_t total_size = 0;
  if (memfault_coredump_has_valid_coredump(&total_size)) {
    MEMFAULT_LOG_INFO("reboot: coredump present!");
    memfault_data_export_dump_chunks();
  } else {MEMFAULT_LOG_INFO("reboot: coredump NOT present!");}

  // !FIXME: Reset System
  NVIC_SystemReset();
  while (1) { } // unreachable
}

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  // !FIXME: If the device tracks real time, update 'unix_timestamp_secs' with seconds since epoch
  // This will cause events logged by the SDK to be timestamped on the device rather than when they
  // arrive on the server
  *time = (sMemfaultCurrentTime) {
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = 0
    },
  };

  // !FIXME: If device does not track time, return false, else return true if time is valid
  return false;
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    // !FIXME: Update with list of valid memory banks to collect in a coredump
    {.start_addr = 0x00000000, .length = 0xFFFFFFFF},
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_mcu_mem_regions); i++) {
    const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
    const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return MEMFAULT_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

//! !FIXME: This function _must_ be called by your main() routine prior
//! to starting an RTOS or baremetal loop.
int memfault_platform_boot(void) {
  // !FIXME: Add init to any platform specific ports here.
  // (This will be done in later steps in the getting started Guide)
  memfault_freertos_port_boot();

  memfault_build_info_dump();
  memfault_device_info_dump();
  memfault_platform_reboot_tracking_boot();

  size_t total_size = 0;
  if (memfault_coredump_has_valid_coredump(&total_size)) {
    MEMFAULT_LOG_INFO("boot: coredump present! size=%d", total_size);
  } else {MEMFAULT_LOG_INFO("boot: coredump NOT present!");}

  // initialize the event storage buffer
  static uint8_t s_event_storage[1024];
  const sMemfaultEventStorageImpl *evt_storage =
    memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));

  // configure trace events to store into the buffer
  memfault_trace_event_boot(evt_storage);

  // record the current reboot reason
  memfault_reboot_tracking_collect_reset_info(evt_storage);

  // configure the metrics component to store into the buffer
  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);

  MEMFAULT_LOG_INFO("Memfault Initialized!");

  return 0;
}
