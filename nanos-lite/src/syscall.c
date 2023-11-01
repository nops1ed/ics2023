#include <common.h>
#include "syscall.h"

#define CONFIG_STRACE

static int sys_yield() {
  yield();
  int ret_val = 0;
#ifdef CONFIG_STRACE
  printf("sys_yield(NULL) = %d\n", ret_val);
#endif
  return ret_val;  
}

static void sys_exit(int a) {
#ifdef CONFIG_STRACE
  printf("sys_exit(%d) = 0\n", a);
#endif
  halt(a);
}

size_t sys_write(int fd, const void *buf, size_t count) {
  unsigned long int stream = (long int)buf;
  int ret_val = -1;
  /* Indicate stdout/stderr and just call putch(). */
  if((fd == 1 || fd == 2) && count != 0)
    for(ret_val = 0; ret_val < count; ret_val++) {
      unsigned char __x = ((unsigned char *) stream)[0];
      stream++;
      /* Write to serial. */
      putch(__x);
    }
  else
    printf("sys_write: Error\n");

#ifdef CONFIG_STRACE
  printf("sys_write(%d, %p, %d) = %d\n", fd, buf, count, ret_val);
#endif
  return ret_val;
}

int sys_brk(void *addr) {
  return 0;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  //printf("do_syscall: %d\n", a[0]);
  switch (a[0]) {
    case SYS_yield: sys_yield();  break;
    case SYS_exit:  sys_exit((int)a[1]);  break;
    case SYS_write: sys_write((int)a[1], (const void*)a[2], (size_t)a[3]);  break;
    case SYS_brk:   sys_brk((void *)a[1]);  break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
