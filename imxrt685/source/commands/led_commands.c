#include "led_commands.h"

#include "led.h"

#include <stdio.h>
#include <stdlib.h>

#include "command_helpers.h"

void
led_state_command(int argc, char **argv) {
  CHK_ARGC(2, 2);
  uint32_t state = atol(argv[1]);

  printf("Setting led state to %ld\n", state);
  set_led_state(state);
}
