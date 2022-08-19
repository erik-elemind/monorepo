#ifndef __COMMANDS_H__
#define __COMMANDS_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef enum command_permission {
  P_SHELL = 0x01,
  P_BLE = 0x02,
  P_SCRIPT = 0x04,
  P_ALL = P_SHELL | P_BLE | P_SCRIPT
} command_permission;

typedef struct {
  command_permission permission;
  char *name;
  void (*function )(int argc, char **argv);
  char *description;
} shell_command_t;

// Maximum number of commands
#define MAX_COMMANDS 256 //was 128,140,192

// The commands are defined elsewhere:
extern const shell_command_t commands[];
extern const int commands_count;


#ifdef __cplusplus
}
#endif

#endif //__COMMANDS_H__
