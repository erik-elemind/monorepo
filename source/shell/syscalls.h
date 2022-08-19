#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#ifdef __cplusplus
extern "C" {
#endif


enum {
  SYSCALL_NONE = 0,
  SYSCALL_USB = 1,
  SYSCALL_DEBUG_UART = 2,
};


void syscalls_set_write_loc(uint32_t write_loc);
uint32_t syscalls_get_write_loc();
void syscalls_set_read_loc(uint32_t read_loc);
uint32_t syscalls_get_read_loc();

void syscalls_pretask_init(void);

// NOTE: _read() and _write() are defined by newlib-nano, and are weakly linked.

#ifdef __cplusplus
}
#endif

#endif /* __SYSCALLS_H__ */
