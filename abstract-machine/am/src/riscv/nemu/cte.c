#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;
void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

Context* __am_irq_handle(Context *c) {
  printf("Traping into irq_handle\n");
  __am_get_cur_as(c);
  if (user_handler) {
    Event ev = {0};
    /* All of the interrupts will be treated as MODE_MACHINE. */
    if(c->mcause == MODE_M) {
      switch(c->GPR1) {
        case -1:
          ev.event = EVENT_YIELD;
          break;
        case 0 ... 19:
          ev.event = EVENT_SYSCALL;
          break;
        default:
          ev.event = EVENT_ERROR;
      }
      c->mepc += 4;
    }
    else {
      printf("\033[33mUser/Supervisor mode is not supported\033[0m\n");
      assert(0);
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
  Context *kctx = (Context *)(kstack.end - sizeof(Context)); 
  memset(kctx, 0, sizeof(kctx));
  kctx->gpr[10] = (uintptr_t)arg;
  kctx->mepc = (uintptr_t)entry;
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
