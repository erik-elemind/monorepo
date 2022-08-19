/*
 * filehash.c
 *
 *  Created on: Apr 16, 2022
 *      Author: DavidWang
 */

#include "config.h"
#include "sha256.h"
#include "filehash.h"
#include "ff.h"
#include "fatfs_utils.h"


#define HASH_BUFFER_SIZE 256

// Computes the hash of filename
// hash - buffer to store hash, must be 32 bytes
// hash_size - size of hash buffer
// returns 0 on success
int filehash_sha256(const char *filename, unsigned char *hashbuf) {

   FRESULT result;
   FIL file = {0};
   UINT bytes_read;
   sha256_t hash;
   uint8_t filebuf[HASH_BUFFER_SIZE];

   // Open the file for reading:
   result = f_open(&file, filename, FA_READ);
   if (FR_OK != result){
     return -1;
   }

   // Initiate the hash
   sha256_init(&hash);

   // Iterate over the file computing the hash
   while( true )
   {
     result = f_read(&file, filebuf, HASH_BUFFER_SIZE, &bytes_read);
     if (FR_OK != result){
       // close the file
       if (f_is_open(&file)) {
         f_close(&file);
       }
       return -1;
     }
     if(bytes_read != 0){
       sha256_update(&hash, filebuf, bytes_read);
     }else{
       break;
     }
   }

   // Output the hash to the output hashbuf buffer
   sha256_final(&hash, hashbuf);

   // Close the file
   if (f_is_open(&file)) {
     f_close(&file);
   }

  return 0;
}

