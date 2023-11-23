#include <common.h>
#include <proc.h>
#include <fs.h>
#include "syscall.h"

#define CONFIG_STRACE
void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
void switch_boot_pcb();

typedef struct timeval {
  int64_t tv_sec;     // seconds
  int32_t tv_usec;    // microseconds
}timeval;

static void sys_yield(Context *c) {
  yield();
  intptr_t ret_val = 0;
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_yield(NULL) = %d\n", ret_val);
#endif
  c->GPRx= (intptr_t)ret_val;
}

static void sys_write(Context *c) {
  int ret_val = fs_write((int)c->GPR2, (const void *)c->GPR3, (size_t)c->GPR4);
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_write(%d, %p, %d) = %d\n", c->GPR2, c->GPR3, c->GPR4, ret_val);
#endif
  c->GPRx = (intptr_t)ret_val;
}

static void sys_read(Context *c) {
  size_t ret_val = fs_read((int)c->GPR2, (void *)c->GPR3, (size_t)c->GPR4);
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_read(%d, %p, %d) = %d\n", c->GPR2, c->GPR3, c->GPR4, ret_val);
#endif
  c->GPRx = (intptr_t)ret_val;
}

static void sys_lseek(Context *c) {
  size_t ret_val = fs_lseek((int)c->GPR2, (size_t)c->GPR3, (int)c->GPR4);
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_lseek(%d, %p, %d) = %d\n", c->GPR2, c->GPR3, c->GPR4, ret_val);
#endif
  c->GPRx = (intptr_t)ret_val;
}

static void sys_open(Context *c) {
  int ret_val = fs_open((const char *)c->GPR2, (int)c->GPR3, (int)c->GPR4);
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_open(%d, %p, %d) = %d\n", c->GPR2, c->GPR3, c->GPR4, ret_val);
#endif
  c->GPRx = (intptr_t)ret_val;
}

static void sys_close(Context *c) {
  int ret_val = fs_close((int)c->GPR2);
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_close(%d) = %d\n", c->GPR2, ret_val);
#endif
  c->GPRx = (intptr_t)ret_val;
}

static void sys_brk(Context *c) {
  c->GPRx = 0;
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_brk(NULL) = %d\n", c->GPRx);
#endif
}

static void sys_kill(Context *c) {
  panic("Not implement");
}

static void sys_getpid(Context *c) {
  panic("Not implement");
}

static void sys_fstat(Context *c) {
  panic("Not implement");
}

static void sys_time(Context *c) {
  panic("Not implement");
}

static void sys_signal(Context *c) {
  panic("Not implement");
}

static void sys_execve(Context *c) { 
  context_uload(current, (const char *)c->GPR2, (char **const)(uintptr_t)c->GPR3, (char **const)(uintptr_t)c->GPR4);
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_execve(%s, %s, %s)  \n", c->GPR2, c->GPR3, c->GPR4);
#endif
  switch_boot_pcb();
  yield();
}

static void sys_exit(Context *c) {
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_exit(0) = 0\n");
#endif
  printf("sys_exit(0) = 0\n");
  naive_uload(NULL, "/bin/menu");
}

static void sys_fork(Context *c) {
  panic("Not implement");
}

static void sys_link(Context *c) {
  panic("Not implement");
}

static void sys_unlink(Context *c) {
  panic("Not implement");
}

static void sys_wait(Context *c) {
  panic("Not implement");
}

static void sys_times(Context *c) {
  panic("Not implement");
}

static void sys_gettimeofday(Context *c) {
  int ret_val = 0;
  ioe_read(AM_TIMER_UPTIME, &(((timeval *)c->GPR2)->tv_usec));
  ((timeval *)c->GPR2)->tv_sec = (int32_t)(((timeval *)c->GPR2)->tv_usec / 1000000);
  c->GPRx = ret_val;
#ifdef CONFIG_STRACE
  fs_curfilename();
  printf("sys_gettimeofday() = %d\n", c->GPRx);
#endif
}

void (*syscall_table[])() = {
  &sys_exit, &sys_yield, &sys_open, &sys_read, &sys_write, &sys_kill,
  &sys_getpid, &sys_close, &sys_lseek, &sys_brk, &sys_fstat, &sys_time,
  &sys_signal, &sys_execve, &sys_fork, &sys_link, &sys_unlink, &sys_wait,
  &sys_times, &sys_gettimeofday,
};

#define NR_SYST (sizeof(syscall_table) / sizeof(syscall_table[0]))

void do_syscall(Context *c) {
  uintptr_t a = c->GPR1;
  if(a >= 0 && a < NR_SYST)
    syscall_table[a](c);
  else 
    panic("Unhandled syscall ID = %d", a);
}
