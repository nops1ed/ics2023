#ifndef __FTRACE_H__
#define __FTRACE_H__

#include <common.h>

void INIT_SYMBOL_TABLE(const char *elf_filename);
void ftrace_call();
void ftrace_ret();

#endif