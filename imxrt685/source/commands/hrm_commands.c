#include "hrm.h"
#include "max86140.h"
#include <stdio.h>
#include "loglevels.h"

static const char *TAG = "hrm_commands";	// Logging prefix for this module

void hrm_off(int argc, char **argv){
	// TODO: Complete HRM driver and implementation of this command.
	LOGW(TAG, "%s command is not yet implemented.", argv[0]);
#if 0
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  max86140_stop();
#endif
}

void hrm_on(int argc, char **argv){
	// TODO: Complete HRM driver and implementation of this command.
	LOGW(TAG, "%s command is not yet implemented.", argv[0]);
#if 0
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  max86140_start();
#endif
}

void hrm_test(int argc, char **argv){

	if (argc != 1) {
	    printf("Error: %s takes no arguments\n", argv[0]);
	    return;
	}

	max86140_test();
}

void hrm_test_read(int argc, char **argv){
	  if (argc != 2) {
	    printf("Error: Missing register address\n");
	    printf("Usage: %s <reg address>\n", argv[0]);
	    return;
	  }

	  // Get address
	  uint8_t address;
	  if (!parse_uint8_arg(argv[0], argv[1], &address)) {
	    return;
	  }


	  max86140_test_read(address);

}

void hrm_test_write(int argc, char **argv){
	  if (argc != 3) {
	    printf("Error: Wrong format of command\n");
	    printf("Usage: %s <reg address> <value>\n", argv[0]);
	    return;
	  }

	  // Get address
	  uint8_t address;
	  if (!parse_uint8_arg(argv[0], argv[1], &address)) {
	    return;
	  }

	  // Get value
	  uint8_t value;
	  if (!parse_uint8_arg(argv[0], argv[2], &value)) {
		return;
	  }

	  printf("write reg %d with %d", address, value);
	  max86140_test_write(address, value);

}
