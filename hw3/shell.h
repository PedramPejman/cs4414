#ifndef SHELL_H
#define SHELL_H

#include "fat.h"

#define COMMAND_LENGTH 128
#define SHELL_PROMPT ":/>"
enum commands_t {
  LS,
  CD,
  CPIN,
  CPOUT,
  EXIT,
  INVALID
};


void run_shell(Cursor *cursor);
void print_shell_prompt(Cursor *cursor);
#endif
