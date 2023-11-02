#include <common.h>
#include "fs.h"
#include "syscall.h"

#define CONFIG_STRACE

static void sys_yield(Context *c) {
  yield();
  intptr_t ret_val = 0;
#ifdef CONFIG_STRACE
  printf("sys_yield(NULL) = %d\n", ret_val);
#endif
  c->GPRx= ret_val;
}

static void sys_exit(Context *c) {
#ifdef CONFIG_STRACE
  printf("sys_exit(%d) = 0\n", c->GPR2);
#endif
  halt(c->GPR2);
}

static void sys_write(Context *c) {
  int ret_val = fs_write((int)c->GPR2, (const void *)c->GPR3, (size_t)c->GPR4);
#ifdef CONFIG_STRACE
  printf("sys_write(%d, %p, %d) = %d\n", c->GPR2, c->GPR3, c->GPR4, ret_val);
#endif
  c->GPRx = ret_val;
}

static void sys_read(Context *c) {
  panic("Not implement");
}

static void sys_lseek(Context *c) {
  panic("Not implement");
}

static void sys_open(Context *c) {
  panic("Not implement");
}

static void sys_close(Context *c) {
  panic("Not implement");
}

static void sys_brk(Context *c) {
  c->GPRx = 0;
}

static void sys_kill(Context *c) {
  panic("Not implement");
}

static void sys_getpid(Context *c) {
  panic("Not implement");
}


void (*syscall_table[])() = {
  &sys_exit, &sys_yield, &sys_open, &sys_read, &sys_write, &sys_kill,
  &sys_getpid, &sys_close, &sys_lseek, &sys_brk,
};

#define NR_SYST sizeof(syscall_table) / sizeof(syscall_table[0])

void do_syscall(Context *c) {
  uintptr_t a = c->GPR1;
    if(a >= 0 && a < NR_SYST)
      syscall_table[a](c);
    else 
      panic("Unhandled syscall ID = %d", a);
}
