/*
 *  linux/lib/read.c - RISC-V port
 */

#define __LIBRARY__
#include <unistd.h>

_syscall3(int,read,int,fd,char *,buf,off_t,count)
