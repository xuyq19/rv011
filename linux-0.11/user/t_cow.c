/*
 * t_cow.c - copy-on-write and brk functional test.
 */

#define __LIBRARY__
#include <unistd.h>
#include <string.h>

#include "uprint.h"

_syscall0(int, fork)
_syscall3(int, waitpid, int, pid, int *, stat, int, options)
_syscall1(int, brk, unsigned long, end)
_syscall0(int, getpid)

static unsigned char buf[8192];

static unsigned long sbrk0(unsigned long inc)
{
	unsigned long old = (unsigned long) brk(0);
	unsigned long new = old + inc;
	unsigned long ret = (unsigned long) brk(new);

	up_printf("brk: old=%lx new=%lx ret=%lx\n", old, new, ret);
	if (ret != new)
		return 0;
	return old;
}

int main(void)
{
	int i, pid, st, ok = 1;
	unsigned long * big;

	for (i = 0; i < 8192; i++)
		buf[i] = (unsigned char) (i & 0xff);

	pid = fork();
	if (pid < 0) {
		up_printf("fork failed\n");
		return 1;
	}
	if (!pid) {
		for (i = 0; i < 8192; i++)
			buf[i] = (unsigned char) ((i + 1) & 0xff);
		up_printf("child: wrote over buf, pid=%d\n", getpid());
		_exit(0);
	}
	waitpid(pid, &st, 0);
	for (i = 0; i < 8192; i++)
		if (buf[i] != (unsigned char) (i & 0xff)) {
			ok = 0;
			break;
		}
	up_printf("cow: parent data intact = %s\n", ok ? "yes" : "NO");
	if (!ok)
		return 1;

	/* grow the heap with brk, touch pages, fork, and modify in child */
	big = (unsigned long *) sbrk0(64 * 1024);
	if (!big) {
		up_printf("brk grow failed\n");
		return 1;
	}
	for (i = 0; i < 64 * 1024 / 4; i++)
		big[i] = (unsigned long) i;
	pid = fork();
	if (pid < 0) {
		up_printf("fork2 failed\n");
		return 1;
	}
	if (!pid) {
		for (i = 0; i < 64 * 1024 / 4; i++)
			big[i] = ~(unsigned long) i;
		up_printf("child2: modified heap\n");
		_exit(0);
	}
	waitpid(pid, &st, 0);
	ok = 1;
	for (i = 0; i < 64 * 1024 / 4; i++)
		if (big[i] != (unsigned long) i) {
			ok = 0;
			break;
		}
	up_printf("cow: heap intact = %s\n", ok ? "yes" : "NO");
	return ok ? 0 : 1;
}
