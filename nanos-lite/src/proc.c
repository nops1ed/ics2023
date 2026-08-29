#include <proc.h>
#include <fs.h>
#include <device.h>
#include <common.h>

#define MAX_NR_PROC 4

void naive_uload(PCB *pcb, const char *filename);
Context *context_kload(PCB* pcb, void(*func)(void *), void *args);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);

static char *args_pal[] __attribute__((used)) = {"/bin/pal", "--skip", NULL};
static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;
static int time_chip __attribute__((used));
static int proc_running __attribute__((used)) = 1;
extern int fg_pcb;

void switch_boot_pcb() {
  current = &pcb_boot;
}

bool select_fg_pcb(int index) {
  if (index < 1 || index >= MAX_NR_PROC || pcb[index].cp == NULL) {
    return false;
  }
  fg_pcb = index;
  return true;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    //Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    //Log("HEllo");
    j ++;
    yield();
  }
}

void init_proc() {

  //context_uload(&pcb[0], "/bin/menu", NULL, NULL);
  context_uload(&pcb[0], "/bin/hello", NULL, NULL);
  context_uload(&pcb[1], "/bin/pal", args_pal, NULL);
  //context_uload(&pcb[1], "/bin/nterm", NULL, NULL);
  //context_uload(&pcb[2], "/bin/bird", NULL, NULL);
  //Log("Before menu created");
  //context_uload(&pcb[3], "/bin/menu", NULL, NULL);
  switch_boot_pcb();

  Log("Initializing processes...");
  // load program here
  //naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
   static int prio_count = 0;
  current->cp = prev;
  if (fg_pcb < 1 || fg_pcb >= MAX_NR_PROC || pcb[fg_pcb].cp == NULL) {
    fg_pcb = 1;
  }
  assert(pcb[fg_pcb].cp != NULL);
  if (prio_count < 100) {
    prio_count ++;
    current = &pcb[fg_pcb];
  } else {
    prio_count = 0;
    current = &pcb[0];
  }
  Log("schedule %p(updir %p) -> %p(updir %p)", prev, prev->pdir, current->cp, current->cp->pdir);
  return current->cp;
}

void schedule_proc(int index) {
  if(index == proc_running)
    return;
  switch_boot_pcb();
  proc_running = index;
  pcb[0].cp->pdir = NULL;
  yield();
}