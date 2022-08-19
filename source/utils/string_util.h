/*
 * string_util.h
 *
 *  Created on: Mar 4, 2021
 *      Author: DavidWang
 */

#ifndef UTILS_STRING_UTIL_H_
#define UTILS_STRING_UTIL_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int readable_char(char* buf, uint32_t buf_size, char c);
int readable_cstr(char* buf, uint32_t buf_size, char* cstr);

size_t str_append(char* dest, size_t dest_size, const char* src, size_t src_size);
size_t str_append2(char* dest, size_t dest_size, const char* src);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_STRING_UTIL_H_ */
