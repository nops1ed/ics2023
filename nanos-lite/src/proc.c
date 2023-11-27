#include <proc.h>
#include <fs.h>
#include <common.h>

#define MAX_NR_PROC 4

void naive_uload(PCB *pcb, const char *filename);
Context *context_kload(PCB* pcb, void(*func)(void *), void *args);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);

//static char *args_pal[] = {"/bin/pal", "--skip", NULL};
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
    //Log("Message from %s", (uintptr_t)arg);
    j ++;
    yield();
  }
}

void init_proc() {
  printf("\033[31mtraping here...\033[0m\n");
  //context_kload(&pcb[0], hello_fun, "Message from proc #1");
  printf("\033[31mpcb 0 finished ...\033[0m\n");
  //context_uload(&pcb[0], "/bin/hello"); 
  //context_uload(&pcb[1], "/bin/pal", args_pal, NULL); 
  context_uload(&pcb[0], "/bin/nterm", NULL, NULL); 
  printf("\033[31mpcb 1 finished ...\033[0m\n");
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
