/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 */
#include "../memory_manager/memlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "config.h"

/* 
 * mem_init - initialize the memory system model
 */
void mem_init(memlib_t* memlib, void* buf, size_t buf_size)
{
    memlib->mem_start_brk = buf;
    memlib->mem_max_addr = memlib->mem_start_brk + buf_size;  /* max legal heap address */
    memlib->mem_brk = memlib->mem_start_brk;                  /* heap is empty initially */
}


/*
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit(memlib_t* memlib)
{
    // do nothing
}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk(memlib_t* memlib)
{
  memlib->mem_brk = memlib->mem_start_brk;
}

/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
void *mem_sbrk(memlib_t* memlib, int incr)
{
    char *old_brk = memlib->mem_brk;

    if ( (incr < 0) || ((memlib->mem_brk + incr) > memlib->mem_max_addr)) {
	errno = ENOMEM;
//	fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
	return (void *)-1;
    }
    memlib->mem_brk += incr;
    return (void *)old_brk;
}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo(memlib_t* memlib)
{
    return (void *)(memlib->mem_start_brk);
}

/* 
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi(memlib_t* memlib)
{
    return (void *)(memlib->mem_brk - 1);
}

/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize(memlib_t* memlib)
{
    return (size_t)(memlib->mem_brk - memlib->mem_start_brk);
}
