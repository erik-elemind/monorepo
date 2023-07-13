#ifndef HRM_COMMANDS_H
#define HRM_COMMANDS_H

#include "command_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

void hrm_off(int argc, char **argv);
void hrm_on(int argc, char **argv);
void hrm_test(int argc, char **argv);
void hrm_test_read(int argc, char **argv);
void hrm_test_write(int argc, char **argv);

// End: Tell C++ compiler to include this C header.
#ifdef __cplusplus
}
#endif

#endif // HRM_COMMANDS_H
