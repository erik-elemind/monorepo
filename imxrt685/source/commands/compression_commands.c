/*
 * compression_commands.c
 *
 *  Created on: Mar 13, 2022
 *      Author: DavidWang
 */


#include "compression_commands.h"
#include "compression_test_task.h"

void comp_test1_command(int argc, char **argv){
  compression_test_task_test1();
}

void comp_test2_command(int argc, char **argv){
  compression_test_task_test2();
}

void comp_test3_command(int argc, char **argv){
  compression_test_task_test3();
}

void comp_test4_command(int argc, char **argv){
  //compression_test4(); //TODO: Can we delete this?
}
