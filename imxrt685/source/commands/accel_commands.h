#ifndef ACCEL_COMMANDS_H
#define ACCEL_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

void accel_stop_command(int argc, char **argv);
void accel_start_sample_command(int argc, char **argv);
void accel_start_motion_detect_command(int argc, char **argv);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif // ACCEL_COMMANDS_H
