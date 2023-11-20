#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
/*
  |               |
  +---------------+ <---- ustack.end
  |  Unspecified  |
  +---------------+
  |               | <----------+
  |    string     | <--------+ |
  |     area      | <------+ | |
  |               | <----+ | | |
  |               | <--+ | | | |
  +---------------+    | | | | |
  |  Unspecified  |    | | | | |
  +---------------+    | | | | |
  |     NULL      |    | | | | |
  +---------------+    | | | | |
  |    ......     |    | | | | |
  +---------------+    | | | | |
  |    envp[1]    | ---+ | | | |
  +---------------+      | | | |
  |    envp[0]    | -----+ | | |
  +---------------+        | | |
  |     NULL      |        | | |
  +---------------+        | | |
  | argv[argc-1]  | -------+ | |
  +---------------+          | |
  |    ......     |          | |
  +---------------+          | |
  |    argv[1]    | ---------+ |
  +---------------+            |
  |    argv[0]    | -----------+
  +---------------+
  |      argc     |
  +---------------+ <---- cp->GPRx
  |               |
*/

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  int argc = *((int *)args); 
  printf("So the argc is %d\n",argc);
  char **argv = (char **)args + 1; 
  printf("CTE0: Argv[0] is %s\n",argv[0]);
  printf("CTE0: Argv[1] is %s\n",argv[1]);
  char **envp = argv + argc;
  while(*envp != NULL) envp++;
  environ = envp;
  exit(main(argc, argv, envp));
  assert(0);

/*
  char *empty[] =  {NULL };
  environ = empty;
  exit(main(0, empty, empty));
  assert(0);
  */
}