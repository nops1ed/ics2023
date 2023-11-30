#include <common.h>

void do_syscall(Context*);
Context* schedule(Context *);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_SYSCALL:
      do_syscall(c);
      break;
    case EVENT_IRQ_TIMER:
      /* fall through here. */
      Log("\033[33mTimer interrupt emit\033[0m");
    case EVENT_YIELD:
      c = schedule(c);
      break;
    default: panic("\033[31mUnhandled event ID = %d\033[0m\n", e.event);
  }
  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
