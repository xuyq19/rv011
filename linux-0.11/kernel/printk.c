/*
 *  linux/kernel/printk.c - RISC-V port
 *
 *  Early printk goes straight to the 16550 UART.
 */

#include <stdarg.h>
#include <stddef.h>

#include <linux/kernel.h>

static char buf[1024];

extern int vsprintf(char * buf, const char * fmt, va_list args);
extern void uart_puts(char * s);

int printk(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);
	uart_puts(buf);
	return i;
}
