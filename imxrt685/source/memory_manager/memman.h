
#ifndef MEMORY_MANAGER_MEMMAN_H_
#define MEMORY_MANAGER_MEMMAN_H_

#include <stdio.h>
#include "../memory_manager/memlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  /* root and nil node of Red-black tree, which will be allocated in heap */
  void *rb_root, *rb_null;
  memlib_t mem;
} mm_t;

int mm_init (mm_t* mm, void* buf, size_t buf_size);
void *mm_malloc (mm_t* mm, size_t size);
void mm_free (mm_t* mm, void *ptr);
void mm_exit (mm_t* mm);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_MANAGER_MEMMAN_H_ */
