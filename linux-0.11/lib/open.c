/*
 *  linux/lib/open.c - RISC-V port
 */

#define __LIBRARY__
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

int open(const char * filename, int flag, ...)
{
	register int res;
	va_list arg;

	va_start(arg,flag);
	{
		register long __a7 __asm__("a7") = __NR_open;
		register long __a0 __asm__("a0") = (long) filename;
		register long __a1 __asm__("a1") = flag;
		register long __a2 __asm__("a2") = va_arg(arg,int);

		__asm__ volatile ("ecall"
			: "+r" (__a7), "+r" (__a0), "+r" (__a1), "+r" (__a2)
			:: "memory");
		res = (int) __a0;
	}
	va_end(arg);
	if (res>=0)
		return res;
	errno = -res;
	return -1;
}
