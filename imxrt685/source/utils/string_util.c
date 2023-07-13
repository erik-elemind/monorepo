/*
 * string_util.c
 *
 *  Created on: Mar 4, 2021
 *      Author: DavidWang
 */


#include "string_util.h"
#include <stdbool.h>
#include <stdio.h>
#include "string.h"

int readable_char(char* buf, uint32_t buf_size, char c){
  if(c>=32 && c<=126){
    return snprintf(buf,buf_size,"%c",c);
  }else{
    return snprintf(buf,buf_size,"[%d]",c);
  }
}


int readable_cstr(char* buf, uint32_t buf_size, char* cstr){
  uint32_t     i = 0;
  int total_size = 0;
  while (true) {
    char c = cstr[i];
    if(cstr[i]=='\0') {
      *buf = '\0';
      return total_size;
    } else {
      int write_size = readable_char(buf,buf_size,c);
      buf        += write_size;
      buf_size   -= write_size;
      total_size += write_size;
    }
    i++;
  }
}


// Appends src string to dest string.
// The "dest_size"/"src_size" values should count all characters except the
// terminating null character of the "dest"/"src" string, respectively.
// i.e. If dest = "apple", then dest_size = 5.
// The appended result is returned in the "dest" buffer, with a null-character added at the end.
// The size of the resulting string is returned (not counting the terminating null character).
size_t str_append(char* dest, size_t dest_size, const char* src, size_t src_size){
  memcpy(dest+dest_size, src, src_size);
  dest[ dest_size + src_size ] = '\0';
  return dest_size + src_size;
}

size_t str_append2(char* dest, size_t dest_size, const char* src){
  return str_append(dest, dest_size, src, strlen(src));
}
