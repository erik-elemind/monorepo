#include "hrm.h"
#include <stdio.h>

void hrm_off(int argc, char **argv){
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  hrm_event_turn_off();
}

void hrm_on(int argc, char **argv){
  if (argc != 1) {
    printf("Error: %s takes no arguments\n", argv[0]);
    return;
  }

  hrm_event_turn_on();
}