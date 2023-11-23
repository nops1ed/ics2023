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
  //sh_printf("%s:%s$", __USERNAME__, getenv("PATH"));
  sh_printf("sh> ");
}

static void sh_handle_cmd(const char *cmd) {
  int pos = strcspn(cmd, "\n");
  char *new_str = (char *)malloc(pos + 1); 
  strncpy(new_str, cmd, pos); 
  new_str[pos] = '\0'; 
  char *__tmp = (char *)malloc(sizeof(char) * pos + 1);

  /* Need to handle bulid-in commands here. */
  //TODO()

  /* Parse arguments count. */
  int argc = 1;
  strcpy(__tmp, new_str);
  strtok(__tmp, " ");
  while(strtok(NULL, " ")) argc++;

  /* Parse arguments list. */ 
  char *argv[argc + 1] = {NULL};
  strcpy(__tmp, new_str);
  argv[0] = strtok(__tmp, " ");
  for(int i = 1; i < argc; i++) {
    argv[i] = strtok(NULL, " ");
    if(argv[i] == NULL) break;
  }

  if(execvp(argv[0], argv) == -1)
    sh_printf("%s: command not found\n", argv[0]);
  free(new_str);
  free(__tmp);
}

void builtin_sh_run() {
  //setenv("PATH", "/bin/", 0);
  sh_banner();
  sh_prompt();
  setenv("PATH", "/bin/", 0);
  setenv("PATH", "/usr/bin/", 0);
  printf("%s\n", getenv("PATH"));
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
