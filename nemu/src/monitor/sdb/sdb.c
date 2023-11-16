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

#define _FILENAME_SIZE 256

static int is_batch_mode = false;
static FILE *snaplist_file = NULL;

void isa_snapshot_save(FILE *);
void isa_snapshot_load(FILE *);
void pmem_snapshot_save(FILE *);
void pmem_snapshot_load(FILE *);
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
  /* parse args */
  char *arg = strtok(NULL , " ");
  if (!strcmp(arg , "r")) {
		isa_reg_display();
    //isa_csr_display();
  }
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
    else printf("The val is %ld\n" , val);
  }
  else {
    printf("Unknown info command: \"%s\".  Try \"help info\".\n" , arg);
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
		printf("0x%lx: " , addr + 4 * j);
		for (int k = 3 ; k >= 0 ; k--)
			//Little endian
			printf("%02lx " , vaddr_read(addr + 4 * j + k, 1));		
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
  printf("%-10s : 0x%-5lx\n" ,arg , val);
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
      puts("Invalid Arguments\n");
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

static int cmd_detach(char *args) {
  //difftest_skip_ref();
  //difftest_skip_dut(0, 0);
  return 0;
}

static int cmd_attach(char *args) {
  return 0;
}

/* We hope to imitate the snapshot function of standard virtual machines,
* so we have implemented the following functions
*/
static void init_snaplist(void) {
  char *__filename = (char *)malloc(sizeof(char) * _FILENAME_SIZE);
  strcpy(__filename, getenv("NEMU_HOME"));
  if(__filename == NULL) panic("NEMU_HOME does not exist. \nCheck whether your environment is configured correctly");
  strcat(__filename, "/snapshot_list"); 
  /* Initialize snaplist file pointer. */
  snaplist_file = fopen(__filename, "a+");
  free(__filename);
}

static int cmd_snaplist(char *args) {
  if (snaplist_file == NULL) {
    /* Try to init snaplist again. */
    init_snaplist();
    if(snaplist_file == NULL) panic("Snaplist file does not exist");
  }
  fseek(snaplist_file, 0, SEEK_SET);
  char ch;
  while ((ch = fgetc(snaplist_file)) != EOF)  printf("%c", ch); 
  fseek(snaplist_file, 0, SEEK_END);
  return 0;
}

/* The save and load operations create and load snapshots , respectively
* 'save' accepts an absolute path and creates (overwrites if a file with the same name exists) a file, 
*     writing to the current system snapshot
* 'load' reads the file name and loads the corresponding snapshot to the system
* Note that the above two operations will execute the default path when the parameters are empty
*/
static int cmd_save(char *args) {
  char *pathname = strtok(NULL, " ");
  /* Default pathname. */
  if(pathname == NULL) {
    pathname = (char *)malloc(sizeof(char) * _FILENAME_SIZE);
    /* It is important to note that the string returned by 'getenv' may be statically allocated,
    * which means it may be modified by subsequent calls to getenv, putenv, setenv, or unsetenv.
    * Therefore, the caller should not modify the returned string, 
    * nor should they rely on its persistence.
    */
    strcpy(pathname, getenv("NEMU_HOME"));
    if(pathname == NULL) panic("NEMU_HOME does not exist. \nCheck whether your environment is configured correctly");
    strcat(pathname, "/snapshot"); 
  }
  FILE *fp = fopen(pathname, "w+");
  if(fp == NULL) {
    printf("File %s opened(or created) failed\nCheck whether your nemu path is valid\n", pathname);
    return 0;
  }
  /* Append current snapfile to snaplist_file. */
  char *line = (char *)malloc(sizeof(char) * _FILENAME_SIZE);
  while(fgets(line, sizeof(line), snaplist_file) != NULL) {
    line[strcspn(line, "\n")] = '\0';
    if (strcmp(pathname, line) == 0)
      goto store;
  }
  fprintf(snaplist_file, "%s\n", pathname);
store:
  free(line);
  /* store nemu_state to file, including its reg file and mem. */
  printf("Creating system snapshot, please wait...\n");
  isa_snapshot_save(fp);
  pmem_snapshot_save(fp);
  printf("System snapshot created successfully.\nThe snapshot file is located at %s\n", pathname);
  fclose(fp);
  free(pathname);
  return 0;
}

static int cmd_load(char *args) {
  char *pathname = strtok(NULL, " ");
  if(pathname == NULL) {
    printf("Missing Arguments. \n");
    return 0;
  }
  FILE *fp = fopen(pathname, "r");
  if(fp == NULL) {  
    printf("File %s opened(or created) failed\nCheck whether your nemu path is valid\n", pathname);
    return 0;
  }
  /* load snapshot to nemu, including its reg file and mem. */
  printf("Loading system snapshot, please wait...\n");
  isa_snapshot_load(fp);
  pmem_snapshot_load(fp);
  printf("System snapshot %s loaded successfully.\n", pathname);
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help",     "Display information about all supported commands", cmd_help },
  { "c",        "Continue the execution of the program", cmd_c },
  { "q",        "Exit NEMU", cmd_q },
  { "si" ,      "Execute by single step" , cmd_si },
  { "info" ,    "Show information" , cmd_info },
  { "x" ,       "Scan Memory" , cmd_x },
  { "p" ,       "Evaluate the expression" , cmd_p },
  { "w" ,       "Set watchpoint" , cmd_w },
  { "d" ,       "Delete watchpoint" , cmd_d },
//{ "b" ,       "Set breakpoint" , cmd_b },
  { "detach" ,  "Disable Difftest Mode" , cmd_detach },
  { "attach" ,  "Enable Difftest Mode" , cmd_attach },
  { "save" ,    "Create a new snapshot" , cmd_save },
  { "load" ,    "Load a snapshot" , cmd_load},
  { "snaplist", "Display all existing or created system snapshots" , cmd_snaplist},
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
        printf("%-10s - %-20s\n", cmd_table[i].name, cmd_table[i].description);
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

  /* Initialize the snaplist file. */
  init_snaplist();
}
