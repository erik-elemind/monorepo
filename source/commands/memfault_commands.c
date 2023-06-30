#include "memfault/components.h"
#include "cmsis_gcc.h"
#include "ff.h"
#include <stdio.h>
#include "nand_W25N04KW.h"
#include "command_helpers.h"
#include "fs_commands.h"
#include "system_monitor.h"

// Mock declarations
static void memfault_clear_event_chunk_files(void);
static void memfault_clear_coredump_chunk_files(void);
static void memfault_save_coredump_chunks(void);

// Test platform ports
void memfault_test_logging_command(int argc, char *argv[]) {
  MEMFAULT_LOG_DEBUG("Debug log!");
  MEMFAULT_LOG_INFO("Info log!");
  MEMFAULT_LOG_WARN("Warning log!");
  MEMFAULT_LOG_ERROR("Error log!");
}

void memfault_info_dump_command(int argc, char *argv[]) {
  memfault_build_info_dump();
  memfault_device_info_dump();
}

// Dump Memfault data collected to console/file
void memfault_test_export_command(int argc, char *argv[]) {
      memfault_data_export_dump_chunks(); // Send to console (base64 encoded)
}

// Runs a sanity test to confirm coredump port is working as expected
void memfault_test_coredump_storage_command(int argc, char *argv[]) {

  // Note: Coredump saving runs from an ISR prior to reboot so should
  // be safe to call with interrupts disabled.
	__disable_irq();
  memfault_coredump_storage_debug_test_begin();
  __enable_irq();

  memfault_coredump_storage_debug_test_finish();
}

// Test core SDK functionality
// Triggers an immediate heartbeat capture (instead of waiting for timer
// to expire)
void memfault_test_heartbeat_command(int argc, char *argv[]) {
  memfault_metrics_heartbeat_debug_trigger();
}

void memfault_test_trace_command(int argc, char *argv[]) {
  MEMFAULT_TRACE_EVENT_WITH_LOG(critical_error, "A test error trace!");
}

//! Trigger a user initiated reboot and confirm reason is persisted
void memfault_test_reboot_command(int argc, char *argv[]) {
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, NULL);
  memfault_platform_reboot();
}

// Test different crash types where a coredump should be captured
void memfault_test_assert_command(int argc, char *argv[]) {
  MEMFAULT_ASSERT(0);
  MEMFAULT_LOG_ERROR("test assert failed!"); // should never get here
}

void memfault_test_fault_command(int argc, char *argv[]) {
  void (*bad_func)(void) = (void *)0xEEEEDEAD;
  bad_func();
  MEMFAULT_LOG_ERROR("test fault failed!"); // should never get here
}

void memfault_test_hang_command(int argc, char *argv[]) {
  stop_wwdt_feed_timer();
  while (1) {}
  MEMFAULT_LOG_ERROR("test hang failed!"); // should never get here
}

void memfault_check_coredump(int argc, char *argv[]) {
	size_t total_size;
	if(memfault_coredump_has_valid_coredump(&total_size)) {
		MEMFAULT_LOG_INFO("Valid coredump found!");
		memfault_data_export_dump_chunks();
	}
	else {
		MEMFAULT_LOG_INFO("No valid coredump found :( ");
	}
}

void memfault_check_coredump_nand(int argc, char *argv[]) {
	static uint8_t buf[NAND_PAGE_SIZE];
	memset(buf, 0x00, sizeof(buf));
	memfault_platform_coredump_storage_read(0, buf, sizeof(buf));
	hex_dump(buf, NAND_PAGE_SIZE+16, 0); // one extra row to see all
}

void memfault_test_coredump_nand_write(int argc, char *argv[]) {
	static uint8_t data[NAND_PAGE_SIZE] = {0};
	memset(data, 0xE4, sizeof(data));
	memfault_platform_coredump_storage_write(0, data, sizeof(data));
}

void memfault_test_coredump_nand_erase(int argc, char *argv[]) {
	memfault_platform_coredump_storage_erase(0, NAND_BLOCK_SIZE);
}

void memfault_save_eventlog_chunks_command(int argc, char *argv[]) {
	memfault_save_eventlog_chunks();
}

void memfault_clear_event_chunk_files_command(int argc, char *argv[]) {
	memfault_clear_event_chunk_files();
}

void memfault_save_coredump_chunks_command(int arg, char *argv[]) {
	memfault_save_coredump_chunks();
}

void memfault_clear_coredump_chunk_files_command(int arg, char *argv[]) {
	memfault_clear_coredump_chunk_files();
}

void memfault_save_eventlog_chunks(void)
{
	  static int32_t chunk_num = 0;
	  static FIL file;
	  static FRESULT result;

	  // make sure dir exists
	  f_mkdir("memfault/");
	  f_mkdir("memfault/chunks");

	  // buffer to copy chunk data into
	  uint8_t buf[MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN];
	  size_t buf_len = sizeof(buf);

	  // set active sources to events and logs
	  memfault_packetizer_set_active_sources((kMfltDataSourceMask_Event | kMfltDataSourceMask_Log));
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
      log_fsize = str_append2(log_fname, log_fsize, "memfault/chunks"); // directory
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

// Test mocks for memfault_platform_port.c
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

static void memfault_save_coredump_chunks(void)
{
	size_t total_size;
	if(memfault_coredump_has_valid_coredump(&total_size)) {
		  MEMFAULT_LOG_INFO("Valid coredump found!");

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
	else {
		MEMFAULT_LOG_INFO("No valid coredump found :( ");
	}
}

