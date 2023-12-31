/*
 * zmdm.h
 * zmodem primitives prototypes and global data
 * (C) Mattheij Computer Service 1994
 */

#ifndef _ZMDM_H

#define _ZMDM_H

#ifdef ZMDM
#define EXTERN
#else
#define EXTERN extern
#endif

#include <errno.h>
#include "zmodem.h"
#include "ff.h"

#define TRUE  1
#define FALSE 0

#define ENDOFFRAME 2
#define FRAMEOK    1
#define TIMEOUT   -1    /* rx routine did not receive a character within timeout */
#define INVHDR    -2    /* invalid header received; but within timeout */
#define INVDATA   -3    /* invalid data subpacket received */
#define ZDLEESC 0x8000  /* one of ZCRCE; ZCRCG; ZCRCQ or ZCRCW was received; ZDLE escaped */

#define HDRLEN     5    /* size of a zmodme header */

void
tx_hheader(unsigned char *buf, int n);

void
tx_bheader(unsigned char *buf, int n);

int
rx_header(int to); /* receive any header with timeout in milliseconds */

void
rx_purge(void);
void
tx_raw(int c);
void
tx_esc(int c);
void
tx(unsigned char c);
void
tx_flush(void);
void
tx_nibble(int n);
void
tx_hex(int h);
void
tx_hex_header(unsigned char *p);
void
tx_bin32_header(unsigned char *p);
void
tx_bin16_header(unsigned char *p);
void
tx_header(unsigned char *p);
void
tx_32_data(int sub_frame_type, unsigned char *p, int l);
void
tx_16_data(int sub_frame_type, unsigned char *p, int l);
void
tx_data(int sub_frame_type, unsigned char *p, int l);
void
tx_pos_header(int type, long pos);
void
tx_znak(void);
int
rx_poll(void);
int
rx_raw(int to);
int
rx(int to);
int
rx_32_data(unsigned char *p, int *l);
int
rx_16_data(register unsigned char *p, int *l);
int
rx_data(unsigned char *p, int *l);
int
rx_nibble(int to);
int
rx_hex(int to);
void
rx_bin16_header(int to);
void
rx_hex_header(int to);
void
rx_bin32_header(int to);
int
rx_header_raw(int to, int errors);
int
rx_header_and_check(int timeout);
void
cleanup(void);
int
receive_file_data(char *name, FIL *fp);
void
tx_zrinit(void);
int
receiveFile(FILE*, char*, int, int);

#endif

