#ifndef __COMMANDS_H__
#define __COMMANDS_H__


#ifdef __cplusplus
extern "C" {
#endif

struct shell_command {
  char *name;
  void (*function )(int argc, char **argv);
  char *description;
};

// Maximum number of commands
#define MAX_COMMANDS 140 //was 128

// The commands are defined elsewhere:
extern struct shell_command commands[];
extern const int commands_count;


#ifdef __cplusplus
}
#endif

#endif //__COMMANDS_H__
