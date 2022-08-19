
#ifndef MEMORY_MANAGER_MEMLIB_H_
#define MEMORY_MANAGER_MEMLIB_H_

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  /* private variables */
  char *mem_start_brk;  /* points to first byte of heap */
  char *mem_brk;        /* points to last byte of heap */
  char *mem_max_addr;   /* largest legal heap address */
} memlib_t;

void mem_init(memlib_t* memlib, void* buf, size_t buf_size);
void mem_deinit(memlib_t* memlib);
void *mem_sbrk(memlib_t* memlib, int incr);
void mem_reset_brk(memlib_t* memlib);
void *mem_heap_lo(memlib_t* memlib);
void *mem_heap_hi(memlib_t* memlib);
size_t mem_heapsize(memlib_t* memlib);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_MANAGER_MEMLIB_H_ */
