/*
 * user/uprint.c - tiny user-space printf for the test programs.
 * Supports %s %c %d %u %x %% via write(1,...).
 */

#define __LIBRARY__
#include <unistd.h>
#include <stdarg.h>

#include "uprint.h"

/* private raw write so user tests can define their own write() */
static int up_write_raw(int fd, const char * buf, int count)
{
	register long __a7 __asm__("a7") = __NR_write;
	register long __a0 __asm__("a0") = fd;
	register long __a1 __asm__("a1") = (long) buf;
	register long __a2 __asm__("a2") = count;

	__asm__ volatile ("ecall"
		: "+r" (__a7), "+r" (__a0), "+r" (__a1), "+r" (__a2)
		:: "memory", "t0", "t1");
	return (int) __a0;
}

static void up_putc(int fd, char c);
static void up_puts(int fd, const char * s);
static void up_num(int fd, unsigned long v, int base, int upper);

int up_printf2(const char * fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			const char * e = fmt;
			while (*e && *e != '%')
				e++;
			up_write_raw(2, fmt, e - fmt);
			fmt = e - 1;
			continue;
		}
		switch (*++fmt) {
		case 's':
			{
				const char * s = va_arg(ap, const char *);
				while (*s)
					up_write_raw(2, s++, 1);
			}
			break;
		case 'd': {
			int v = va_arg(ap, int);
			if (v < 0) {
				up_write_raw(2, "-", 1);
				v = -v;
			}
			up_num(2, v, 10, 0);
			break;
		}
		case 'x':
			up_num(2, va_arg(ap, unsigned int), 16, 0);
			break;
		default:
			up_write_raw(2, fmt - 1, 2);
			break;
		}
	}
	va_end(ap);
	return 0;
}

/* variadic open()/fcntl() wrappers (their libc signatures are varargs) */
int open(const char * filename, int flag, ...)
{
	va_list arg;
	register long __a7 __asm__("a7") = __NR_open;
	register long __a0 __asm__("a0") = (long) filename;
	register long __a1 __asm__("a1") = flag;
	register long __a2 __asm__("a2");

	va_start(arg, flag);
	__a2 = va_arg(arg, int);
	va_end(arg);
	__asm__ volatile ("ecall"
		: "+r" (__a7), "+r" (__a0), "+r" (__a1), "+r" (__a2)
		:: "memory", "t0", "t1");
	if (__a0 >= 0)
		return (int) __a0;
	errno = -(int) __a0;
	return -1;
}

int fcntl(int fildes, int cmd, ...)
{
	va_list arg;
	register long __a7 __asm__("a7") = __NR_fcntl;
	register long __a0 __asm__("a0") = fildes;
	register long __a1 __asm__("a1") = cmd;
	register long __a2 __asm__("a2");

	va_start(arg, cmd);
	__a2 = va_arg(arg, int);
	va_end(arg);
	__asm__ volatile ("ecall"
		: "+r" (__a7), "+r" (__a0), "+r" (__a1), "+r" (__a2)
		:: "memory", "t0", "t1");
	if (__a0 >= 0)
		return (int) __a0;
	errno = -(int) __a0;
	return -1;
}

static void up_putc(int fd, char c)
{
	up_write_raw(fd, &c, 1);
}

static void up_puts(int fd, const char * s)
{
	while (*s)
		up_putc(fd, *s++);
}

static void up_num(int fd, unsigned long v, int base, int upper)
{
	char buf[12];
	char * p = buf + sizeof(buf);
	const char * digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

	*--p = 0;
	do {
		*--p = digits[v % base];
		v /= base;
	} while (v);
	up_puts(fd, p);
}

int up_printf(const char * fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			up_putc(1, *fmt);
			continue;
		}
		switch (*++fmt) {
		case 's':
			up_puts(1, va_arg(ap, const char *));
			break;
		case 'c':
			up_putc(1, (char) va_arg(ap, int));
			break;
		case 'd': {
			int v = va_arg(ap, int);
			if (v < 0) {
			up_putc(1, '-');
			v = -v;
		}
		up_num(1, (unsigned long) v, 10, 0);
		break;
		}
		case 'u':
			up_num(1, va_arg(ap, unsigned int), 10, 0);
			break;
		case 'x':
			up_num(1, va_arg(ap, unsigned int), 16, 0);
			break;
		case '%':
			up_putc(1, '%');
			break;
		case 'l':
			if (fmt[1] == 'x') {
				fmt++;
				up_num(1, va_arg(ap, unsigned long), 16, 0);
			} else if (fmt[1] == 'd') {
				long v;
				fmt++;
				v = va_arg(ap, long);
				if (v < 0) {
					up_putc(1, '-');
					v = -v;
				}
				up_num(1, (unsigned long) v, 10, 0);
			} else {
				up_putc(1, '%');
				up_putc(1, 'l');
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			/* skip width digits; only %02x/%04x matter for tests */
			while (fmt[1] >= '0' && fmt[1] <= '9')
				fmt++;
			if (fmt[1] == 'x') {
				fmt++;
				up_num(1, va_arg(ap, unsigned int), 16, 0);
			} else {
				up_putc(1, '%');
				up_putc(1, *fmt);
			}
			break;
		}
		default:
			up_putc(1, '%');
			if (*fmt)
				up_putc(1, *fmt);
			break;
		}
	}
	va_end(ap);
	return 0;
}
