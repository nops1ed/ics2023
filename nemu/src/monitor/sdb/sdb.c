/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {	
  //parse args
  char *arg = strtok(NULL," ");
  int i = 0;
  if (!arg)	i = 1;
	else sscanf(arg , "%d" , &i);
  cpu_exec(i);	  
  return 0;
}

static int cmd_info(char *args) {
  //parse args
  char *arg = strtok(NULL , " ");
  //Note: feature of fall through
  if (!strcmp(arg , "r"))
		isa_reg_display();
  else if (!strcmp(arg , "w"))
    sdb_watchpoint_display();
  else if (!strcmp(arg , "f")) {
    char buf[64];
    //getline(buf , 64 , 0);
    bool success = true;
    word_t val = expr(buf , &success);
    if (!success) {
      printf("Error\n");
      return 0;
    }
    else printf("The val is %d\n" , val);
  }
  else {
    printf("Undefined info command: \"%s\".  Try \"help info\".\n" , arg);
  }
  return 0;
}

static int cmd_x(char *args) {
  char *arg = strtok(NULL , " ");
  int i = 0;
  if (!arg) {
    printf("Arguments parse failed.\n");
    return 0;
  } 
  sscanf(arg , "%d" , &i);
  arg = strtok(NULL , "\0");
  if (!arg) {
    printf("Arguments parse failed.\n");
    return 0;
  }
  bool success = true;
  vaddr_t addr = expr(arg , &success);
  if (!success) {
    printf("A syntax error in expression, near '%s'.\n" , arg);
    return 0;
  }
  for (int j = 0 ; j < i ; j++) {
    //printf("0x%x: %08x\n" , addr + 4 * j, vaddr_read(addr + 4 * j , 4));
		printf("0x%x: " , addr + 4 * j);
		for (int k = 3 ; k >= 0 ; k--)
			//Little endian
			printf("%02x " , vaddr_read(addr + 4 * j + k, 1));		
		printf("\n");
	}
  return 0;
}

static int cmd_p(char *args) {
  char *arg = strtok(NULL , "\0");
  if (!arg) {
    printf("Missing Arguments. \n");
    return 0;
  }
  bool success = true;
  word_t val = expr(arg , &success);
  if (!success) {
    printf("A syntax error in expression, near '%s'.\n" , arg);
    return 0;
  }
  printf("%-10s : 0x%-5x\n" ,arg , val);
  return 0;
}

static int cmd_w(char *args) {
  char *arg = strtok(NULL , "\0");
  if (!arg) {
    printf("Missing Arguments. \n");
    return 0;
  }
  int wp_no = sdb_watchpoint_create(arg);
  if (wp_no == -1) return 0;
  printf("Hardware watchpoint %d: %s\n" , wp_no , arg);
  return 0;
}

static int cmd_d(char *args) {
  char *arg = strtok(NULL , "\0");
  /* Default behavior indicates deleting all WPs */
  if (!arg) {
    printf("Delete all breakpoints? (y or n) ");
    char buf[128];
    if(!fgets(buf , sizeof(buf) , stdin)) {
      puts("Valid Arguments\n");
      return 0;
    }
    /* It does not work on Linux */
    //fflush(0);
    //if(!scanf("%*[^\n] %*s"));
    char c = buf[0];
    switch(c) {
      case 'Y':
      case 'y':
        sdb_watchpoint_delete_all();
        return 0;
      case 'N':
      case 'n':
        return 0;
      default:
        printf("\nUndefined choice %c\n" , c);
        return 0;
    }
  }
  int wp_no;
  sscanf(arg , "%d" , &wp_no);
  sdb_watchpoint_delete(wp_no);
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si" , "Execute by single step" , cmd_si },
  { "info" , "Show information" , cmd_info },
  { "x" , "Scan Memory" , cmd_x },
  { "p" , "Evaluate the expression" , cmd_p },
  { "w" , "Set watchpoint" , cmd_w },
  { "d" , "Delete watchpoint" , cmd_d },
//{ "b" , "Set breakpoint" , cmd_b },
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    puts("List of classes of commands:\n");
    for (i = 0; i < NR_CMD; i ++) {
      printf("%-5s - %-20s\n", cmd_table[i].name, cmd_table[i].description);
    }
    puts("\nType \"help\" followed by command name for full documentation.");
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%-5s - %-20s\n", cmd_table[i].name, cmd_table[i].description);
        //TODO
        puts("Full descriptions todo ...");
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}


void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
