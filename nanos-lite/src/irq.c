#include <common.h>

void do_syscall(Context*);

static Context* do_event(Event e, Context* c) {
  printf("Nano: event is %d\n", e.event);
  switch (e.event) {
    case EVENT_SYSCALL:
      Log("Event SYSCALL emit");
      do_syscall(c);
      break;
    case EVENT_YIELD:
      printf("Nano: Event yield emit\n");
      break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
