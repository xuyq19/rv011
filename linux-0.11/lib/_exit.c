/*
 *  linux/lib/_exit.c - RISC-V port
 */

#define __LIBRARY__
#include <unistd.h>

volatile void _exit(int exit_code)
{
	register long __a7 __asm__("a7") = __NR_exit;
	register long __a0 __asm__("a0") = exit_code;

	__asm__ volatile ("ecall" :: "r" (__a7), "r" (__a0)
		: "memory", "t0", "t1");
	for (;;) ;
}
