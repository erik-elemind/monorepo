#include "memfault/components.h"
#include "cmsis_gcc.h"
#include "ff.h"
#include <stdio.h>

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
//	  static int chunk_num = 0;
//	  FIL file;
//	  FRESULT result;
//
//	  // buffer to copy chunk data into
//	  uint8_t buf[MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN];
//	  size_t buf_len = sizeof(buf);
//
//	  // make sure dir exists
//	  f_mkdir("memfault/");
//	  f_mkdir("memfault/chunks");
//
//	  while (memfault_packetizer_get_chunk(buf, &buf_len))
//	  {
//		//save single chunk to memfault/chunks dir as single file
//		char log_fnum_buf[15];
//		snprintf ( log_fnum_buf, sizeof(log_fnum_buf), "%d", chunk_num );
//
//		// create log file name
//		char log_fname[128];
//		size_t log_fsize = 0;
//		log_fsize = str_append2(log_fname, log_fsize, "memfault/chunks"); // directory
//		log_fsize = str_append2(log_fname, log_fsize, "/");               // path separator
//		log_fsize = str_append2(log_fname, log_fsize, "chunk");                      // file name
//		log_fsize = str_append2(log_fname, log_fsize, log_fnum_buf);                        // sequence number
//		log_fsize = str_append2(log_fname, log_fsize, ".bin");                       // suffix
//		printf("%s\n", log_fname);
//		// create and write chunk file
//		result = f_open(&file, log_fname, FA_CREATE_NEW | FA_WRITE);
//		if (result != FR_OK)
//		{
//			printf("Failed to open file! Error: %d\n", result);
//			break;
//		}
//
//		UINT bytesWritten;
//		result = f_write(&file, buf, buf_len, &bytesWritten);
//		if (result != FR_OK)
//		{
//			printf("Failed to write to file! Error: %d\n", result);
//			break;
//		}
//
//		result = f_close(&file);
//		if (result != FR_OK)
//		{
//			printf("Failed to close file! Error: %d\n", result);
//			break;
//		}
//		printf("Data saved to file %s successfully!\n", log_fname);
//		chunk_num++;
//	  }
//	  printf("No data left to save\n");
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
	static uint8_t buf[2048];
	memset(buf, 0x00, sizeof(buf));
	memfault_platform_coredump_storage_read(0, buf, sizeof(buf));
	hex_dump(buf, 2048+16, 0);
}

void memfault_test_coredump_nand_write(int argc, char *argv[]) {
	static uint8_t data[2048] = {0};
	memset(data, 0xE4, sizeof(data));
	memfault_platform_coredump_storage_write(0, data, sizeof(data));
}

void memfault_test_coredump_nand_erase(int argc, char *argv[]) {
	memfault_platform_coredump_storage_erase(0, 2048);
}
