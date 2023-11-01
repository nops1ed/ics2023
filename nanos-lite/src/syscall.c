#include <common.h>
#include "syscall.h"

#define CONFIG_STRACE 1

int sys_yield() {
  yield();
  int ret_val = 0;
#ifdef CONFIG_STRACE
  printf("sys_yield(NULL) = %d\n", ret_val);
#endif
  return ret_val;  
}

void sys_exit(int a) {
#ifdef CONFIG_STRACE
  printf("sys_exit(%d) = 0\n", a);
#endif
  halt(a);
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  switch (a[0]) {
    case SYS_yield: sys_yield(); break;
    case SYS_exit:  sys_exit((int)c->GPR2);  break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
