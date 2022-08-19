#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "../ble/ble_shell.h"
#include <sys/time.h>
#include "ff.h"
#include "fatfs_writer.h"
#include "fatfs_utils.h"
#include "zmdm.h"
#include "crctab.h"
#include "zmodem.h"
#include <sys/stat.h>
#include <string.h>
#include "zmodem_transfer.h"
#include "utils.h"


int subpacket_size = MAX_BUF_LEN; /* data subpacket size. may be modified during a session */

unsigned char tx_data_subpacket[MAX_BUF_LEN];
long current_file_size;
int n_files_remaining;

extern int can_full_duplex;
extern int can_overlap_io;
extern int can_break;
extern int can_fcs_32;
extern int escape_all_control_characters; /* guess */
extern int escape_8th_bit;

extern int use_variable_headers; /* use variable length headers */

//----------  receive section ----------//

long mdate; /* file date of file being received */
char filename[0x80]; /* filename of file being received */
char *name; /* pointer to the part of the filename used in the actual open */
unsigned char rx_data_subpacket[MAX_BUF_LEN]; /* zzap = 8192 */
int junk_pathnames = FALSE; /* junk incoming path names or keep them */

extern int receive_32_bit_data;
extern int raw_trace;
extern int want_fcs_32;
extern long ack_file_pos; /* file position used in acknowledgement of correctly */
/* received data subpackets */
extern unsigned char rxd_header[ZMAXHLEN]; /* last received header */

/*
 * send from the current position in the file
 * all the way to end of file or until something goes wrong.
 * (ZNAK or ZRPOS received)
 * the name is only used to show progress
 */

int
send_from(char *name, FIL *fp) {
  int n;
  UINT bytes_read;
  int type = ZCRCG;
  unsigned char zdata_frame[] = { ZDATA, 0, 0, 0, 0 };
  int prev_count = 0;
  /*
   * put the file position in the ZDATA frame
   */
  zdata_frame[ZP0] = f_tell(fp) & 0xff;
  zdata_frame[ZP1] = (f_tell(fp) >> 8) & 0xff;
  zdata_frame[ZP2] = (f_tell(fp) >> 16) & 0xff;
  zdata_frame[ZP3] = (f_tell(fp) >> 24) & 0xff;
  tx_header(zdata_frame);
  vTaskDelay((50) / portTICK_PERIOD_MS);
  /*
   * send the data in the file
   */
  while (!f_eof(fp) && (prev_count < current_file_size)) {
    /*
     * read a block from the file
     */
    memset(tx_data_subpacket, 0xFF, sizeof(tx_data_subpacket));
    n = f_read(fp, &tx_data_subpacket[0], subpacket_size, &bytes_read);
    if (FR_OK != n) {
      // Try to close the file.
      f_close(fp);
      return -1;
    }
    /*
     * at end of file wait for an ACK
     */
    if (f_tell(fp) == current_file_size) {
      type = ZCRCW;
    }
#if defined (ZMODEM_VIA_VC)
          debug_uart_puts("bytes=%d\r\n", bytes_read);
#elif defined (ZMODEM_VIA_DEBUG_UART)
          printf("bytes=%d\r\n", bytes_read);
#elif defined (ZMODEM_VIA_BLE)
          printf("bytes=%d\r\n", bytes_read);
#endif
    tx_data(type, tx_data_subpacket, bytes_read);
    vTaskDelay((50) / portTICK_PERIOD_MS);
    if (type == ZCRCW) {
      int type;
      do {
        type = rx_header(10000);
        if (type == ZNAK || type == ZRPOS) {
#if defined (ZMODEM_VIA_VC)
          debug_uart_puts("exit here...\r\n");
#elif defined (ZMODEM_VIA_DEBUG_UART)
          printf("exit here...\r\n");
#elif defined (ZMODEM_VIA_BLE)
          printf("exit here...\r\n");
#endif

          return type;
        }
      } while (type != ZACK);

      prev_count += bytes_read;
      if (f_tell(fp) == current_file_size) {
        return ZACK;
      }
    }
    /* characters from the other side check out that header */

  }

  /*
   * end of file reached.
   * should receive something... so fake ZACK
   */
  return ZACK;
}

int
send_file(char *name) {
  long pos;
  long size;
  unsigned char *p;
  unsigned char zfile_frame[] = { ZFILE, 0, 0, 0, 0 };
  unsigned char zeof_frame[] = { ZEOF, 0, 0, 0, 0 };
  int type;
  char *n;
  FRESULT result;
  FIL file;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmtx: sending file \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
          printf("zmtx: sending file \"%s\"\r", name);
#elif defined(ZMODEM_VIA_BLE)
          printf("zmtx: sending file \"%s\"\r", name);
#endif
  /*
   * before doing a lot of unnecessary work check if the file exists
   */
  result = f_open(&file, name, FA_READ);
  if (result) {
#if defined(ZMODEM_VIA_VC)
    debug_uart_puts("zmtx: can't open file \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmtx: can't open file %s\n", name);
#elif defined(ZMODEM_VIA_BLE)
  printf("zmtx: can't open file %s\n", name);
#endif
    return 0;
  }
  FILINFO finfo;
  result = f_stat(name, &finfo);
  if (result) {
    //LOGE(TAG, "f_stat for %s returned %u\n", filename, result);
    return -1;
  }
  size = finfo.fsize;
  current_file_size = size;

  /*
   * the file exists. now build the ZFILE frame
   */

  /*
   * set conversion option
   * (not used; always binary)
   */

  zfile_frame[ZF0] = ZF0_ZCBIN;

  zfile_frame[ZF1] = ZF1_ZMNEW; //overwriting destination always
  /*
   * transport options
   * (just plain normal transfer)
   */

  zfile_frame[ZF2] = ZF2_ZTNOR;

  /*
   * extended options
   */

  zfile_frame[ZF3] = 0;

  /*
   * now build the data subpacket with the file name and lots of other
   * useful information.
   */

  /*
   * first enter the name and a 0
   */

  p = tx_data_subpacket;

  /*
   * strip the path name from the filename
   */

  n = strrchr(name, '/');
  if (n == NULL) {
    n = name;
  }
  else {
    n++;
  }

  strcpy((char*) p, (const char*) n);

  p += strlen((const char*) p) + 1;

  /*
   * next the file size
   */

  sprintf((char*) p, (const char*) "%ld ", size);

  p += strlen((const char*) p);

  /*
   * modification date
   */

  sprintf((char*) p, (const char*) "%d ", finfo.fdate);

  p += strlen((const char*) p);

  /*
   * file mode
   */

  sprintf((char*) p, (const char*) "0 ");

  p += strlen((const char*) p);

  /*
   * serial number (??)
   */

  sprintf((char*) p, (const char*) "0 ");

  p += strlen((const char*) p);

  /*
   * number of files remaining
   */

  sprintf((char*) p, (const char*) "%d ", n_files_remaining);

  p += strlen((const char*) p);

  /*
   * file type
   */

  sprintf((char*) p, (const char*) "0");

  p += strlen((const char*) p) + 1;
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmtx: sending header and data \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmtx: sending header and data \r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("zmtx: sending header and data \r\n");
#endif

  do {
    /*
     * send the header and the data
     */
    tx_header(zfile_frame);
    tx_data(ZCRCW, tx_data_subpacket, p - tx_data_subpacket);

    vTaskDelay((50) / portTICK_PERIOD_MS);
    /*
     * wait for anything but an ZACK packet
     */

    do {
      type = rx_header(10000);
    } while (type == ZACK);
    if (type == ZSKIP) {
      f_close(&file);
      return -1;
    }

  } while (type != ZRPOS);

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("transfer start \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("transfer start \r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("transfer start \r\n");
#endif

  do {
    /*
     * fetch pos from the ZRPOS header
     */

    if (type == ZRPOS) {
      pos = rxd_header[ZP0] | (rxd_header[ZP1] << 8) | (rxd_header[ZP2] << 16)
          | (rxd_header[ZP3] << 24);
    }
    /*
     * seek to the right place in the file
     */

    f_lseek(&file, pos);
    /*
     * and start sending
     */
    type = send_from(n, &file);
    if (type == ZFERR || type == ZABORT) {
      f_close(&file);
      return TRUE;
    }
  } while (type == ZRPOS || type == ZNAK);

  /*
   * file sent. send end of file frame
   * and wait for zrinit. if it doesnt come then try again
   */

  zeof_frame[ZP0] = finfo.fsize & 0xff;
  zeof_frame[ZP1] = (finfo.fsize >> 8) & 0xff;
  zeof_frame[ZP2] = (finfo.fsize >> 16) & 0xff;
  zeof_frame[ZP3] = (finfo.fsize >> 24) & 0xff;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("sending eof \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("sending eof \r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("sending eof \r\n");
#endif

  do {
    tx_hex_header(zeof_frame);
    vTaskDelay((50) / portTICK_PERIOD_MS);
    type = rx_header(10000);
  } while (type != ZRINIT);

  /*
   * and close the input file
   */
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmtx: sent file \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmtx: sent file \"%s\"\n", name);
#elif defined(ZMODEM_VIA_BLE)
  printf("zmtx: sent file \"%s\"\n", name);
#endif
  f_close(&file);
  rx_purge();
  return TRUE;
}

int
zmodem_send_file(char *filename) {

  unsigned char zrqinit_header[] = { ZRQINIT, 0, 0, 0, 0 };
  /*
   * Clear the input queue from any possible garbage
   * this also clears a possible ZRINIT from an already started
   * zmodem receiver. this doesn't harm because we reinvite to
   * receive again below and it may be that the receiver whose
   * ZRINIT we are about to wipe has already died.
   */
  rx_purge();
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmtx: establishing contact with receiver... \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmtx: establishing contact with receiver...\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("zmtx: establishing contact with receiver...\r\n");
#endif

  do {
    tx_raw('r');
    tx_raw('z');
    tx_raw('\r');
    vTaskDelay((50) / portTICK_PERIOD_MS);

    tx_hex_header(zrqinit_header);
    vTaskDelay((50) / portTICK_PERIOD_MS);
  } while (rx_header(7000) != ZRINIT);

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("header received \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("header received \r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("header received \r\n");
#endif

  can_full_duplex = (rxd_header[ZF0] & ZF0_CANFDX) != 0;
  can_overlap_io = (rxd_header[ZF0] & ZF0_CANOVIO) != 0;
  can_break = (rxd_header[ZF0] & ZF0_CANBRK) != 0;
  can_fcs_32 = (rxd_header[ZF0] & ZF0_CANFC32) != 0;
  escape_all_control_characters = (rxd_header[ZF0] & ZF0_ESCCTL) != 0;
  escape_8th_bit = (rxd_header[ZF0] & ZF0_ESC8) != 0;

  use_variable_headers = (rxd_header[ZF1] & ZF1_CANVHDR) != 0;
  int ret = send_file(filename);
  return ret;

}

/* -------------------  receive section ---------------------------- */

/*
 * receive a header and check for garbage
 */

/*
 * receive file data until the end of the file or until something goes wrong.
 * the name is only used to show progress
 */
int
receive_file_data(char *name, FIL *fp) {
  long pos;
  int n;
  int type;

  /*
   * create a ZRPOS frame and send it to the other side
   */
  tx_pos_header(ZRPOS, f_tell(fp));
  vTaskDelay((50) / portTICK_PERIOD_MS);
  /*
   * wait for a ZDATA header with the right file offset
   * or a timeout or a ZFIN
   */

  do {
    do {
      type = rx_header(10000);
      if (type == TIMEOUT) {
        return TIMEOUT;
      }
    } while (type != ZDATA);

    pos = rxd_header[ZP0] | (rxd_header[ZP1] << 8) | (rxd_header[ZP2] << 16)
        | (rxd_header[ZP3] << 24);
  } while (pos != f_tell(fp));
  memset(rx_data_subpacket, 0x00, sizeof(rx_data_subpacket));
  type = rx_data(rx_data_subpacket, &n);
  n = n - 1;

  if (type == ENDOFFRAME || type == FRAMEOK) {
    UINT bytes_written;
    FRESULT result;
    result = f_write(fp, rx_data_subpacket + 1, n, &bytes_written);
    if (result) {
      if (f_is_open(fp)) {
        f_sync_wait(fp);
        f_close(fp);
      }
    }
  }
  return type;
}
/*
 * receive a file
 * if the file header info packet was garbled then send a ZNAK and return
 * (using ZABORT frame)
 */

int
receive_file() {
  long size;
  int type;
  int l;

  FIL file;
  /*
   * fetch the management info bits from the ZRFILE header
   */
  /*
   * read the data subpacket containing the file information
   */
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("receive_file: beginning of read data subpacket... \r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("receive_file: beginning of read data subpacket...\r\n");
#endif

  type = rx_data(rx_data_subpacket, &l);
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("receive_file: end of read data subpacket...\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("receive_file: end of read data subpacket...\r\n");
#endif

  if (type != FRAMEOK && type != ENDOFFRAME) {
    if (type != TIMEOUT) {
      /*
       * file info data subpacket was trashed
       */
      tx_znak();
    }
    return -1;
  }
  /*
   * extract the relevant info from the header.
   */

  strcpy(filename, (const char*) rx_data_subpacket + 1);

  if (junk_pathnames) {
    name = strrchr(filename, '/');
    if (name != NULL) {
      name++;
    }
    else {
      name = filename;
    }
  }
  else {
    name = filename;
  }

  sscanf((const char*) rx_data_subpacket + strlen((const char*) rx_data_subpacket) + 1,
      (const char*) "%ld %lo", &size, &mdate);

  unsigned long fs_free_bytes;
  unsigned long fs_total_bytes;

  FRESULT result;

  result = f_getfreebytes(&fs_free_bytes, &fs_total_bytes);
  if (FR_OK != result) {
    return -1;
  }
  current_file_size = size;
  /*
   * transfer the file
   * either not present; remote newer; ok to clobber or no options set.
   * (no options->clobber anyway)
   */

  result = f_open(&file, name, FA_OPEN_ALWAYS | FA_WRITE);
  if (result) {
    tx_pos_header(ZSKIP, 0L);
#if defined(ZMODEM_VIA_VC)
    debug_uart_puts("zmrx: can't open file\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmrx: can't open file %s\r\n", name);
#endif
    return -1;
  }

  while (f_tell(&file) != size) {
    type = receive_file_data(name, &file);
    if (type == ZEOF) {
      break;
    }
  }
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("receive_file: end of receive_file_data...\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("receive_file: end of receive_file_data...\r\n");
#endif

  /*
   * wait for the eof header
   */

  while (type != ZEOF) {
    type = rx_header_and_check(10000);
  }

  /*
   * close and exit
   */

  f_close(&file);

  /*
   * and close the input file
   */
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmrx: received fil\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmrx: received file \"%s\"\r\n", name);
#endif

  debug_uart_puts("zmrx: received file\r\n");
  return current_file_size;
}

long
zmodem_receive_file(void) {

  unsigned char zrinit_header[] = { ZRINIT, 0, 0, 0, 4 | ZF0_CANFDX | ZF0_CANOVIO | ZF0_CANFC32 };
  long size = 0;
  int type;
  /*
   * Clear the input queue from any possible garbage
   * this also clears a possible ZRINIT from an already started
   * zmodem receiver. this doesn't harm because we reinvite to
   * receive again below and it may be that the receiver whose
   * ZRINIT we are about to wipe has already died.
   */

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmtx: establishing contact with receiver...\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmtx: establishing contact with receiver...\r\n");
#endif

  do {
    tx_hex_header(zrinit_header);
    vTaskDelay((50) / portTICK_PERIOD_MS);
    type = rx_header(7000);
  } while (type == ZRQINIT || type == TIMEOUT);

  do {
    switch (type) {
    case ZFILE:
#if defined(ZMODEM_VIA_VC)
      debug_uart_puts("main: beginning of receive file...\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("main: beginning of receive file...\r\n");
#endif
      size = receive_file();
      break;
    default:
      tx_pos_header(ZCOMPL, 0l);
      break;
    }
    do {
#if defined(ZMODEM_VIA_VC)
      debug_uart_puts("main: ask for next file transfer...\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("main: ask for next file transfer...\r\n");
#endif

      tx_hex_header(zrinit_header);
      vTaskDelay((50) / portTICK_PERIOD_MS);
      type = rx_header(7000);
    } while (type == TIMEOUT);
  } while (type != ZFIN);
  /*
   * close the session
   */
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmrx: closing the session\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmrx: closing the session\r\n");
#endif

  unsigned char zfin_header[] = { ZFIN, 0, 0, 0, 0 };

  tx_hex_header(zfin_header);
  vTaskDelay((50) / portTICK_PERIOD_MS);

  /*
   * wait for the over and out sequence
   */

  int c;
  do {
    c = rx_raw(1000);
  } while (c != 'O' && c != TIMEOUT);

  rx_purge();
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmrx: cleanup and exit\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmrx: cleanup and exit\n");
#endif

  return size;
}
