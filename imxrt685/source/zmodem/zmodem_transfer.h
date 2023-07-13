/*
 * zmdm.h
 * zmodem primitives prototypes and global data
 * (C) Mattheij Computer Service 1994
 */

#ifndef _ZMODEM_TRANSFER_H

#define _ZMODEM_TRANSFER_H

//#define ZMODEM_VIA_DEBUG_UART
//#define ZMODEM_VIA_VC
#define ZMODEM_VIA_BLE

int
zmodem_send_file(char *filename);
long
zmodem_receive_file();

#endif

