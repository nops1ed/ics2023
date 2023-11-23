#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("%s:%s$", __USERNAME__, getenv("PATH"));
}

/*
static void sh_handle_cmd(const char *cmd) {
  int pos = strcspn(cmd, "\n");
  char *new_str = (char *)malloc(pos + 1); 
  strncpy(new_str, cmd, pos); 
  new_str[pos] = '\0'; 
  int argc = 1;
  for(int i = 0; *(new_str + i); i++)
    if(*(new_str + i) == ' ') 
      argc += 1;
    printf("New_str is %s\n", new_str);
  char **argv = (char **)malloc(sizeof(char *) * argc);
  printf("So argc is %d\n", argc);
  *argv = strtok(new_str, " ");
  for(int i = 1; i < argc; i++) {
    *(argv + i) = strtok(NULL, " ");
    printf("So argv[%d] is %s\n", i, *(argv + i));
  }
  for(int i = 0; i < argc; i++) 
    printf("argv[%d] is %s\n", i, argv[i]);
  printf("Finished ...\n");
  execvp(argv[0], argv);
  for(int i = 0; i < argc; i++) 
    free(*(argv + i));
  free(argv);
  free(new_str);
}

*/
static size_t get_argc(char *str)
{
  size_t i = 0;
  if (strtok(str, " ") == NULL)
    return i;
  else
    i++;

  while (strtok(NULL, " ") != NULL)
    i++;
  return i;
}

static void get_argv(char *cmd, char **argv)
{
  int argc = 0;

  // printf("cmd is %s\n", cmd);
  argv[argc++] = strtok(cmd, " ");
  // printf("argv0 %s\n", argv[0]);

  while (true)
  {
    argv[argc++] = strtok(NULL, " ");
    if (argv[argc - 1] == NULL)
      break;
    // printf("argv%d %s\n", argc, argv[argc]);
  }
  return;
}

static void sh_handle_cmd(const char *cmd)
{
  printf("\n");
  // printf("here\n");
  // printf("cmd is %s\n", cmd);
  if (cmd[0] == '\n')
    return;

  char cmd_cpy[strlen(cmd) + 1];
  char *cmd_extract = strtok(strcpy(cmd_cpy, cmd), "\n");
  char *cmd_name = strtok(cmd_extract, " ");
  char *args = strtok(NULL, "");
  // char *cmd_end = cmd_extract + strlen(cmd_extract);

  // char *args = cmd_name + strlen(cmd_name) + 1;
  // printf("cmd is %s, args is %s\n", cmd_name, args);
  // if (args > cmd_end)
  //   args = NULL;
  cmd_extract = strtok(strcpy(cmd_cpy, cmd), "\n");
  int argc = get_argc(cmd_extract);
  // printf("argc is %d\n", argc);

  char *(argv[argc + 1]) = {NULL};
  cmd_extract = strtok(strcpy(cmd_cpy, cmd), "\n");

  get_argv(cmd_extract, argv);
  // printf("argv0 is %s\n", argv[0]);

  if (execvp(argv[0], argv) < 0)
    sh_printf("sh: command not found: %s\n", argv[0]);

  return;
}


void builtin_sh_run() {
  setenv("PATH", "/bin/", 0);
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
