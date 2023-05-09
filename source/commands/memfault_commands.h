#ifndef MEMFAULT_COMMANDS_H
#define MEMFAULT_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void memfault_test_logging_command(int argc, char *argv[]);
void memfault_test_export_command(int argc, char *argv[]);
void memfault_test_coredump_storage_command(int argc, char *argv[]);
void memfault_test_heartbeat_command(int argc, char *argv[]);
void memfault_test_trace_command(int argc, char *argv[]);
void memfault_test_reboot_command(int argc, char *argv[]);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif // ACCEL_COMMANDS_H
