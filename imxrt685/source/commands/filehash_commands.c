/*
 * filehash_commands.c
 *
 *  Created on: Apr 17, 2022
 *      Author: DavidWang
 */

#include <stdio.h>
#include "filehash_commands.h"
#include "filehash.h"
#include "binary_interface_inst.h"
#include "utils.h"

void filehash_sha256_command(int argc, char **argv){
  if (argc != 2) {
    printf("Error: Missing file path\n");
    printf("Usage: %s <filepath>\n", argv[0]);
    return;
  }

  unsigned char hashbuf[32];
  int status = filehash_sha256(argv[1], hashbuf);
  if(status == 0){
    for( size_t j = 0; j < 32; j++ )
    {
      printf( "%02x", hashbuf[j] );
    }
    printf("\n");
  }else{
      printf("error %d", status);
  }
}

void ble_filehash_sha256_command(int argc, char **argv){
  // The size of cbuf must be at least this large to fit the string hash: 32*2+1+1
  // 32*2 = 32 bytes represented in hex.
  // +1   = line feed characters
  // +1   = null character
  char cbuf[255];
  size_t cbuf_size;

  if (argc != 2) {
    cbuf_size = snprintf(cbuf,sizeof(cbuf),"Error: Missing file path\n");
    bin_itf_send_uart_command(cbuf,cbuf_size);
    cbuf_size = snprintf(cbuf,sizeof(cbuf),"Usage: %s <filepath>\n", argv[0]);
    bin_itf_send_uart_command(cbuf,cbuf_size);
    return;
  }

  unsigned char hashbuf[32];
  int status = filehash_sha256(argv[1], hashbuf);
  if(status == 0){
    configASSERT(sizeof(cbuf) >= 66);
    char* cbufptr = cbuf;
    cbuf_size = 0;
    for( size_t j = 0; j < sizeof(hashbuf); j++ )
    {
      cbuf_size += sprintf(cbufptr+cbuf_size,"%02x", hashbuf[j]);
    }
    cbuf_size += sprintf(cbuf+cbuf_size,"\n");
    bin_itf_send_uart_command(cbuf,cbuf_size);
  }else{
    cbuf_size = snprintf(cbuf,sizeof(cbuf),"error %d\n", status);
    bin_itf_send_uart_command(cbuf,cbuf_size);
  }
}
