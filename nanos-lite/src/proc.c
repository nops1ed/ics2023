#include <proc.h>
#include <fs.h>
#include <common.h>

#define MAX_NR_PROC 4

void naive_uload(PCB *pcb, const char *filename);

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
}

Context *context_kload(PCB* pcb, void(*func)(void *), void *args) {
  Area stack = RANGE((char *)(uintptr_t)pcb, (char *)(uintptr_t)pcb + STACK_SIZE);
  Context *kcxt = kcontext(stack, func, args);
  return kcxt;
} 

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    //Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    Log("Message from %s", (uintptr_t)arg);
    j ++;
    yield();
  }
}

void init_proc() {
  context_kload(&pcb[0], hello_fun, "proc0");
  context_kload(&pcb[1], hello_fun, "proc1");
  switch_boot_pcb();

  Log("Initializing processes...");
  // load program here
  naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  return current->cp;
}
