#include <common.h>
#include "syscall.h"
void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case SYS_yield: break;
    //case SYS_exit:  SYS_exit();  break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}

/*
void SYS_yield() {
  
}

void SYS_exit() {
  //halt();
}
*/