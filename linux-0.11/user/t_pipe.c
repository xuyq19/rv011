/*
 * t_pipe.c - pipe functional test: 4 KB parent->child, then child->parent.
 */

#define __LIBRARY__
#include <unistd.h>
#include <string.h>

#include "uprint.h"

_syscall0(int, fork)
_syscall1(int, pipe, int *, fds)
_syscall3(int, read, int, fd, char *, buf, int, count)
_syscall3(int, write, int, fd, const char *, buf, int, count)
_syscall1(int, close, int, fd)
_syscall3(int, waitpid, int, pid, int *, stat, int, options)

int main(void)
{
	int fds[2], pid, st, i, err = 0;
	unsigned char out[4096], in[4096], ack[8];

	for (i = 0; i < 4096; i++)
		out[i] = (unsigned char) (i & 0xff);
	if (pipe(fds) < 0) {
		up_printf("pipe: pipe failed\n");
		return 1;
	}
	pid = fork();
	if (pid < 0) {
		up_printf("pipe: fork failed\n");
		return 1;
	}
	if (!pid) {
		i = 0;
		while (i < 4096) {
			int n = read(fds[0], in + i, 4096 - i);
			if (n <= 0)
				break;
			i += n;
		}
		if (i != 4096 || memcmp(in, out, 4096)) {
			up_printf("pipe: child got wrong data (%d)\n", i);
			_exit(1);
		}
		if (write(1, "pipe: child ok\n", 15) != 15)
			_exit(2);
		if (write(fds[1], "ack!", 4) != 4)
			_exit(3);
		close(fds[0]);
		close(fds[1]);
		_exit(0);
	}
	if (write(fds[1], out, 4096) != 4096) {
		up_printf("pipe: parent write failed\n");
		err = 1;
	}
	waitpid(pid, &st, 0);
	if (st != 0)
		err = 1;
	i = 0;
	while (i < 4) {
		int n = read(fds[0], ack + i, 4 - i);
		if (n <= 0)
			break;
		i += n;
	}
	if (i != 4 || memcmp(ack, "ack!", 4)) {
		up_printf("pipe: parent got bad ack (%d)\n", i);
		err = 1;
	}
	close(fds[0]);
	up_printf("pipe test %s\n", err ? "FAILED" : "passed");
	return err ? 1 : 0;
}
