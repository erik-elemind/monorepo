/******************************************************************************/
/* Project : Unite!       File : zmodem general        Version : 1.02         */
/*                                                                            */
/* (C) Mattheij Computer Service 1994                                         */
/*                                                                            */
/* contact us through (in order of preference)                                */
/*                                                                            */
/*   email:          jacquesm@hacktic.nl                                      */
/*   mail:           MCS                                                      */
/*                   Prinses Beatrixlaan 535                                  */
/*                   2284 AT  RIJSWIJK                                        */
/*                   The Netherlands                                          */
/*   voice phone:    31+070-3936926                                           */
/******************************************************************************/

/*
 * zmodem primitives and other code common to zmtx and zmrx
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "fsl_common.h"
#include "config.h"

#include <sys/time.h>
#include "zmdm.h"
#include "crctab.h"
#include "zmdm.h"
#include "utils.h"
#include "board_config.h"
#include "zmodem_transfer.h"
#include "config.h"
#include "ble_shell.h"

unsigned char rxd_header[ZMAXHLEN]; /* last received header */
int rxd_header_len; /* last received header size */

unsigned char inputbuffer[MAX_BUF_LEN];
static int inputbuffer_index = 0;

#if defined(ZMODEM_VIA_VC)
#define _getchar(X) getchar()
#define _putchar    putchar
#endif

#if defined(ZMODEM_VIA_BLE)
#define _getchar(X)      ble_shell_getchar(X)
#define _putchar         ble_shell_putchar
#endif

#if defined(ZMODEM_VIA_DEBUG_UART)
int get_pos = 0;
static int pos = 0;
int actual_cnt = 0;
static int offset = 0;
#endif

/*
 * receiver capability flags
 * extracted from the ZRINIT frame as received
 */

int can_full_duplex;
int can_overlap_io;
int can_break;
int can_fcs_32;
int escape_all_control_characters; /* guess */
int escape_8th_bit;

int use_variable_headers; /* use variable length headers */

int receive_32_bit_data;
int want_fcs_32 = TRUE;
long ack_file_pos; /* file position used in acknowledgement of correctly */
/* received data subpackets */

#define XXX_SKIP_CRC 1 /* XXX: Skip CRC check at the moment */

/*
 * read bytes as long as rdchk indicates that
 * more data is available.
 */

void
rx_purge(void) {
  memset(inputbuffer, 0x00, sizeof(inputbuffer));
  inputbuffer_index = 0;
#if defined(ZMODEM_VIA_DEBUG_UART)
  pos = 0;
  offset = 0;
  debug_uart_buffer_clear();
#endif
}

int last_sent = -1;

/*
 * transmit a character.
 * this is the raw modem interface
 */

void
tx_raw(int c) {
  last_sent = c & 0x7f;
#if defined(ZMODEM_VIA_VC)
  _putchar(c);
#elif defined(ZMODEM_VIA_DEBUG_UART)
  USART_RTOS_Send(&USART_DEBUG_RTOS_HANDLE, (uint8_t*) &c, 1);
#endif
#if defined(ZMODEM_VIA_BLE)
  _putchar(c);
#endif

}

/*
 * transmit a character ZDLE escaped
 */

void
tx_esc(int c) {
  tx_raw(ZDLE);
  /*
   * exclusive or; not an or so ZDLE becomes ZDLEE
   */
  tx_raw(c ^ 0x40);
}

/*
 * transmit a character; ZDLE escaping if appropriate
 */

void
tx(unsigned char c) {
  switch (c) {
  case ZDLE:
    tx_esc(c);
    return;
    break;
  case 0x8d:
  case 0x0d:
    if (escape_all_control_characters && last_sent == '@') {
      tx_esc(c);
      return;
    }
    break;
  case 0x10:
  case 0x90:
  case 0x11:
  case 0x91:
  case 0x13:
  case 0x93:
    tx_esc(c);
    return;
    break;
  default:
    if (escape_all_control_characters && (c & 0x60) == 0) {
      tx_esc(c);
      return;
    }
    break;
  }
  /*
   * anything that ends here is so normal we might as well transmit it.
   */
  tx_raw((int) c);
}

/*
 * transmit a hex header.
 * these routines use tx_raw because we're sure that all the
 * characters are not to be escaped.
 */

void
tx_nibble(int n) {
  n &= 0x0f;
  if (n < 10) {
    n += '0';
  }
  else {
    n += 'a' - 10;
  }

  tx_raw(n);
}

void
tx_hex(int h) {
  tx_nibble(h >> 4);
  tx_nibble(h);
}

void
tx_hex_header(unsigned char *p) {
  int i;
  unsigned short int crc;
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("* tx_hex_header = %02X *\r\n", *p);
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("* tx_hex_header = %02X *\r\n", *p);
#elif defined(ZMODEM_VIA_BLE)
  printf("* tx_hex_header = %02X *\r\n", *p);
#endif

  tx_raw(ZPAD);
  tx_raw(ZPAD);
  tx_raw(ZDLE);
  if (use_variable_headers) {
    tx_raw(ZVHEX);
    tx_hex(HDRLEN);
  }
  else {
    tx_raw(ZHEX);
  }

  /*
   * initialise the crc
   */

  crc = 0;

  /*
   * transmit the header
   */

  for (i = 0; i < HDRLEN; i++) {
    tx_hex(*p);
    crc = UPDCRC16(*p, crc);
    p++;
  }
  /*
   * update the crc as though it were zero
   */

  crc = UPDCRC16(0, crc);
  crc = UPDCRC16(0, crc);
  /* transmit the crc */

  tx_hex(crc >> 8);
  tx_hex(crc);
  /*
   * end of line sequence
   */

  tx_raw(0x0d);
  tx_raw(0x0a);

  tx_raw(XON);

}

/*
 * Send ZMODEM binary header hdr
 */

void
tx_bin32_header(unsigned char *p) {
  int i;
  unsigned long crc;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("tx binary header 32 bits crc\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("tx binary header 32 bits crc\r\n");
#endif

  tx_raw(ZPAD);
  tx_raw(ZPAD);
  tx_raw(ZDLE);

  if (use_variable_headers) {
    tx_raw(ZVBIN32);
    tx(HDRLEN);
  }
  else {
    tx_raw(ZBIN32);
  }

  crc = 0xffffffffL;

  for (i = 0; i < HDRLEN; i++) {
    crc = UPDCRC32(*p, crc);
    tx(*p++);
  }

  crc = ~crc;

  tx(crc);
  tx(crc >> 8);
  tx(crc >> 16);
  tx(crc >> 24);
}

void
tx_bin16_header(unsigned char *p) {
  int i;
  unsigned int crc;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("tx binary header 16 bits crc\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("tx binary header 16 bits crc\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("tx binary header 16 bits crc\r\n");
#endif

  tx_raw(ZPAD);
  tx_raw(ZPAD);
  tx_raw(ZDLE);

  if (use_variable_headers) {
    tx_raw(ZVBIN);
    tx(HDRLEN);
  }
  else {
    tx_raw(ZBIN);
  }

  crc = 0;

  for (i = 0; i < HDRLEN; i++) {
    crc = UPDCRC16(*p, crc);
    tx(*p++);
  }

  crc = UPDCRC16(0, crc);
  crc = UPDCRC16(0, crc);

  tx(crc >> 8);
  tx(crc);
}

/*
 * transmit a header using either hex 16 bit crc or binary 32 bit crc
 * depending on the receivers capabilities
 * we dont bother with variable length headers. I dont really see their
 * advantage and they would clutter the code unneccesarily
 */

void
tx_header(unsigned char *p) {
  if (can_fcs_32) {
    if (want_fcs_32) {
      tx_bin32_header(p);
    }
    else {
      tx_bin16_header(p);
    }
  }
  else {
    tx_hex_header(p);
  }
}

/*
 * data subpacket transmission
 */

void
tx_32_data(int sub_frame_type, unsigned char *p, int l) {
  unsigned long crc;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("tx_32_data\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("tx_32_data \r\n");
#endif

  crc = 0xffffffffl;

  while (l > 0) {
    crc = UPDCRC32(*p, crc);
    tx(*p++);
    l--;
  }

  crc = UPDCRC32(sub_frame_type, crc);

  tx_raw(ZDLE);
  tx_raw(sub_frame_type);

  crc = ~crc;

  tx((int) (crc) & 0xff);
  tx((int) (crc >> 8) & 0xff);
  tx((int) (crc >> 16) & 0xff);
  tx((int) (crc >> 24) & 0xff);
}

void
tx_16_data(int sub_frame_type, unsigned char *p, int l) {
  unsigned short crc;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("tx_16_data\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("tx_16_data \r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("tx_16_data \r\n");
#endif

  crc = 0;

  while (l > 0) {
    crc = UPDCRC16(*p, crc);
    tx(*p++);
    l--;
  }

  crc = UPDCRC16(sub_frame_type, crc);

  tx_raw(ZDLE);
  tx_raw(sub_frame_type);

  crc = UPDCRC16(0, crc);
  crc = UPDCRC16(0, crc);

  tx(crc >> 8);
  tx(crc);
}

/*
 * send a data subpacket using crc 16 or crc 32 as desired by the receiver
 */

void
tx_data(int sub_frame_type, unsigned char *p, int l) {
  if (want_fcs_32 && can_fcs_32) {
    tx_32_data(sub_frame_type, p, l);
  }
  else {
    tx_16_data(sub_frame_type, p, l);
  }

  if (sub_frame_type == ZCRCW) {
    tx_raw(XON);
  }
}

void
tx_pos_header(int type, long pos) {
  unsigned char header[5];

  header[0] = type;
  header[ZP0] = pos & 0xff;
  header[ZP1] = (pos >> 8) & 0xff;
  header[ZP2] = (pos >> 16) & 0xff;
  header[ZP3] = (pos >> 24) & 0xff;

  tx_hex_header(header);
}

void
tx_znak() {
  tx_pos_header(ZNAK, ack_file_pos);
}

/*
 * rx_raw ; receive a single byte from the line.
 * reads as many are available and then processes them one at a time
 * check the data stream for 5 consecutive CAN characters;
 * and if you see them abort. this saves a lot of clutter in
 * the rest of the code; even though it is a very strange place
 * for an exit. (but that was wat session abort was all about.)
 */
int
rx_raw(int to) {

#if defined(ZMODEM_VIA_VC)
  static char c;

  c = _getchar(10);
  if (c == 0x8d) {
    c = _getchar(to);
    if (c == 0x8a) {
      c = _getchar(to);
      if (c == 0x11) {
        return 0;
      }
      else {
        inputbuffer[inputbuffer_index] = c;
        return 0;
      }
    }
    return 0;
  }
  inputbuffer[inputbuffer_index] = c;
  inputbuffer_index++;

  if (inputbuffer_index >= MAX_BUF_LEN) {
    int data;
    data = inputbuffer[MAX_BUF_LEN - 1];
    memset(inputbuffer, 0x00, sizeof(inputbuffer));
    inputbuffer_index = 0;
    return data;
  }
  return inputbuffer[inputbuffer_index - 1];
#elif defined(ZMODEM_VIA_DEBUG_UART)

  static char c;
   actual_cnt = get_recv_count();
     if (actual_cnt != pos + offset && actual_cnt != 0) {
       c = debug_uart_getc(pos + offset);
       if (c == 0x8d) {
         pos++;
         c = debug_uart_getc(pos + offset);
         if (c == 0x8a) {
           pos++;
           c = debug_uart_getc(pos + offset);
           if (c == 0x11) {
             pos++;
             return 0;
           }
           else {
             pos++;
             inputbuffer[inputbuffer_index] = c;
             return 0;
           }
         }
         return 0;
       }
       inputbuffer[inputbuffer_index] = c;
       pos++;
       inputbuffer_index++;
       if (inputbuffer_index >= MAX_BUF_LEN) {
         int data;
         data = inputbuffer[MAX_BUF_LEN - 1];
         offset += pos;
         memset(inputbuffer, 0x00, sizeof(inputbuffer));
         inputbuffer_index = 0;
         pos = 0;
         return data;
       }
       return inputbuffer[inputbuffer_index - 1];
     }
     return 0;
#elif defined(ZMODEM_VIA_BLE)
  static char c;

  c = _getchar(0);
  if (c == 0x8d) {
    c = _getchar(to);
    if (c == 0x8a) {
      c = _getchar(to);
      if (c == 0x11) {
        return 0;
      }
      else {
        inputbuffer[inputbuffer_index] = c;
        return 0;
      }
    }
    return 0;
  }
  inputbuffer[inputbuffer_index] = c;
  inputbuffer_index++;

  if (inputbuffer_index >= MAX_BUF_LEN) {
    int data;
    data = inputbuffer[MAX_BUF_LEN - 1];
    memset(inputbuffer, 0x00, sizeof(inputbuffer));
    inputbuffer_index = 0;
    return data;
  }
  return inputbuffer[inputbuffer_index - 1];

#endif

}

/*
 * rx; receive a single byte undoing any escaping at the
 * sending site. this bit looks like a mess. sorry for that
 * but there seems to be no other way without incurring a lot
 * of overhead. at least like this the path for a normal character
 * is relatively short.
 */

int
rx(int to) {
  int c;
  /*
   * outer loop for ever so for sure something valid
   * will come in; a timeout will occur or a session abort
   * will be received.
   */

  while (TRUE) {
    while (TRUE) {
      c = rx_raw(to);
      if (c == TIMEOUT) {
        return c;
      }

      switch (c) {
      case ZDLE:
        break;
      case 0x11:
      case 0x91:
      case 0x13:
      case 0x93:
        continue;
        break;
      default:
        /*
         * if all control characters should be escaped and
         * this one wasnt then its spurious and should be dropped.
         */
        if (escape_all_control_characters && (c & 0x60) == 0) {
          continue;
        }
        /*
         * normal character; return it.
         */
        return c;
      }
      break;
    }
    while (TRUE) {
      c = rx_raw(to);
      if (c == TIMEOUT) {
        return c;
      }
      if (c == 0x11 || c == 0x13 || c == 0x91 || c == 0x93 || c == ZDLE) {
        /*
         * these can be dropped.
         */
        continue;
      }
      switch (c) {
      /*
       * these four are really nasty.
       * for convenience we just change them into
       * special characters by setting a bit outside the
       * first 8. that way they can be recognized and still
       * be processed as characters by the rest of the code.
       */
      case ZCRCE:
      case ZCRCG:
      case ZCRCQ:
      case ZCRCW:
        return (c | ZDLEESC);
        break;
      case ZRUB0:
        return 0x7f;
        break;
      case ZRUB1:
        return 0xff;
        break;
      default:
        if (escape_all_control_characters && (c & 0x60) == 0) {
          /*
           * a not escaped control character; probably
           * something from a network. just drop it.
           */
          continue;
        }
        /*
         * legitimate escape sequence.
         * rebuild the orignal and return it.
         */
        if ((c & 0x60) == 0x40) {
          return c ^ 0x40;
        }
        break;
      }
      break;
    }

  }

  /*
   * not reached.
   */
  return 0;
}

/*
 * receive a data subpacket as dictated by the last received header.
 * return 2 with correct packet and end of frame
 * return 1 with correct packet frame continues
 * return 0 with incorrect frame.
 * return TIMEOUT with a timeout
 * if an acknowledgement is requested it is generated automatically
 * here.
 */

/*
 * data subpacket reception
 */
int
rx_32_data(unsigned char *p, int *l) {

  int c;
  unsigned long rxd_crc;
  unsigned long crc;
  int sub_frame_type;

  crc = 0xffffffffl;

  do {
    c = rx(1000);

    if (c == TIMEOUT) {
      return TIMEOUT;
    }
    if (c < 0x100) {
      crc = UPDCRC32(c, crc);
      *p++ = c;
      (*l)++;
      continue;
    }
  } while (c < 0x100);

  sub_frame_type = c & 0xff;

  crc = UPDCRC32(sub_frame_type, crc);

  crc = ~crc;

  rxd_crc = rx(1000);
  rxd_crc |= rx(1000) << 8;
  rxd_crc |= rx(1000) << 16;
  rxd_crc |= rx(1000) << 24;

  ack_file_pos += *l;

  return sub_frame_type;
}

int
rx_16_data(unsigned char *p, int *l) {
  int c;
  int sub_frame_type;
  unsigned short crc;
  unsigned short rxd_crc;
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("rx_16_data\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("rx_16_data \r\n");
#endif

  crc = 0;

  do {
    vTaskDelay((50) / portTICK_PERIOD_MS);
    c = rx(5000);
    if (c == TIMEOUT) {
      return TIMEOUT;
    }
    if (c < 0x100) {
      crc = UPDCRC16(c, crc);
      *p++ = c;
      (*l)++;
    }
  } while (c < 0x100);
  sub_frame_type = c & 0xff;

  crc = UPDCRC16(sub_frame_type, crc);

  crc = UPDCRC16(0, crc);
  crc = UPDCRC16(0, crc);

  rxd_crc = rx(1000) << 8;
  rxd_crc |= rx(1000);

  ack_file_pos += *l;

  return sub_frame_type;
}

int
rx_data(unsigned char *p, int *l) {
  int sub_frame_type;
  long pos;

  /*
   * fill in the file pointer in case acknowledgement is requested.
   * the ack file pointer will be updated in the subpacket read routine;
   * so we need to get it now
   */

  pos = ack_file_pos;

  /*
   * receive the right type of frame
   */

  *l = 0;

  if (receive_32_bit_data) {
    sub_frame_type = rx_32_data(p, l);
  }
  else {
    sub_frame_type = rx_16_data(p, l);
  }
  switch (sub_frame_type) {
  case TIMEOUT:
    return TIMEOUT;
    break;
    /*
     * frame continues non-stop
     */
  case ZCRCG:
    return FRAMEOK;
    break;
    /*
     * frame ends
     */
  case ZCRCE:
    return ENDOFFRAME;
    break;
    /*
     * frame continues; ZACK expected
     */
  case ZCRCQ:
    tx_pos_header(ZACK, pos);
    return FRAMEOK;
    break;
    /*
     * frame ends; ZACK expected
     */
  case ZCRCW:
    tx_pos_header(ZACK, pos);
    return ENDOFFRAME;
    break;
  }

  return FALSE;
}

int
rx_nibble(int to) {
  int c;

  c = rx(to);

  if (c == TIMEOUT) {
    return c;
  }
  if (c > '9') {
    if (c < 'a' || c > 'f') {
      /*
       * illegal hex; different than expected.
       * we might as well time out.
       */
      return TIMEOUT;
    }
    c -= 'a' - 10;
  }
  else {
    if (c < '0') {
      /*
       * illegal hex; different than expected.
       * we might as well time out.
       */
      return TIMEOUT;
    }
    c -= '0';
  }

  return c;
}

int
rx_hex(int to) {
  int n1;
  int n0;

  n1 = rx_nibble(to);

  if (n1 == TIMEOUT) {
    return n1;
  }
  n0 = rx_nibble(to);

  if (n0 == TIMEOUT) {
    return n0;
  }
  return (n1 << 4) | n0;
}

/*
 * receive routines for each of the six different styles of header.
 * each of these leaves rxd_header_len set to 0 if the end result is
 * not a valid header.
 */

void
rx_bin16_header(int to) {
  int c;
  int n;
  unsigned short int crc;
  unsigned short int rxd_crc;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("rx_bin16_header: rx binary header 16 bits crc\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("rx_bin16_header: rx binary header 16 bits crc\r\n");
#endif

  crc = 0;

  for (n = 0; n < 4; n++) {
    c = rx(to);
    if (c == TIMEOUT) {
      return;
    }
    crc = UPDCRC16(c, crc);
    rxd_header[n] = c;
  }

  crc = UPDCRC16(ZDLE, crc);

  for (; n < 6; n++) {
    c = rx(to);
    if (c == TIMEOUT) {
      return;
    }
    rxd_header[n] = c;
  }

  rxd_crc = rxd_header[4] << 8;
  rxd_crc |= rxd_header[5];

  rxd_header_len = 5;
}

void
rx_hex_header(int to) {
  int c;
  int i;
  unsigned short int crc = 0;
  unsigned short int rxd_crc;
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("rx_hex_header\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("rx_hex_header\r\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("rx_hex_header\r\n");
#endif

  for (i = 0; i < 5; i++) {
    c = rx_hex(to);
    if (c == TIMEOUT) {
      return;
    }
    crc = UPDCRC16(c, crc);

    rxd_header[i] = c;
  }

  crc = UPDCRC16(0, crc);
  crc = UPDCRC16(0, crc);

  /*
   * receive the crc
   */

  c = rx_hex(to);

  if (c == TIMEOUT) {
    return;
  }
  rxd_crc = c << 8;

  c = rx_hex(to);

  if (c == TIMEOUT) {
    return;
  }
  rxd_crc |= c;

  if (rxd_crc == crc) {
    rxd_header_len = 5;
  }
#if defined(ZMODEM_VIA_VC)
  else {
    debug_uart_puts("bad crc.\r\n");
  }
#elif defined(ZMODEM_VIA_DEBUG_UART)
  else {
     printf("bad crc.\n");
   }
#elif defined(ZMODEM_VIA_BLE)
  else {
     printf("bad crc = %04X, %04X.\n", rxd_crc, crc);
   }
#endif

  /*
   * drop the end of line sequence after a hex header
   */
  c = rx(to);
  if (c == CR) {
    /*
     * both are expected with CR
     */
    c = rx(to);
  }
}

void
rx_bin32_header(int to) {
  int c;
  int n;
  unsigned long crc;
  unsigned long rxd_crc;

#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("rx binary header 32 bits crc\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("rx binary header 32 bits crc\r\n");
#endif

  crc = 0xffffffffL;

  for (n = 0; n < 5; n++) {
    c = rx(1000);
    if (c == TIMEOUT) {
      return;
    }
    crc = UPDCRC32(c, crc);
    rxd_header[n] = c;
  }

  crc = ~crc;

  rxd_crc = rx(1000);
  rxd_crc |= rx(1000) << 8;
  rxd_crc |= rx(1000) << 16;
  rxd_crc |= rx(1000) << 24;

  rxd_header_len = 5;
}

/*
 * receive any style header
 * if the errors flag is set than whenever an invalid header packet is
 * received INVHDR will be returned. otherwise we wait for a good header
 * also; a flag (receive_32_bit_data) will be set to indicate whether data
 * packets following this header will have 16 or 32 bit data attached.
 * variable headers are not implemented.
 */

int
rx_header_raw(int to, int errors) {
  int c;
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("rx_header_raw\r\n");
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("rx_header_raw...\n");
#elif defined(ZMODEM_VIA_BLE)
  printf("rx_header_raw...\n");
#endif

  rxd_header_len = 0;

  do {
    do {
      c = rx_raw(to);
      if (c == TIMEOUT) {
        return c;
      }
    } while (c != ZPAD);
    c = rx_raw(to);
    if (c == TIMEOUT) {
      return c;
    }
    if (c == ZPAD) {
      c = rx_raw(to);
      if (c == TIMEOUT) {
        return c;
      }
    }
    /*
     * spurious ZPAD check
     */

    if (c != ZDLE) {
      continue;
    }
    /*
     * now read the header style
     */
    c = rx(to);
    if (c == TIMEOUT) {
      return c;
    }
    switch (c) {
    case ZBIN:
      rx_bin16_header(to);
      receive_32_bit_data = FALSE;
      break;
    case ZHEX:
      rx_hex_header(to);
      receive_32_bit_data = FALSE;
      break;
    case ZBIN32:
      rx_bin32_header(to);
      receive_32_bit_data = TRUE;
      break;
    default:
      /*
       * unrecognized header style
       */
      if (errors) {
        return INVHDR;
      }
      continue;
      break;
    }
    if (errors && rxd_header_len == 0) {

      return INVHDR;
    }
  } while (rxd_header_len == 0);

  /*
   * this appears to have been a valid header.
   * return its type.
   */

  if (rxd_header[0] == ZDATA) {

    ack_file_pos = rxd_header[ZP0] | (rxd_header[ZP1] << 8) | (rxd_header[ZP2] << 16)
        | (rxd_header[ZP3] << 24);
  }
  if (rxd_header[0] == ZFILE) {
    ack_file_pos = 0l;
  }
  return rxd_header[0];
}

int
rx_header(int timeout) {
  int type = rx_header_raw(timeout, FALSE);
#if defined(ZMODEM_VIA_VC)
  debug_uart_puts("zmrx: Rec header type:%02X \r\n",type);
#elif defined(ZMODEM_VIA_DEBUG_UART)
  printf("zmrx: Rec header type:%02X \r\n",type);
#elif defined(ZMODEM_VIA_BLE)
  printf("zmrx: Rec header type:%02X \r\n",type);
#endif
  return type;
}

int
rx_header_and_check(int timeout) {
  int type;
  while (TRUE) {
    type = rx_header_raw(timeout, TRUE);

    if (type != INVHDR) {
      break;
    }
    tx_znak();
  }

  return type;
}
