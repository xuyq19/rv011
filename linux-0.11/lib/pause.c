/*
 *  linux/lib/pause.c - RISC-V port
 */

#define __LIBRARY__
#include <unistd.h>

_syscall0(int,pause)
