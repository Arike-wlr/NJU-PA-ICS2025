/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
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
#include <memory/vaddr.h>
#include <watchpoint.h>
#include <expr.h>

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
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} 
//The structure to hold command information

cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si", "Single step execution [N] instructions (default:N=1)", cmd_si},
  {"info","Display the current state of registers or watchpoints", cmd_info},
  {"x","Examine memory [N] words at address [EXPR]", cmd_x},
  {"p", "Evaluate the expression [EXPR] and print the result", cmd_p},
  {"w", "Pause the program when the value of the expression [EXPR] changes.", cmd_w},
  {"d", "Delete the watchpoint with number [N]", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  /* Single step execution [N] instructions (default:N=1). */
  if(args== NULL) {
    cpu_exec(1); // Default value for N (no args)
  } 
  else {
    char* endptr ;
    int n=strtol(args, &endptr, 10);// Convert the argument to an integer

    if (*endptr != '\0') { // Check if the conversion was complete
      // If not, the argument was not a valid number
      printf("Invalid character '%c' in argument(not a number): %s\n",*endptr, args);
      return 0;
    }// If the argument is a number, we check if it is valid
    if (n <= 0) { // The number must be greater than 0
      printf("Invalid number of instructions(lower than 1): %s\n", args);
      return 0;
    }
    cpu_exec(n);
  }
  return 0;
}

static int cmd_info(char *args) {
  /* Display the current state of registers or watchpoints. */
  if (strcmp(args, "r") == 0) {
    isa_reg_display(); // Display registers
  } 
  else if (strcmp(args, "w") == 0) {
    display_wp(); // Display watchpoints
  } 
  else {
    printf("Unknown argument '%s' for info command\n", args);
    printf("Usage: info [r|w]\n");
    printf(" r: Display registers\n");
    printf(" w: Display watchpoints\n");
  }

  return 0;
}

bool success = true; // Global variable to indicate success of expression evaluation

static int cmd_x(char *args) {
  /* Examine memory [N] words at address [EXPR]. */
  if (args == NULL) {
    printf("Arguments missing for x command\n");
    printf("Usage: x N EXPR\n");
    return 0;
  }
  //Convert the arg N（要显示的字数）
  char *endptr;
  long n = strtol(args, &endptr, 10);
  if (n <= 0 || endptr == args) {
    printf("Invalid number of words: %s\n", args);
    return 0;
  }
  char *exp = endptr + 1; // Move past the space to the expression
  if (*exp == '\0') {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  // Evaluate the expression to get the starting address
  word_t start_addr = expr(exp, &success);
  if (!success) {
    printf("Failed to evaluate expression: %s\n", exp);
    return 0;
  }
  // Print the memory contents
  printf("Memory at address 0x%x:\n", start_addr);
  for (long i = 0; i < n; i++) {
    // Read the memory at the address
    vaddr_t curr_addr = start_addr + i * 4; 
    word_t value = vaddr_read(curr_addr, 4);
    printf("0x%x: 0x%x\n", curr_addr, value);
  }
  return 0;
}

static int cmd_p(char *args) {
  /* Evaluate the expression [EXPR] and print the result. */
  if (args == NULL) {
    printf("Expression missing for p command\n");
    return 0;
  }

  // Evaluate the expression
  word_t result = expr(args,&success);
  if(!success) {
    printf("Failed to evaluate expression: %s\n", args);
    return 0;
  }
  printf("%s = %u\n", args, result);
  return 0;
}

static int cmd_w(char *args) {
  /*Pause the program when the value of the expression [EXPR] changes.*/
  if (args == NULL) {
    printf("Expression missing for w command\n");
    return 0;
  }

  expr(args, &success);// Evaluate the expression to check if it is valid
  if (!success) {
    printf("Failed to evaluate expression: %s\n", args);
    return 0;
  }
  
  create_watchpoint(&success, args );//Create the wp:
  if (!success) {
    printf("Failed to create watchpoint for expression: %s\n", args);
    return 0;
  }

  return 0;
}

static int cmd_d(char *args) {
  /* Delete the watchpoint with number [N]. */
  if (args == NULL) {
    printf("Watchpoint number missing for d command\n");
    return 0;
  }

  // Convert the argument to an integer
  char *endptr;
  int wp_num = strtol(args, &endptr, 10);
  if (*endptr != '\0' || wp_num < 0) {
    printf("Invalid watchpoint number: %s\n", args);
    return 0;
  }
  // Delete the watchpoint
  delete_watchpoint(wp_num, &success);
  if (!success) {
    printf("Failed to delete watchpoint number: %d\n", wp_num);
    return 0;
  }
  printf("Watchpoint number %d deleted\n", wp_num);
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
