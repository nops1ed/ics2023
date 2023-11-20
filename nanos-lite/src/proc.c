#include <proc.h>
#include <fs.h>
#include <common.h>

#define MAX_NR_PROC 4

void naive_uload(PCB *pcb, const char *filename);
Context *context_kload(PCB* pcb, void(*func)(void *), void *args);
void context_uload(PCB *pcb, const char *filename);

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
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
  //context_kload(&pcb[0], hello_fun, "Message from proc #1");
  context_uload(&pcb[0], "/bin/hello"); 
  context_uload(&pcb[1], "/bin/pal"); 
  //context_kload(&pcb[1], hello_fun, "proc1");
  switch_boot_pcb();

  Log("Initializing processes...");
  // load program here
  //naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  return current->cp;
}
