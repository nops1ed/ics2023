#include <proc.h>
#include <fs.h>
#include <common.h>

#define MAX_NR_PROC 4

void naive_uload(PCB *pcb, const char *filename);
Context *context_kload(PCB* pcb, void(*func)(void *), void *args);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);

static char *args_pal[] __attribute__((unused)) = {"/bin/pal", "--skip", NULL};
static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;
static int time_chip __attribute__((unused));

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

void init_proc() {
 
  //context_uload(&pcb[0], "/bin/menu", NULL, NULL); 
  context_kload(&pcb[0], hello_fun, NULL); 
  context_uload(&pcb[1], "/bin/pal", args_pal, NULL); 
  switch_boot_pcb();

  Log("Initializing processes...");
  // load program here
  //naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
  //printf("\033[33mschedule: Traping here...\033[0m\n");
  current->cp = prev;
  /*
  time_chip++;
  if(time_chip > 100) {
    current = &pcb[0];
    time_chip = 0;
  }
  else 
    current = &pcb[1];
  */
  current = &pcb[1];
    //current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  //printf("\033[33mFinished...\033[0m\n");
  return current->cp;
}