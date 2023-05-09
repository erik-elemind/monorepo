#include "memfault/components.h"
#include <stdio.h>

int memfault_test_logging_command(int argc, char *argv[]) {
  MEMFAULT_LOG_DEBUG("Debug log!");
  MEMFAULT_LOG_INFO("Info log!");
  MEMFAULT_LOG_WARN("Warning log!");
  MEMFAULT_LOG_ERROR("Error log!");
  return 0;
}

// Dump Memfault data collected to console
int memfault_test_export_command(int argc, char *argv[]) {
  memfault_data_export_dump_chunks();
  return 0;
}
