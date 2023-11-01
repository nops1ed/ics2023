#include <common.h>
#include "syscall.h"

int sys_yield() {
  printf("nano: In sys_yield now\n");
  yield();
  return 1145;  
}

void sys_exit() {
  panic("No exit function defined here.");
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  printf("syscall: a[0] has val %d\n", a[0]);
  printf("syscall: a[1] has val %d\n", a[1]);
  printf("syscall: a[2] has val %d\n", a[2]);
  printf("syscall: a[3] has val %d\n", a[3]);
  switch (a[0]) {
    case SYS_yield: sys_yield(); break;
    case SYS_exit:  sys_exit();  break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
