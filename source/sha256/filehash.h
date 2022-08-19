/*
 * filehash.h
 *
 *  Created on: Apr 16, 2022
 *      Author: DavidWang
 */

#ifndef SHA256_FILEHASH_H_
#define SHA256_FILEHASH_H_

#ifdef __cplusplus
extern "C" {
#endif

int filehash_sha256(const char *filename, unsigned char *hashbuf);

#ifdef __cplusplus
}
#endif

#endif /* SHA256_FILEHASH_H_ */
