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

static void sh_handle_cmd(const char *cmd) {
  int pos = strcspn(cmd, "\n");
  char *new_str = (char *)malloc(pos + 1); 
  strncpy(new_str, cmd, pos); 
  new_str[pos] = '\0'; 
  /* Parse arguments list. */ 
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
