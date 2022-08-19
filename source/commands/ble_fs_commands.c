/*
 * ble_fs_commands.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for FS interface.
 *
 */
#include <ble_shell.h>
#include <ymodem.h>
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "fsl_gpio.h"

// TODO: update for FATFS
#include "ff.h"
#include "fatfs_utils.h"


#include "board_config.h"
#include "command_helpers.h"

#include "utils.h"

#include "binary_interface_inst.h"
#include "interpreter.h"

#define CBUF_SIZE 100
char cbuf[CBUF_SIZE];
int cnum;

#define TENDSTR "\n"

#define MAX_PATH_LENGTH 128

static
int CountObjectUnder(const char *path)
{
  int count = 0;
  DIR dir;
  FILINFO finfo;

  FRESULT result = f_opendir(&dir, path);
  if (FR_OK == result) {
    result = f_readdir(&dir, &finfo);
    // FatFS indicates end of directory with a null name
    while (FR_OK == result && 0 != finfo.fname[0]) {
      count++;
      result = f_readdir(&dir, &finfo);
    }
    f_closedir(&dir);
  }
  return count;
}

void
ble_fs_ls_command(int argc, char *argv[])
{

  DIR dir;
  FILINFO finfo;
  int count = 0;
  char buf[MAX_PATH_LENGTH+2];
  const char *name = "/";
  char *sub;

  CHK_ARGC(1, 2);

  if (argc > 1) {
    name = argv[1];
  }

  FRESULT result = f_opendir(&dir, name);
  if (FR_OK != result) {
    cnum = snprintf(cbuf,CBUF_SIZE,"Can't open '%s' for list\n", name);
    bin_itf_send_uart_command(cbuf, cnum);
  }
  else {
    cnum = snprintf(cbuf,CBUF_SIZE,"------name-----------size---------serial-----" TENDSTR);
    bin_itf_send_uart_command(cbuf, cnum);

    result = f_readdir(&dir, &finfo);
    // FatFS indicates end of directory with a null name
    while (FR_OK == result && 0 != finfo.fname[0]) {
      cnum = snprintf(cbuf,CBUF_SIZE,"%9s", finfo.fname);
      bin_itf_send_uart_command(cbuf, cnum);

      strncpy(buf, name, sizeof(buf)-1);
      buf[sizeof(buf)-1] = '\0';
      sub = buf;
      if (name[strlen(name)-1] != '/') {
        sub = strcat(buf, "/");
      }
      sub = strcat(sub, finfo.fname);
      if (finfo.fattrib & AM_DIR) {
        sub = strcat(sub, "/");
        cnum = snprintf(cbuf,CBUF_SIZE,"/  \t<%8d>", CountObjectUnder(sub));
        bin_itf_send_uart_command(cbuf, cnum);

      }
      else {
        cnum = snprintf(cbuf,CBUF_SIZE,"   \t %8ld ", finfo.fsize);
        bin_itf_send_uart_command(cbuf, cnum);

      }
      cnum = snprintf(cbuf,CBUF_SIZE,"\t%6d" TENDSTR, 0);
      bin_itf_send_uart_command(cbuf, cnum);

      count++;
      result = f_readdir(&dir, &finfo);
    }

        f_closedir(&dir);

    cnum = snprintf(cbuf,CBUF_SIZE,"Total: %d objects." TENDSTR, count);
    bin_itf_send_uart_command(cbuf, cnum);

  }
}

void
ble_fs_rm_command(int argc, char *argv[])
{
  const char *name = NULL;
  FRESULT result;

  CHK_ARGC(2, 2);

  name = argv[1];

  result = f_unlink(name);
  if (FR_OK == result) {
    cnum = snprintf(cbuf,CBUF_SIZE,"Delete '%s' succ.\n", name);
    bin_itf_send_uart_command(cbuf, cnum);
  }
  else {
    cnum = snprintf(cbuf,CBUF_SIZE,"Delete '%s' fail!\n", name);
    bin_itf_send_uart_command(cbuf, cnum);
  }
}

void
ble_fs_mkdir_command(int argc, char *argv[])
{
  const char *name;

  CHK_ARGC(2, 2);

  name = argv[1];
  f_mkdir(name);
}


void
ble_fs_mv_command(int argc, char *argv[])
{
  const char *oldname;
  const char *newname;

  CHK_ARGC(3, 3);

  oldname = argv[1];
  newname = argv[2];

  f_rename(oldname, newname);
}


void
ble_fs_ymodem_recv_command(int argc, char *argv[])
{
  // stop therapy
  interpreter_event_stop_script(false);

  CHK_ARGC(1, 1);

//  printf("Please start YMODEM sending of the file in your terminal program.\n"
//         "Starting YMODEM receive in 1 second...\n\n");

  // Wait 1 s for sender to get set up
  vTaskDelay((1000)/portTICK_PERIOD_MS);

  size_t size = ymodem_receive_file(&ble_interface);
//  size_t size = file_transfer_recv_file_wait("",0,FILE_TRANSFER_MODE_UART);
  if (size <= 0) {
//    printf("Error receiving file!\n");
  }
}

void
ble_fs_ymodem_send_command(int argc, char *argv[])
{
  // stop therapy
  interpreter_event_stop_script(false);
  // todo: wait for interpreter to finish.

  CHK_ARGC(2, 2);

  const char* name = argv[1];

//  printf("Please start YMODEM receiving in your terminal program.\n"
//         "Starting YMODEM send of file %s in 1 second...\n\n", name);

  // Wait 1 s for receiver to get set up
  vTaskDelay((1000)/portTICK_PERIOD_MS);

  int ret = ymodem_send_file(&ble_interface, name);
//  int ret = file_transfer_send_file_wait(name, strlen(name), FILE_TRANSFER_MODE_UART);

//  debug_uart_puts("ble_fs_ymodem_send_command 0");

  if (ret < 0) {
//    debug_uart_puts("ble_fs_ymodem_send_command 1");

//    printf("Error: YMODEM transfer failed!\n");
  }else{

//    debug_uart_puts("ble_fs_ymodem_send_command 2");
//    ymodem_end_session(&ble_interface);
  }

}
