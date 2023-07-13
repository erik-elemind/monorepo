/*
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Debug shell commands for filesystem interface.
 *
 */
#include <ble_shell.h>
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "fsl_gpio.h"

#include "ff.h"
#include "fatfs_utils.h"

#include "board_config.h"
#include "command_helpers.h"
#include "fs_commands.h"
#include "interpreter.h"
#include "utils.h"
#include "ymodem.h"
#include "syscalls.h"

#include "../zmodem/zmodem_transfer.h"
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
fs_write_command(int argc, char **argv)
{
  FIL file;
  FRESULT result;
  UINT bytes_written;

  printf("Starting \"fs_write_command\"...\n");

  CHK_ARGC(1, 1);

  result = f_open(&file, "/test", FA_CREATE_ALWAYS | FA_WRITE);
  if (FR_OK != result) {
    printf("f_open(): error: %d\n", result);
    return;
  }

  const char data[] = "0123456789ABCDEF";
  result = f_write(&file, data, sizeof(data), &bytes_written);
  if (FR_OK != result || bytes_written != sizeof(data)) {
    printf("f_write(): error: %d (%u bytes written)\n", result, bytes_written);
  }

  result = f_close(&file);
  if (FR_OK != result) {
    printf("f_close(): error: %d\n", result);
  }

  printf("Completed \"fs_write_command\".\n");
}

void
fs_read_command(int argc, char **argv)
{
  FRESULT result;
  FIL file;
  UINT bytes_read;
  const char *name = "/test";

  CHK_ARGC(1, 2);

  if (argc > 1)
  {
	  name = argv[1];
  }

  result = f_open(&file, name, FA_READ);
  if (FR_OK != result) {
    printf("f_open(): error: %d\n", result);
  }

  char data[20];
  result = f_read(&file, data, sizeof(data), &bytes_read);
  if (FR_OK != result) {
    printf("f_read(): error: %d (%u bytes read)\n", result, bytes_read);
  }

  hex_dump((uint8_t*)data, bytes_read, 0);

  result = f_close(&file);
  if (FR_OK != result) {
    printf("f_close(): error: %d\n", result);
  }
}

void
fs_format_command(int argc, char **argv)
{
  CHK_ARGC(1, 3);
  format_drive();
}

void
fs_touch_command(int argc, char *argv[])
{
  FRESULT result;
  FIL file;

  CHK_ARGC(2, 2);

  const char *name = argv[1];
  result = f_open(&file, name, FA_OPEN_ALWAYS);
  if (FR_OK != result) {
    printf("Create %s fail, err: %d\n", name, result);
  }
  else {
    printf("Create %s succ.\n", name);
    f_close(&file);
  }
}

void
fs_ls_command(int argc, char *argv[])
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
    printf("Can't open '%s' for list: %d\n", name, result);
  }
  else {
    printf("------name-----------size---------serial-----" TENDSTR);
    result = f_readdir(&dir, &finfo);
    // FatFS indicates end of directory with a null name
    while (FR_OK == result && 0 != finfo.fname[0]) {
      printf("%9s", finfo.fname);
      strncpy(buf, name, sizeof(buf)-1);
      buf[sizeof(buf)-1] = '\0';
      sub = buf;
      if (name[strlen(name)-1] != '/') {
        sub = strcat(buf, "/");
      }
      sub = strcat(sub, finfo.fname);
      if (finfo.fattrib & AM_DIR) {
        sub = strcat(sub, "/");
        printf("/  \t<%8d>", CountObjectUnder(sub));
      }
      else {
        printf("   \t %8ld ", finfo.fsize);
      }
      printf("\t%6d" TENDSTR, 0);
      count++;
      result = f_readdir(&dir, &finfo);
    }

    f_closedir(&dir);

    printf("Total: %d objects." TENDSTR, count);
  }
}

void
fs_rm_command(int argc, char *argv[])
{
  const char *name = NULL;

  FRESULT result;

  CHK_ARGC(2, 2);

  name = argv[1];

  result = f_unlink(name);
  if (FR_OK == result) {
    printf("Delete '%s' succ.\n", name);
  }
  else {
    printf("Delete '%s' fail: %u!\n", name, result);
  }
}

static int
fs_rm_recursive(const char* path){
  DIR dir;
  FILINFO finfo;
  char buf[MAX_PATH_LENGTH+2];
  const char *name = "/";
  bool is_root;
  char *sub;
  FRESULT result;

  // If a path argument has been passed in, use it.
  if(path != NULL && path[0] != '\0'){
    name = path;
  }
  // Check if the path is root, which needs to be handled differently.
  is_root = name[0]=='/' && name[1]=='\0';


  bool is_directory = true;
  if (is_root){
    is_directory = true;
  }else{
    result = f_stat(name, &finfo);
    if (FR_OK != result) {
      printf("Can't open '%s' for remove: %d\n", name, result);
      return -1;
    } else {
      is_directory = (finfo.fattrib & AM_DIR) != 0;
    }
  }

    if ( is_directory ) {
      FRESULT result = f_opendir(&dir, name);
      if (FR_OK != result) {
        printf("Can't open '%s' directory for remove\n", name);
        return -1;
      } else {
        // Remove the files in the directory
        result = f_readdir(&dir, &finfo);
        // FatFS indicates end of directory with a null name
        while (FR_OK == result && 0 != finfo.fname[0]) {
          // create name string for child file/folder
          strncpy(buf, name, sizeof(buf)-1);
          buf[sizeof(buf)-1] = '\0';
          sub = buf;
          if (name[strlen(name)-1] != '/') {
            sub = strcat(buf, "/");
          }
          sub = strcat(sub, finfo.fname);
          if (finfo.fattrib & AM_DIR) {
            sub = strcat(sub, "/");
          }
          // recursively delete child file/folder
          if (fs_rm_recursive(sub) != 0){
            return -1;
          }
          // move to next child file/folder
          result = f_readdir(&dir, &finfo);
        }
        f_closedir(&dir);
        // Remove the directory if it isn't root
        if(!is_root){
          if (f_unlink(name) != 0){
            return -1;
          }
        }
        return 0;
      }
    } else {
      return f_unlink(name);
    }

}

void
fs_rm_datalogs_command(int argc, char **argv){
  FRESULT result;
  DIR dir;
  FILINFO finfo;
  char buf[MAX_PATH_LENGTH+2];
  const char *datalog_name = "/datalogs";
  const char *usermetrics_name = "/user_metrics";
  const char *memfault_name = "/memfault";
  char *sub;
  int ret = 0;

  // remove datalogs
  result = f_opendir(&dir, datalog_name);
  if (FR_OK != result) {
    printf("Can't open '%s' directory\n", datalog_name);
  }
  else {
    result = f_readdir(&dir, &finfo);
    // FatFS indicates end of directory with a null name
    while (FR_OK == result && 0 != finfo.fname[0]) {
      strncpy(buf, datalog_name, sizeof(buf)-1);
      buf[sizeof(buf)-1] = '\0';
      sub = buf;
      if (datalog_name[strlen(datalog_name)-1] != '/') {
        sub = strcat(buf, "/");
      }
      sub = strcat(sub, finfo.fname);
      ret = f_unlink(sub);
      if (ret == 0) {
        printf("Delete '%s' succ.\n", sub);
      }
      else {
        printf("Delete '%s' fail!\n", sub);
      }
      // move to next entry
      result = f_readdir(&dir, &finfo);
    }

    f_closedir(&dir);
  }

  // remove user_metric files
  result = f_opendir(&dir, usermetrics_name);
  if (FR_OK != result) {
    printf("Can't open '%s' directory\n", usermetrics_name);
  }
  else {
    result = f_readdir(&dir, &finfo);
    // FatFS indicates end of directory with a null name
    while (FR_OK == result && 0 != finfo.fname[0]) {
      strncpy(buf, usermetrics_name, sizeof(buf)-1);
      buf[sizeof(buf)-1] = '\0';
      sub = buf;
      if (usermetrics_name[strlen(usermetrics_name)-1] != '/') {
        sub = strcat(buf, "/");
      }
      sub = strcat(sub, finfo.fname);
      ret = f_unlink(sub);
      if (ret == 0) {
        printf("Delete '%s' succ.\n", sub);
      }
      else {
        printf("Delete '%s' fail!\n", sub);
      }
      // move to next entry
      result = f_readdir(&dir, &finfo);
    }

    f_closedir(&dir);
  }

  // remove memfault files
  result = f_opendir(&dir, memfault_name);
  if (FR_OK != result) {
    printf("Can't open '%s' directory\n", memfault_name);
  }
  else {
    result = f_readdir(&dir, &finfo);
    // FatFS indicates end of directory with a null name
    while (FR_OK == result && 0 != finfo.fname[0]) {
      strncpy(buf, memfault_name, sizeof(buf)-1);
      buf[sizeof(buf)-1] = '\0';
      sub = buf;
      if (memfault_name[strlen(memfault_name)-1] != '/') {
        sub = strcat(buf, "/");
      }
      sub = strcat(sub, finfo.fname);
      ret = f_unlink(sub);
      if (ret == 0) {
        printf("Delete '%s' succ.\n", sub);
      }
      else {
        printf("Delete '%s' fail!\n", sub);
      }
      // move to next entry
      result = f_readdir(&dir, &finfo);
    }

    f_closedir(&dir);
  }
}

void
fs_rm_all_command(int argc, char *argv[])
{
  int ret = 0;
  char *name = "/";

  CHK_ARGC(1, 2);

  if (argc > 1) {
    name = argv[1];
  }

  ret = fs_rm_recursive(name);
  if (ret == 0) {
    printf("Delete '%s' succ.\n", name);
  }
  else {
    printf("Delete '%s' fail!\n", name);
  }
}

void
fs_mkdir_command(int argc, char *argv[])
{
  const char *name;
  FRESULT result;

  CHK_ARGC(2, 2);

  name = argv[1];
  result = f_mkdir(name);
  if (FR_OK == result) {
    printf("Make directory %s succ.\n", name);
  }
  else {
    printf("Make directory %s fail, err: %d\n", name, result);
  }
}

void
fs_mv_command(int argc, char *argv[])
{
  const char *oldname;
  const char *newname;
  FRESULT result;

  CHK_ARGC(3, 3);

  oldname = argv[1];
  newname = argv[2];

  result = f_rename(oldname, newname);
  if (FR_OK == result) {
    printf("Rename from '%s' to '%s' succ.\n", oldname, newname);
  }
  else {
    printf("Rename from '%s' to '%s' fail: %u\n", oldname, newname, result);
  }
}

void
fs_cp_command(int argc, char *argv[])
{
  const char *fn_src = argv[1];
  const char *fn_dst = argv[2];
  char buf[512];

  FRESULT result;
  FIL f_src, f_dst;
  UINT bytes_read, bytes_written;

  CHK_ARGC(3, 3);

  // open source and dest files
  // FA_CREATE_NEW means f_open() will fail w/ FR_EXIST if file exists.
  result = f_open(&f_src, fn_src, FA_READ);
  if (FR_OK != result) {
    printf("couldn't open file %s: %u\n", fn_src, result);
  }
  result = f_open(&f_dst, fn_dst, FA_WRITE | FA_CREATE_NEW);
  if (FR_OK != result) {
    printf("couldn't open file %s: %u\n", fn_dst, result);
  }

  // loop until the end of the source file
  while (0 == f_eof(&f_src)) {
    // read a chunk from the source file
    result = f_read(&f_src, buf, sizeof(buf), &bytes_read);
    if (FR_OK == result) {
      // write the chunk to the destination
      result = f_write(&f_dst, buf, bytes_read, &bytes_written);
      if (FR_OK != result || bytes_written != bytes_read) {
        printf("write file %s fail: %u\n", fn_dst, result);
        goto fail_ext;
      }
    }
    else {
      printf("read file %s fail: %u\n", fn_src, result);
      goto fail_ext;
    }
  }

  fail_ext:
  f_close(&f_src);
  f_close(&f_dst);
}

void
fs_cat_command(int argc, char *argv[])
{
  #if 1
  printf("not implemented\n");
  #else
  int fd;
  const char *name = NULL;
  char buf[100];
  int start = 0, size = 0, printed = 0, n, len;

  CHK_ARGC(2, 4);

  name = argv[1];

  if ((fd = uffs_open(name, UO_RDONLY)) < 0) {
    printf("Can't open %s\n", name);
    goto fail;
  }

  if (argc > 2) {
    start = strtol(argv[2], NULL, 10);
    if (argc > 3) {
      size = strtol(argv[3], NULL, 10);
    }
  }

  if (start >= 0) {
    uffs_seek(fd, start, USEEK_SET);
  }
  else {
    uffs_seek(fd, -start, USEEK_END);
  }

  while (uffs_eof(fd) == 0) {
    len = uffs_read(fd, buf, sizeof(buf) - 1);
    if (len == 0) {
      break;
    }
    if (len > 0) {
      if (size == 0 || printed < size) {
        n = (size == 0 ? len : (size - printed > len ? len : size - printed));
        buf[n] = 0;
        printf("%s", buf);
        printed += n;
      }
      else {
        break;
      }
    }
  }
  printf(TENDSTR);
  uffs_close(fd);

 fail:
  return;

  #endif
}

void
fs_info_command(int argc, char *argv[])
{
  // Get free space in filesystem.
  // Based on example from here:
  // http://elm-chan.org/fsw/ff/doc/getfree.html
  DWORD fre_clust, fre_sect, tot_sect, fre_bytes, tot_bytes;
  FATFS* fs;
  FRESULT result;

  result = f_getfree("0:", &fre_clust, &fs);
  if (result) {
    printf("f_getfree() returned %u\n", result);
    return;
  }
  /* Get total sectors and free sectors */
  tot_sect = (fs->n_fatent - 2) * fs->csize;
  fre_sect = fre_clust * fs->csize;
  /* Convert to bytes */
  tot_bytes = tot_sect * FF_MIN_SS;
  fre_bytes = fre_sect * FF_MIN_SS;

  printf("Cluster size [sectors]: %d\n", fs->csize);
  printf("Sector size [bytes]:    %d\n", FF_MAX_SS);
  printf("Total sectors:          %lu\n", tot_sect);
  printf("Total bytes:            %lu\n", tot_bytes);
  printf("Free sectors            %lu\n", fre_sect);
  printf("Free bytes              %lu\n", fre_bytes);
}

void
fs_ymodem_recv_command(int argc, char *argv[])
{
  // stop therapy
  interpreter_event_stop_script(false);
  // todo: wait for interpreter stop?

  CHK_ARGC(1, 1);

  printf("Please start YMODEM sending of the file in your terminal program.\n"
         "Starting YMODEM receive in 1 second...\n\n");

  // Wait 1 s for sender to get set up
  vTaskDelay((1000)/portTICK_PERIOD_MS);

  size_t size = ymodem_receive_file(&uart_interface);
//  size_t size = file_transfer_recv_file_wait("",0,FILE_TRANSFER_MODE_UART);

  if (size <= 0) {
    printf("Error receiving file!\n");
  }
}

void
fs_ymodem_send_command(int argc, char *argv[])
{
  // stop therapy
  interpreter_event_stop_script(false);
  // todo: wait for interpreter stop?

  CHK_ARGC(2, 2);

  const char* name = argv[1];

  printf("Please start YMODEM receiving in your terminal program.\n"
         "Starting YMODEM send of file %s in 1 second...\n\n", name);

  // Wait 1 s for receiver to get set up
  vTaskDelay((1000)/portTICK_PERIOD_MS);

  uint32_t saved_write_loc = syscalls_get_write_loc();
  uint32_t saved_read_loc = syscalls_get_read_loc();

  int ret = ymodem_send_file(&uart_interface, name);
//  int ret = file_transfer_send_file_wait(name, strlen(name), FILE_TRANSFER_MODE_UART);

  syscalls_set_write_loc(saved_write_loc);
  syscalls_set_read_loc(saved_read_loc);

  if (ret < 0) {
    printf("Error: YMODEM transfer failed!\n");
  }else{

  }
}

void
fs_ymodem_recv_test_command(int argc, char *argv[])
{
  CHK_ARGC(1, 1);

  static const char create_filename[] = "/testYModemRecv.txt";
  static const int exp_abc_count = 100;
  static const int exp_abc_seq_size = 27;

  FRESULT result;
  FILINFO finfo;
  FIL file;

  printf("Starting YMODEM recv of file in 1 second...\n");
  // Wait 1 s for sender to get set up
  vTaskDelay((1000)/portTICK_PERIOD_MS);

  size_t size = ymodem_receive_file(&uart_interface);
//  size_t size = file_transfer_recv_file_wait("",0,FILE_TRANSFER_MODE_UART);
  if (size <= 0) {
    printf("Error: YMODEM recv file failed!\n");
    return;
  }

  result = f_stat(create_filename, &finfo);
  if(result){
    printf("Error: YMODEM f_stat() failed: %u \n", result);
    return;
  }

  long file_size = finfo.fsize;
  if(file_size != exp_abc_seq_size*exp_abc_count){
    printf("Error: YMODEM recv file has wrong number of bytes. \n");
    return;
  }

  // check the file contents!
  result = f_open(&file, create_filename, FA_READ);
  if(result){
    printf("Error: YMODEM f_open() failed for %s: %u \n", create_filename, result);
    return;
  }

  int abc_count = 0;
  char buffer[exp_abc_seq_size+1];
  while(true){
    memset(&buffer[0],0,exp_abc_seq_size+1);

    UINT bytes_read;
    result = f_read(&file, buffer, exp_abc_seq_size, &bytes_read);
    if(result){
      printf("Error: YMODEM f_read() returned %u\n", result);
    }else if(bytes_read==exp_abc_seq_size && strcmp("abcdefghijklmnopqrstuvwxyz\n",&buffer[0])==0 ){
      abc_count++;
    }else{
      if(abc_count != exp_abc_count){
        printf("Error: YMODEM recv mismatch at %d \n", abc_count);
        return;
      }else{
        // success!
        break;
      }
    }
  }

  printf("PASSED: YMODEM recv file test succeeded!\n");
  f_close(&file);
}

void
fs_ymodem_send_test_command(int argc, char *argv[])
{
  CHK_ARGC(1, 1);

  static const char create_filename[] = "/testYModemSend.txt";
  static const char send_filename[] = "testYModemSend.txt";
  static const char buffer[] = "abcdefghijklmnopqrstuvwxyz\n";
  static const int exp_abc_count = 100;
  static const int exp_abc_seq_size = sizeof(buffer)-1;

  FRESULT result;
  FILINFO finfo;
  FIL file;
  UINT bytes_written;

  // delete any existing file
  f_unlink(create_filename);

  // create the file "testYModemSend.txt"
  result = f_open(&file, create_filename, FA_CREATE_ALWAYS | FA_WRITE);
  if(result){
    printf("Error: YMODEM send failed to open %s: %u. \n", send_filename, result);
    return;
  }

  for(int i=0; i<exp_abc_count; i++){
    result = f_write(&file, buffer, exp_abc_seq_size, &bytes_written);
    if(result!=FR_OK || bytes_written!=exp_abc_seq_size){
      printf("Error: YMODEM send failed to write abc's to testYModemSend.txt. \n");
      return;
    }
  }

  // close the file
  f_close(&file);

  // check the file size
  result = f_stat(create_filename, &finfo);
  if(result){
    printf("Error: YMODEM f_stat() failed: %u \n", result);
    return;
  }

  long file_size = finfo.fsize;
  if(file_size != exp_abc_seq_size*exp_abc_count){
    printf("Error: YMODEM send file has wrong number of bytes. \n");
    return;
  }

  printf("Starting YMODEM send of file %s in 1 second...\n\n", send_filename);
  // Wait 1 s for receiver to get set up
  vTaskDelay((1000)/portTICK_PERIOD_MS);

  int ret = ymodem_send_file(&uart_interface, send_filename);
//  int ret = file_transfer_send_file_wait(send_filename, strlen(send_filename), FILE_TRANSFER_MODE_UART);
  if (ret < 0) {
    printf("Error: YMODEM send failed!\n");
  }
}


void
fs_zmodem_recv_command(int argc, char *argv[]) {
  // stop therapy
  interpreter_event_stop_script(false);
  // todo: wait for interpreter stop?

  CHK_ARGC(1, 1);
  debug_uart_puts("Please start ZMODEM sending of the file in your terminal program.\n"
      "Starting ZMODEM receive in 10 second...\n\n" );
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Please start ZMODEM sending of the file in your terminal program.\n"
      "Starting ZMODEM receive in 10 second...\n\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Please start ZMODEM sending of the file in your terminal program.\n"
       "Starting ZMODEM receive in 10 second...\n\n");
#endif


  // Wait 10 s for sender to get set up
  vTaskDelay((10000) / portTICK_PERIOD_MS);

  size_t size = zmodem_receive_file();
  if (size <= 0) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error receiving file!\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error receiving file!\r\n");
#endif
  }
}

void
fs_zmodem_send_command(int argc, char *argv[]) {
  // stop therapy
  interpreter_event_stop_script(false);
  // todo: wait for interpreter stop?

  CHK_ARGC(2, 2);

  char *name = argv[1];

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Please start ZMODEM receiving of the file in your terminal program.\n"
      "Starting ZMODEM send in 10 second...\n\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Please start ZMODEM receiving of the file in your terminal program.\n"
       "Starting ZMODEM send in 10 second...\n\n");
#endif
  // Wait 10 s for receiver to get set up
  vTaskDelay((10000) / portTICK_PERIOD_MS);

  int ret = zmodem_send_file(name);

  if (ret < 0) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: ZMODEM transfer failed!\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: ZMODEM transfer failed!\r\n");
#endif
  }
  else {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("ZMODEM transfer success!\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("ZMODEM transfer success!\r\n");
#endif
  }
}

void
fs_zmodem_recv_test_command(int argc, char *argv[]) {
  CHK_ARGC(1, 1);

  static const char create_filename[] = "/testZModemRecv.txt";
  static const int exp_abc_count = 100;
  static const int exp_abc_seq_size = 27;

  FRESULT result;
  FILINFO finfo;
  FIL file;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Starting ZMODEM recv of file in 10 second...\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Starting ZMODEM recv of file in 10 second...\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("Starting ZMODEM recv of file in 10 second...\r\n");
#endif
  // Wait 1 s for sender to get set up
  vTaskDelay((10000) / portTICK_PERIOD_MS);

  long size = zmodem_receive_file();
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("ZMODEM recv file success checking size and testing\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("ZMODEM recv file success checking size and testing\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("ZMODEM recv file success checking size and testing\r\n");
#endif
  if (size <= 0) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: ZMODEM recv file failed\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: ZMODEM recv file failed\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("Error: ZMODEM recv file failed\r\n");
#endif
    return;
  }

  result = f_stat(create_filename, &finfo);
  if (result) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: ZMODEM f_stat() failed!\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: ZMODEM f_stat() failed!\r\n");
#endif
    return;
  }

  long file_size = finfo.fsize;
  if (file_size != exp_abc_seq_size * exp_abc_count) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: ZMODEM recv file has wrong number of bytes\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: ZMODEM recv file has wrong number of bytes\r\n");
#endif
    return;
  }

  // check the file contents!
  result = f_open(&file, create_filename, FA_READ);
  if (result) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("file open failed!\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("file open failed!\r\n");
#endif
    return;
  }

  int abc_count = 0;
  char buffer[exp_abc_seq_size + 1];
  while (true) {
    memset(&buffer[0], 0, exp_abc_seq_size + 1);

    UINT bytes_read;
    result = f_read(&file, buffer, exp_abc_seq_size, &bytes_read);
    if (result) {
      printf("Error: ZMODEM f_read() returned %u\n", result);
    }
    else if (bytes_read == exp_abc_seq_size
        && strcmp("abcdefghijklmnopqrstuvwxyz\n", &buffer[0]) == 0) {
      abc_count++;
    }
    else {
      if (abc_count != exp_abc_count) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: ZMODEM recv mismatch\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: ZMODEM recv mismatch\r\n");
#endif
        return;
      }
      else {
        //success
        break;
      }
    }
  }
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("PASSED: ZMODEM recv file test succeeded!\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("PASSED: ZMODEM recv file test succeeded!\r\n");
#endif
  f_close(&file);
}

void
fs_zmodem_send_test_command(int argc, char *argv[]) {
  CHK_ARGC(1, 1);

  char create_filename[] = "/testZModemSend.txt";
  char send_filename[] = "testZModemSend.txt";
  char buffer[] = "abcdefghijklmnopqrstuvwxyz\n";
  int exp_abc_count = 1;
  int exp_abc_seq_size = sizeof(buffer) - 1;

  FRESULT result;
  FILINFO finfo;
  FIL file;
  UINT bytes_written;

  // delete any existing file
  f_unlink(create_filename);

  // create the file "testYModemSend.txt"
  result = f_open(&file, create_filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (result) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: ZMODEM send failed to open\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: ZMODEM send failed to open\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("Error: ZMODEM send failed to open\r\n");
#endif
    return;
  }

  for (int i = 0; i < exp_abc_count; i++) {
    result = f_write(&file, buffer, exp_abc_seq_size, &bytes_written);
    if (result != FR_OK || bytes_written != exp_abc_seq_size) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: ZMODEM send failed to write abc's to testZModemSend.txt.\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: ZMODEM send failed to write abc's to testZModemSend.txt.\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("Error: ZMODEM send failed to write abc's to testZModemSend.txt.\r\n");
#endif
      return;
    }
  }

  // close the file
  f_close(&file);

  // check the file size
  result = f_stat(create_filename, &finfo);
  if (result) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: fstat failed \r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: fstat failed. \r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("Error: fstat failed. \r\n");
#endif
    return;
  }

  long file_size = finfo.fsize;
  if (file_size != exp_abc_seq_size * exp_abc_count) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Error: ZMODEM send file has wrong number of bytes. \r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Error: ZMODEM send file has wrong number of bytes. \r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("Error: ZMODEM send file has wrong number of bytes. \r\n");
#endif
    return;
  }
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("Starting ZMODEM send of file in 10 second...\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("Starting ZMODEM send of file in 10 second...!\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("Starting ZMODEM send of file in 10 second...!\r\n");
#endif
  // Wait 10 s for receiver to get set up
  vTaskDelay((10000) / portTICK_PERIOD_MS);


  int ret = zmodem_send_file(send_filename);
  if (ret < 0) {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("PASSED: ZMODEM transfer failed\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("PASSED: ZMODEM transfer failed!\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("PASSED: ZMODEM transfer failed!\r\n");
#endif
  }
  else {
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("PASSED: ZMODEM transfer success\r\n" );
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("PASSED: ZMODEM transfer success!\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("PASSED: ZMODEM transfer success!\r\n");
#endif
  }
}


enum {
  FS_TEST_SPEED_CHUNK_SIZE = 4096,
//  FS_TEST_SPEED_CHUNK_SIZE = 512,
  FS_TEST_SPEED_NUM_CHUNKS = 256,
  //FS_TEST_SPEED_NUM_CHUNKS = 128,
  FS_TEST_SPEED_INDICATE_CHUNKS = 1
  //  FS_TEST_SPEED_NUM_CHUNKS = 4096,
  //FS_TEST_SPEED_INDICATE_CHUNKS = 64
};



#if (defined(ENABLE_FS_TEST_COMMANDS) && (ENABLE_FS_TEST_COMMANDS > 0U))

void
fs_test_speed_command(int argc, char *argv[])
{
  FRESULT result;
  FIL file;
  UINT bytes_written;
  CHK_ARGC(1, 1);

  printf("Starting \"fs_test_speed_command\"...\n");

  static const char test_filename[] = "/fs_test_speed";

#if 1
  // Allocate data buffer
  uint8_t* buffer = malloc(FS_TEST_SPEED_CHUNK_SIZE);

  if (buffer == NULL) {
    printf("Error: Unable to allocate buffer!\n");
    return;
  }
#else
  uint8_t buffer[FS_TEST_SPEED_CHUNK_SIZE];
#endif

  // Remove test file (if it exists)
  f_unlink(test_filename);

  // Write 16 MB (64 flash blocks) file
  result = f_open(&file, test_filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (FR_OK != result) {
    // File open error--bail out
    printf("Error: unable to open file '%s' for writing: %d\n", test_filename, result);
    goto fs_test_speed_command_exit;
  }

  TickType_t last_ticks = xTaskGetTickCount();
  TickType_t t0 = 0;
  TickType_t t1 = 0;
  TickType_t t_longest = 0;

  for (uint32_t chunk = 0; chunk <= FS_TEST_SPEED_NUM_CHUNKS; chunk++) {

    t0 = xTaskGetTickCount();

    if ((chunk % FS_TEST_SPEED_INDICATE_CHUNKS) == 0) {
      // Print block indicator (erase spinner, space after for spinner)
      printf(".");
      fflush(stdout);
    }

    if ((chunk % (FS_TEST_SPEED_INDICATE_CHUNKS * 80) ) == 0) {
      printf("\n");  // Don't scroll off the end
      fflush(stdout);
    }

    // Populate buffer
    uint16_t data = (uint16_t)chunk;
    for (int i = 0; i < FS_TEST_SPEED_CHUNK_SIZE; i += sizeof(data)) {
      *(uint16_t*)(buffer + i) = data;
    }

    // Write data
    //printf("fs_test_speed_command 0\n");

    result = f_write(&file, buffer, FS_TEST_SPEED_CHUNK_SIZE, &bytes_written);
    if (FR_OK != result || bytes_written < FS_TEST_SPEED_CHUNK_SIZE) {
      printf("\nWrite error: %d (bytes: %u)\n", result, bytes_written);
      goto fs_test_speed_command_exit;
    }

    //printf("fs_test_speed_command 1\n");

    t1 = xTaskGetTickCount();
    if ((t1 - t0) > t_longest) {
      t_longest = (t1 - t0);
    }
  }

  f_sync(&file);
  f_close(&file);

  // Print write speed (in bytes/s)
  {
    TickType_t current_ticks = xTaskGetTickCount();
    TickType_t tick_delta = current_ticks - last_ticks;
    uint32_t ms_delta = tick_delta * portTICK_PERIOD_MS;
    uint32_t bytes = FS_TEST_SPEED_NUM_CHUNKS * FS_TEST_SPEED_CHUNK_SIZE;
    double kbytes_per_s = (double)bytes/ms_delta;
    printf("\nWrite speed: %0.2f KB/s\n", kbytes_per_s);
    printf("\nSlowest chunk write: %d ms\n", (int)t_longest);
  }

  // Read and verify file
  result = f_open(&file, test_filename, FA_READ);
  if (FR_OK != result) {
    // File open error--bail out
    printf("Error: unable to open file '%s' for reading\n", test_filename);
    goto fs_test_speed_command_exit;
  }

  last_ticks = xTaskGetTickCount();
  for (uint32_t chunk = 0; chunk <= FS_TEST_SPEED_NUM_CHUNKS; chunk++) {

    if ((chunk % FS_TEST_SPEED_INDICATE_CHUNKS) == 0) {
      // Print block indicator (erase spinner, space after for spinner)
      printf("\b. ");
    }

    // Read data
    result = f_read(&file, buffer, FS_TEST_SPEED_CHUNK_SIZE, &bytes_written);
    if (FR_OK != result && bytes_written < FS_TEST_SPEED_CHUNK_SIZE) {
      printf("\nRead error: %d (bytes: %u)\n", result, bytes_written);
      goto fs_test_speed_command_exit;
    }

    // Verify data
    uint16_t data = (uint16_t)chunk;
    for (int i = 0; i < FS_TEST_SPEED_CHUNK_SIZE; i += sizeof(data)) {
      if (*(uint16_t*)(buffer + i) != data) {
        printf("\nfs_test: verify error: chunk 0x%06lx, addr 0x%04x:\n",
          chunk, i);
        printf("  expected 0x%04x, got 0x%04x\n",
          data, *(uint16_t*)(buffer + i));
      }
    }

  }

  f_close(&file);

  // Print read speed (in bytes/s)
  {
    TickType_t current_ticks = xTaskGetTickCount();
    TickType_t tick_delta = current_ticks - last_ticks;
    uint32_t ms_delta = tick_delta * portTICK_PERIOD_MS;
    uint32_t bytes = FS_TEST_SPEED_NUM_CHUNKS * FS_TEST_SPEED_CHUNK_SIZE;
    double kbytes_per_s = (double)bytes/ms_delta;
    printf("\nRead speed: %0.2f KB/s\n", kbytes_per_s);
  }

  // Free buffer and close file on exit
  fs_test_speed_command_exit:
#if 1
  free(buffer);
#endif
  (void)f_close(&file);

  // Remove test file (if it exists)
  f_unlink(test_filename);

  printf("Completed \"fs_test_speed_command\".\n");
}

enum {
  FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE = 1024,
  FS_TEST_MIXEDRW_SPEED_NUM_CHUNKS = 1024,
  FS_TEST_MIXEDRW_SPEED_INDICATE_CHUNKS = 1
};

#endif

#if (defined(ENABLE_FS_TEST_COMMANDS) && (ENABLE_FS_TEST_COMMANDS > 0U))

void
fs_test_mixedrw_speed_command(int argc, char **argv)
{
  FRESULT result;
  FIL file_w;
  FIL file_r;

  printf("Starting mixed read/write test...\n");

  // write a file and read a file.
  static const char test_read_filename[]  = "/fs_test_read_speed";
  static const char test_write_filename[] = "/fs_test_write_speed";

  // Allocate data buffer
  uint8_t* buffer = malloc(FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE);
  if (buffer == NULL) {
    printf("Error: Unable to allocate buffer!\n");
    return;
  }

  // Remove test file (if it exists)
  f_unlink(test_read_filename);
  f_unlink(test_write_filename);

  //////////////////
  // SETUP READ FILE
  printf("Creating read file...\n");

  // Write 16 MB (64 flash blocks) file
  result = f_open(&file_w,  test_read_filename, FA_WRITE | FA_CREATE_ALWAYS);
  if (result) {
    // File open error--bail out
    printf("Error: unable to open file '%s' for writing: %u\n", test_read_filename, result);
    goto fs_test_mixedrw_speed_command_exit;
  }

  TickType_t last_ticks = xTaskGetTickCount();
  for (uint32_t chunk = 0; chunk <= FS_TEST_MIXEDRW_SPEED_NUM_CHUNKS; chunk++) {

    if ((chunk % FS_TEST_SPEED_INDICATE_CHUNKS) == 0) {
      // Print block indicator (erase spinner, space after for spinner)
      printf(".");
      fflush(stdout);
    }

    // Populate buffer
    uint16_t data = (uint16_t)chunk;
    for (int i = 0; i < FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE; i += sizeof(data)) {
      *(uint16_t*)(buffer + i) = data;
    }

    printf("fs_commands: fs_test_mixedrw_speed_command 0, chunk: %lu\n", chunk);

    if(chunk==127){
      printf("here");
    }

    // Write data
    UINT bytes_written;
    result = f_write(&file_w, buffer, FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE, &bytes_written);
    if (result != FR_OK || bytes_written < FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE) {
      printf("\nWrite error: %d\n", bytes_written);
      goto fs_test_mixedrw_speed_command_exit;
    }

    printf("fs_commands: fs_test_mixedrw_speed_command 1\n");

  }
  f_close(&file_w);

  // Print write speed (in bytes/s)
  {
    TickType_t current_ticks = xTaskGetTickCount();
    TickType_t tick_delta = current_ticks - last_ticks;
    uint32_t ms_delta = tick_delta * portTICK_PERIOD_MS;
    uint32_t bytes = FS_TEST_MIXEDRW_SPEED_NUM_CHUNKS * FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE;
    double kbytes_per_s = (double)bytes/ms_delta;
    printf("\nWrite speed: %0.2f KB/s\n", kbytes_per_s);
  }

  /////////////////////////////
  // INTERLEAVED READ AND WRITE
  // (do not verify from write file)
  printf("Running interleaved read/writes...\n");

  // Open Read File: Read and verify file
  result = f_open(&file_r, test_read_filename, FA_READ);
  if (result) {
    // File open error--bail out
    printf("Error: unable to open file '%s' for reading: %u\n", test_read_filename, result);
    goto fs_test_mixedrw_speed_command_exit;
  }

  // Open Write File: Write 16 MB (64 flash blocks) file
  result = f_open(&file_w, test_write_filename, FA_WRITE | FA_CREATE_ALWAYS);
  if (result) {
    // File open error--bail out
    printf("Error: unable to open file '%s' for writing: %u\n", test_write_filename, result);
    goto fs_test_mixedrw_speed_command_exit;
  }

  // INTERLEAVED Read and Write
  TickType_t read_tick_total = 0;
  TickType_t write_tick_total = 0;
  for (uint32_t chunk = 0; chunk <= FS_TEST_MIXEDRW_SPEED_NUM_CHUNKS; chunk++) {

    last_ticks = xTaskGetTickCount();

    if ((chunk % FS_TEST_SPEED_INDICATE_CHUNKS) == 0) {
      // Print block indicator (erase spinner, space after for spinner)
      printf(".");
      fflush(stdout);
    }

    // Populate buffer
    uint16_t data = (uint16_t)chunk;
    for (int i = 0; i < FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE; i += sizeof(data)) {
      *(uint16_t*)(buffer + i) = data;
    }

    // Write data
    {
      last_ticks = xTaskGetTickCount();
      UINT bytes_written;
      result = f_write(&file_w, buffer, FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE, &bytes_written);
      if (result != FR_OK || bytes_written < FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE) {
        printf("\nWrite error: %d\n", bytes_written);
        goto fs_test_mixedrw_speed_command_exit;
      }
      write_tick_total += (xTaskGetTickCount() - last_ticks);
    }

    // Read data
    {
      last_ticks = xTaskGetTickCount();
      UINT bytes_read;
      result = f_read(&file_r, buffer, FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE, &bytes_read);
      if (result != FR_OK || bytes_read < FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE) {
        printf("\nRead error: %d\n", bytes_read);
        goto fs_test_mixedrw_speed_command_exit;
      }
      read_tick_total += (xTaskGetTickCount() - last_ticks);
    }

    // Verify written data post read
    {
      uint16_t data = (uint16_t)chunk;
      for (int i = 0; i < FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE; i += sizeof(data)) {
        if (*(uint16_t*)(buffer + i) != data) {
          printf("\nuffs_test: verify error: chunk 0x%06lx, addr 0x%04x:\n",
            chunk, i);
          printf("  expected 0x%04x, got 0x%04x\n",
            data, *(uint16_t*)(buffer + i));
        }
      }
    }

  }
  printf("\n");

  // Print read speed (in bytes/s)
  {
    uint32_t ms_delta = read_tick_total * portTICK_PERIOD_MS;
    uint32_t bytes = FS_TEST_MIXEDRW_SPEED_NUM_CHUNKS * FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE;
    double kbytes_per_s = (double)bytes/ms_delta;
    printf("Interleaved read speed: %0.2f KB/s\n", kbytes_per_s);
  }

  // Print write speed (in bytes/s)
  {
    uint32_t ms_delta = write_tick_total * portTICK_PERIOD_MS;
    uint32_t bytes = FS_TEST_MIXEDRW_SPEED_NUM_CHUNKS * FS_TEST_MIXEDRW_SPEED_CHUNK_SIZE;
    double kbytes_per_s = (double)bytes/ms_delta;
    printf("Interleaved write speed: %0.2f KB/s\n", kbytes_per_s);
  }

  fs_test_mixedrw_speed_command_exit:
  // free the buffer
  free(buffer);
  // close and remove both write and read files
  if(f_is_open(&file_w)){
    f_close(&file_w);
    f_unlink(test_write_filename);
  }
  if(f_is_open(&file_r)){
    f_close(&file_r);
    f_unlink(test_read_filename);
  }

  printf("Completed \"fs_test_mixedrw_speed_command_exit\".\n");
}

#endif



#if (defined(ENABLE_FS_TEST_COMMANDS) && (ENABLE_FS_TEST_COMMANDS > 0U))

#define FS_TEST_FILLING_DATA_SIZE 256

void
fs_test_filling_write_read_command(int argc, char **argv)
{
  printf("Starting \"fs_test_filling_write_read_command\"...\n");

  FRESULT result;
  FIL f_write1, f_write2;

  static const char test_write1_filename[] = "/fs_test_write1";
  static const char test_write2_filename[] = "/fs_test_write2";

  // allocate memory
  uint8_t* write_buffer = malloc(FS_TEST_FILLING_DATA_SIZE);
  if (write_buffer == NULL) {
    printf("Error: Unable to allocate write buffer!\n");
    goto fs_test_filling_write_read_command_exit;
  }

  // Remove test files (if they exist)
  f_unlink(test_write1_filename);
  f_unlink(test_write2_filename);

  // Open write 1 file
  result = f_open(&f_write1, test_write1_filename, FA_WRITE | FA_CREATE_ALWAYS);
  if (FR_OK != result) {
    printf("Could not open %s: %u\n", test_write1_filename, result);
    goto fs_test_filling_write_read_command_exit;
  }

  // Open write 2 file
  result = f_open(&f_write2, test_write2_filename, FA_WRITE | FA_CREATE_ALWAYS);
  if (FR_OK != result) {
    printf("Could not open %s: %u\n", test_write2_filename, result);
    goto fs_test_filling_write_read_command_exit;
  }

  // fill the data segment to be written
  for (int i=0; i<FS_TEST_FILLING_DATA_SIZE; i++) {
    write_buffer[i] = i;
  }

  // 2 files written, and 1000 bytes of left over space, just in case
  int free_space_bytes_threshold = 1000000;//FS_TEST_FILLING_DATA_SIZE*2 + 1000;
  uint32_t data_written_count_threshold = 0xFFFFFFFF; //1000; // set to lower limit to test the command
  uint32_t data_written_count = 0;

  // Repeatedly write to the file system
  while ( true ) {
    DWORD free_space_bytes;
    result = f_getfreebytes(&free_space_bytes, NULL);
    if (FR_OK != result) {
      printf("Couldn't count free bytes: %u\n", result);
      return;
    }
    if ( free_space_bytes <= free_space_bytes_threshold || data_written_count > data_written_count_threshold) {
      break;
    }

    UINT bytes_written = 0;
    // write to file 1
    result = f_write(&f_write1, &(write_buffer[0]), FS_TEST_FILLING_DATA_SIZE, &bytes_written);
    if (result != FR_OK || bytes_written != FS_TEST_FILLING_DATA_SIZE) {
      printf("Failed to write to ");
      printf(test_write1_filename);
      printf(": %u\n", result);
      goto fs_test_filling_write_read_command_exit;
    }

    // write to file 2
    result = f_write(&f_write2, &(write_buffer[0]), FS_TEST_FILLING_DATA_SIZE, &bytes_written);
    if (result != FR_OK || bytes_written != FS_TEST_FILLING_DATA_SIZE) {
      printf("Failed to write to ");
      printf(test_write2_filename);
      printf(": %u\n", result);
      goto fs_test_filling_write_read_command_exit;
    }

    data_written_count++;
    printf("Written %lu chunks of 256 bytes. Free space remaining: %lu bytes.\n", data_written_count, free_space_bytes);
  }

  // Close file write 1
  if(f_is_open(&f_write1)) {
    f_close(&f_write1);
  }

  // Close file write 2
  if(f_is_open(&f_write2)) {
    f_close(&f_write2);
  }

  printf("Done writing to files\n");

  // Open write 1 file, reusing file object
  result = f_open(&f_write1, test_write1_filename, FA_READ);
  if (FR_OK != result) {
    printf("Could not open ");
    printf(test_write1_filename);
    printf("for reading: %u\n", result);
    goto fs_test_filling_write_read_command_exit;
  }

  // Open write 2 file, reusing file object
  result = f_open(&f_write2, test_write2_filename, FA_READ);
  if (FR_OK != result) {
    printf("Could not open ");
    printf(test_write2_filename);
    printf("for reading: %u\n", result);
    goto fs_test_filling_write_read_command_exit;
  }

  // Repeatedly read from and verify the file system
  for ( uint32_t data_read_count=0; data_read_count<data_written_count; data_read_count++ ){
    UINT bytes_read = 0;
    // READ FILE 1
    memset(write_buffer,0,FS_TEST_FILLING_DATA_SIZE);
    result = f_read(&f_write1, write_buffer, FS_TEST_FILLING_DATA_SIZE, &bytes_read);
    // check num read bytes is correct
    if(result != FR_OK || bytes_read != FS_TEST_FILLING_DATA_SIZE){
      printf("Failed to read %d bytes from ", FS_TEST_FILLING_DATA_SIZE);
      printf(test_write1_filename);
      printf(": %u\n", result);
      goto fs_test_filling_write_read_command_exit;
    }
    // check data read is correct
    for(int i=0; i<FS_TEST_FILLING_DATA_SIZE; i++){
      if (write_buffer[i] != i){
        printf("Incorrect bytes read from ");
        printf(test_write1_filename);
        printf("\n");
        goto fs_test_filling_write_read_command_exit;
      }
    }
    // READ FILE 2
    memset(write_buffer,0,FS_TEST_FILLING_DATA_SIZE);
    result = f_read(&f_write2, write_buffer, FS_TEST_FILLING_DATA_SIZE, &bytes_read);
    // check num read bytes is correct
    if(result != FR_OK || bytes_read != FS_TEST_FILLING_DATA_SIZE){
      printf("Failed to read %d bytes from ", FS_TEST_FILLING_DATA_SIZE);
      printf(test_write2_filename);
      printf(": %u\n", result);
      goto fs_test_filling_write_read_command_exit;
    }
    // check data read is correct
    for(int i=0; i<FS_TEST_FILLING_DATA_SIZE; i++){
      if (write_buffer[i] != i){
        printf("Incorrect bytes read from ");
        printf(test_write2_filename);
        printf(":%u\n", result);
        goto fs_test_filling_write_read_command_exit;
      }
    }

    printf("Read %lu chunks of 256 bytes. Percent complete: %f %%.\n", data_read_count, (100.0f*data_read_count/data_written_count));
  }

  fs_test_filling_write_read_command_exit:
  free(write_buffer);

  // Close file write 1
  if (f_is_open(&f_write1)) {
    f_close(&f_write1);
  }

  // Close file write 2
  if (f_is_open(&f_write2)) {
    f_close(&f_write2);
  }

  printf("Completed \"fs_test_filling_write_read_command\".\n");
}

#endif
