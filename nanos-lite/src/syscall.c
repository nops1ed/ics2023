#include <common.h>
#include "syscall.h"

int sys_yield() {
  yield();
  return 0;  
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  printf("syscall: a[0] has val %d\n", a[0]);
  switch (a[0]) {
    case 0: sys_yield(); break;
    //case SYS_exit:  SYS_exit();  break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
