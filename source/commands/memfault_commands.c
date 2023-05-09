#include "memfault/components.h"
#include "cmsis_gcc.h"
#include <stdio.h>

void memfault_test_logging_command(int argc, char *argv[]) {
  MEMFAULT_LOG_DEBUG("Debug log!");
  MEMFAULT_LOG_INFO("Info log!");
  MEMFAULT_LOG_WARN("Warning log!");
  MEMFAULT_LOG_ERROR("Error log!");
}

// Dump Memfault data collected to console
void memfault_test_export_command(int argc, char *argv[]) {
  memfault_data_export_dump_chunks();
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
