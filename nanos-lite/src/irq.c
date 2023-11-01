#include <common.h>

void do_syscall(Context*);
static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_SYSCALL:
      do_syscall(c);
      break;
    case EVENT_YIELD:
      printf("Huh,seems like you trap here successfully\n");
      break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
