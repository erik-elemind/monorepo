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
#include "fw_version.h"
#include "fatfs_utils.h"
#include "rtc.h"
#include "memfault/metrics/platform/overrides.h"

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"
#include "memfault/ports/freertos.h"
#include "nand_W25N04KW.h"
#include "fs_commands.h"
#include "system_watchdog.h"

#include <stdbool.h>

#include <stdio.h>

#include "fsl_debug_console.h"

static void memfault_clear_event_chunk_files(void);
static void memfault_clear_coredump_chunk_files(void);
static void memfault_save_coredump_chunks(void);

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

#define MEMFAULT_COREDUMP_NVADDR (290800)
#define MEMFAULT_COREDUMP_NV_BLKADDR (4075)
#define MEMFAULT_PLATFORM_COREDUMP_NVSTORAGE_SIZE NAND_BLOCK_SIZE


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
   static char sn_buffer[20];
   snprintf(sn_buffer, sizeof(sn_buffer), "%lu%lu%lu%lu", \
		   SYSCTL0->UUID[0], SYSCTL0->UUID[1], SYSCTL0->UUID[2], SYSCTL0->UUID[3]);

  // See https://mflt.io/version-nomenclature for more context
  *info = (sMemfaultDeviceInfo) {
    // An ID that uniquely identifies the device in your fleet
    // (i.e serial number, mac addr, chip id, etc)
    // Regular expression defining valid device serials: ^[-a-zA-Z0-9_]+$
    .device_serial = sn_buffer,
     // A name to represent the firmware running on the MCU.
    // (i.e "ble-fw", "main-fw", or a codename for your project)
    .software_type = "imxrt685-fw",
    // The version of the "software_type" currently running.
    // "software_type" + "software_version" must uniquely represent
    // a single binary
    .software_version = FW_VERSION_FULL,
    // The revision of hardware for the device. This value must remain
    // the same for a unique device.
    // (i.e evt, dvt, pvt, or rev1, rev2, etc)
    // Regular expression defining valid hardware versions: ^[-a-zA-Z0-9_\.\+]+$
    .hardware_version = "revD",
  };
}

//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void memfault_platform_reboot(void) {
  // !FIXME: Perform any final system cleanup here
//  size_t total_size = 0;
//  if (memfault_coredump_has_valid_coredump(&total_size)) {
//    MEMFAULT_LOG_INFO("reboot: coredump present!");
//    memfault_data_export_dump_chunks();
//  } else {MEMFAULT_LOG_INFO("reboot: coredump NOT present!");}

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
      .unix_timestamp_secs = (uint64_t) rtc_get()
    },
  };

  // !FIXME: If device does not track time, return false, else return true if time is valid
  if (time->info.unix_timestamp_secs > 0)
  {
	  return true;
  }
  return false;
}

void memfault_metrics_heartbeat_collect_data(void) {
  
  // get current memory every heartbeat interval
  unsigned long fs_free_bytes;
  f_getfreebytes(&fs_free_bytes, NULL);
  memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(fatfs_free_bytes), fs_free_bytes);

  // FAKE (battery_pct)
  static uint8_t fake_pct = 100;
  memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(battery_pct), fake_pct);
  fake_pct--;
  if (fake_pct == 0) fake_pct = 100;
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

bool memfault_platform_coredump_save_begin(void) {
  // pet dog prior to coredump save
  system_watchdog_pet();
  return true;
}

// MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH = 1
#if !MEMFAULT_PLATFORM_COREDUMP_STORAGE_REGIONS_CUSTOM
//! Collect the active stack as part of the coredump capture.
//! User can implement their own version to override the implementation
const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
   static sMfltCoredumpRegion s_coredump_regions[1];

   const size_t stack_size = memfault_platform_sanitize_address_range(
       crash_info->stack_address, MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT);

   s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
       crash_info->stack_address, stack_size);
   *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
   return &s_coredump_regions[0];
}
#endif

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info = (sMfltCoredumpStorageInfo) {
    .size = MEMFAULT_PLATFORM_COREDUMP_NVSTORAGE_SIZE,
  };
}

static bool prv_op_within_flash_bounds(uint32_t offset, size_t data_len) {
  sMfltCoredumpStorageInfo info = { 0 };
  memfault_platform_coredump_storage_get_info(&info);
  return (offset + data_len) <= info.size;
}

// Create nand_user data struct
static nand_user_data_t g_nand_handle = {
  .chipinfo = &nand_chipinfo,
};

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (!prv_op_within_flash_bounds(offset, erase_size)) {
    return false;
  }

  // erase block 4075 corresponding to page_addr 260800
  if(nand_erase_block(&g_nand_handle, MEMFAULT_COREDUMP_NVADDR) < 0) {
	  return false;
  }

  return true;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if (!prv_op_within_flash_bounds(offset, read_len)) {
    return false;
  }

  int status;
  static uint8_t read_buffer[NAND_PAGE_SIZE];
  memset(read_buffer, 0x00, sizeof(read_buffer));

  // map offset to page_addr and offset
  uint32_t page_num = offset / NAND_PAGE_SIZE;
  uint32_t page_offset = offset % NAND_PAGE_SIZE;

  status = nand_read_page_into_cache(&g_nand_handle, MEMFAULT_COREDUMP_NVADDR + page_num);
  // If ECC_FAIL, still try to read out the page data below (despite ECC errors).
  if (status < 0 && status != NAND_ECC_FAIL) {
    return false;
  }

  status = nand_read_page_from_cache(&g_nand_handle, page_offset, read_buffer, read_len);
  if (status == NAND_ECC_FAIL)
  {
	  return false;
  }

  memcpy(data, read_buffer, read_len);

  return true;
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if (!prv_op_within_flash_bounds(offset, data_len)) {
    return false;
  }

  int status;

  static uint8_t write_buffer[NAND_PAGE_SIZE];
  memset(write_buffer, 0x00, sizeof(write_buffer));

  uint32_t page_num = offset / NAND_PAGE_SIZE;
  uint32_t page_offset = offset % NAND_PAGE_SIZE;

  // Read-Modify-Write

  // Read
  status = nand_read_page_into_cache(&g_nand_handle, MEMFAULT_COREDUMP_NVADDR + page_num);
  // If ECC_FAIL, still try to read out the page data below (despite ECC errors).
  if (status < 0 && status != NAND_ECC_FAIL) {
    return false;
  }
  status = nand_read_page_from_cache(&g_nand_handle, 0, write_buffer, NAND_PAGE_SIZE); // always read from start of page here
  if (status == NAND_ECC_FAIL)
  {
	  return false;
  }
  // Modify
  memcpy(write_buffer+page_offset, data, data_len);

  // Erase
  if(memfault_platform_coredump_storage_erase(0,MEMFAULT_PLATFORM_COREDUMP_NVSTORAGE_SIZE))
  {
	  // Write full page to cache
	  status = nand_write_into_page_cache(&g_nand_handle, 0, write_buffer, NAND_PAGE_SIZE);
	  if (status < 0) {
		return false;
	  }

	  // Program data
	  status = nand_program_page_cache(&g_nand_handle, MEMFAULT_COREDUMP_NVADDR + page_num);
	  if (status < 0) {
		return false;
	  }
  }
  else
  {
	  return false;
  }

  return true;
}

void memfault_platform_coredump_storage_clear(void) {
  const uint8_t clear_byte = 0x0;
  memfault_platform_coredump_storage_write(0, &clear_byte, sizeof(clear_byte));
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

  // save coredump to file system on bootup if present
  size_t total_size = 0;
  if (memfault_coredump_has_valid_coredump(&total_size)) {
    MEMFAULT_LOG_INFO("boot: coredump present! size=%d", total_size);

    // overwrite coredump file
    memfault_clear_coredump_chunk_files();
    memfault_save_coredump_chunks();
  } else {MEMFAULT_LOG_INFO("boot: coredump NOT present!");}

  // clear memfault chunk files
  memfault_clear_event_chunk_files();

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

void vApplicationMallocFailedHook(void) {
  MEMFAULT_ASSERT(0);
}

static void memfault_clear_event_chunk_files(void)
{
	// recursively remove all files in "memfault/chunks"
	int argc = 2;
	char *argv[] = {NULL, "memfault/chunks"};

	fs_rm_all_command(argc, argv);
}

static void memfault_clear_coredump_chunk_files(void)
{
	// recursively remove all files in "memfault/coredump"
	int argc = 2;
	char *argv[] = {NULL, "memfault/coredump"};

	fs_rm_all_command(argc, argv);
}

// only store one coredump at a time, no overwriting
static void memfault_save_coredump_chunks(void)
{
	size_t total_size;

	static int32_t chunk_num = 0;
	static FIL file;
	static FRESULT result;

	// make sure dir exists
	f_mkdir("memfault/");
	f_mkdir("memfault/coredump");

	// buffer to copy chunk data into
	uint8_t buf[MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN];
	size_t buf_len = sizeof(buf);

	// set active sources to events and logs
	memfault_packetizer_set_active_sources(kMfltDataSourceMask_Coredump);
	while (memfault_packetizer_get_chunk(buf, &buf_len))
	{
	  // if the buffer is too small to packetize, memfault doesn't throw an error so break here
	  if (buf_len == 0)
	  {
		  break;
	  }

	  //save single chunk to memfault/chunks dir as single file
	  char log_fnum_buf[15];
	  snprintf ( log_fnum_buf, sizeof(log_fnum_buf), "%d", chunk_num );

	  // create log file name
	  char log_fname[128];
	  size_t log_fsize = 0;
	  log_fsize = str_append2(log_fname, log_fsize, "memfault/coredump"); // directory
	  log_fsize = str_append2(log_fname, log_fsize, "/");               // path separator
	  log_fsize = str_append2(log_fname, log_fsize, "chunk");           // file name
	  log_fsize = str_append2(log_fname, log_fsize, log_fnum_buf);      // sequence number
	  log_fsize = str_append2(log_fname, log_fsize, ".bin");            // suffix

	  // create and write chunk file
	  result = f_open(&file, log_fname, FA_CREATE_NEW | FA_WRITE);
	  if (result != FR_OK)
	  {
		printf("Failed to open file %s! Error: %d\n", log_fname, result);
		break;
	  }

	  UINT bytesWritten;
	  result = f_write(&file, buf, buf_len, &bytesWritten);
	  if (result != FR_OK)
	  {
		printf("Failed to write to file %s! Error: %d\n", log_fname, result);
		f_close(&file);
		break;
	  }

	  result = f_close(&file);
	  if (result != FR_OK)
	  {
		printf("Failed to close file %s! Error: %d\n", log_fname, result);
		break;
	  }
	  printf("Data saved to file %s successfully!\n", log_fname);
	  chunk_num++;
	}

	printf("No data left to save\n");
	memfault_packetizer_set_active_sources(kMfltDataSourceMask_All);
}
