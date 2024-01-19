#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

#ifndef __MACRO_IRQ_NUM__
#define __MACRO_IRQ_NUM__
#if defined(__ISA_X86__)
#define IRQ_TIMER 32
#elif defined(__riscv)
#define IRQ_TIMER 0x8000000000000007
#elif defined(__ISA_MIPS32__)
#define IRQ_TIMER 0
#elif defined(__ISA_LOONGARCH32R__)
#endif
#endif

#define KERNEL_MODE 3
#define USER_MODE 0


static Context* (*user_handler)(Event, Context*) = NULL;
void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

Context* __am_irq_handle(Context *c) {
  uintptr_t mscratch_val = 0;
  uintptr_t kas = 0;
  __asm__ __volatile__("csrr %0, mscratch"
    : "=r"(mscratch_val));
  c->np = (mscratch_val == 0 ? KERNEL_MODE : USER_MODE);
  __asm__ __volatile__("csrw mscratch, %0"
    :
    : "r"(kas));
  __am_get_cur_as(c);

  if (user_handler) {
    Event ev = {0};
    switch(c->mcause) {
      case MODE_M:
        if(c->GPR1 == -1)
          ev.event = EVENT_YIELD;
        else if(c->GPR1 <= 19 && c->GPR1 >= 0)
          ev.event = EVENT_SYSCALL;
        else {
          printf("\033[31mUnknown SYSCALL : %d\033[0m\n", c->GPR1);
          assert(0);
        }
        c->mepc += 4;
        break;
      case IRQ_TIMER:
        ev.event = EVENT_IRQ_TIMER;
        break;
      default :
       ev.event = EVENT_ERROR;
    }
    c = user_handler(ev, c);
    if(c == NULL) {
      printf("\033[31mFatal Error occured when execute user handler.\033[0m\n");
      assert(0);
    }
  }
  __am_switch(c);
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  uintptr_t *t0_buf = kstack.end - 4;
  *t0_buf = 0;

  Context *kctx = (Context *)(kstack.end - sizeof(Context));
  memset(kctx, 0, sizeof(kctx));
  kctx->GPRx = (uintptr_t)arg;
  kctx->mstatus = 0x1800 | 0x40;
  kctx->pdir = NULL;
  kctx->np = USER_MODE;
  kctx->mepc = (uintptr_t)entry;
  kctx->gpr[2]  = (uintptr_t)kstack.end - 4;
  return kctx;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
