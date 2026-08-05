/*
 * t_stress.c - combined stress test:
 *   1) repeated fork+exec of /bin/t_hello
 *   2) concurrent children doing filesystem writes + reads
 *   3) heap pressure via brk until failure, then basic ops still work
 *   4) pipe ping-pong
 *   5) COW hammer with 8 children
 */

#define __LIBRARY__
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "uprint.h"

_syscall0(int, fork)
_syscall0(int, getpid)
_syscall3(int, execve, const char *, file, char **, argv, char **, envp)
_syscall3(int, waitpid, int, pid, int *, stat, int, options)
_syscall2(int, creat, const char *, file, mode_t, mode)
_syscall1(int, close, int, fd)
_syscall3(int, read, int, fd, char *, buf, int, count)
_syscall3(int, write, int, fd, const char *, buf, int, count)
_syscall1(int, unlink, const char *, file)
_syscall1(int, pipe, int *, fds)
_syscall1(int, brk, unsigned long, end)
_syscall0(int, sync)

static unsigned char pat[2048];

static unsigned long brk_old(void)
{
	return (unsigned long) brk(0);
}

static int fs_child(int which)
{
	char name[16];
	int fd, i, n;
	unsigned char buf[2048];

	up_printf("stress: fs child %d pid=%d\n", which, getpid());
	for (i = 0; i < 3; i++) {
		name[0] = '/';
		name[1] = 's';
		name[2] = (char) ('0' + which);
		name[3] = (char) ('0' + i);
		name[4] = '.';
		name[5] = 'd';
		name[6] = 'a';
		name[7] = 't';
		name[8] = 0;
		fd = creat(name, 0644);
		if (fd < 0)
			return 1;
		if (write(fd, pat, sizeof(pat)) != sizeof(pat)) {
			close(fd);
			return 1;
		}
		close(fd);
		fd = open(name, O_RDONLY, 0);
		if (fd < 0)
			return 1;
		n = 0;
		while (n < 2048) {
			int r = read(fd, buf + n, 2048 - n);
			if (r <= 0)
				break;
			n += r;
		}
		close(fd);
		if (n != 2048 || memcmp(buf, pat, 2048))
			return 1;
		unlink(name);
	}
	return 0;
}

int main(void)
{
	int i, j, pid, st, fds[2], fds2[2], err = 0;
	int children[8];
	unsigned char ping[512], pong[512], got[512];
	unsigned long start_brk, cur, grow;
	static unsigned char cow[16384];

	for (i = 0; i < 2048; i++)
		pat[i] = (unsigned char) (i & 0xff);

	up_printf("stress: phase 1 fork+exec x20\n");
	for (i = 0; i < 20; i++) {
		pid = fork();
		if (pid < 0) {
			up_printf("stress: fork failed\n");
			return 1;
		}
		if (!pid) {
			char * argv[2];

			argv[0] = "/bin/t_hello";
			argv[1] = 0;
			execve(argv[0], argv, 0);
			_exit(99);
		}
		waitpid(pid, &st, 0);
		if (st != 0) {
			up_printf("stress: exec child status %x\n", st);
			err = 1;
		}
	}
	up_printf("stress: phase 1 done\n");

	up_printf("stress: phase 2 concurrent fs children x8\n");
	for (i = 0; i < 8; i++) {
		pid = fork();
		if (pid < 0) {
			up_printf("stress: fs fork failed\n");
			return 1;
		}
		if (!pid)
			_exit(fs_child(i) ? 1 : 0);
		children[i] = pid;
	}
	for (i = 0; i < 8; i++) {
		waitpid(-1, &st, 0);
		if (st != 0)
			err = 1;
	}
	up_printf("stress: phase 2 done\n");

	up_printf("stress: phase 3 heap pressure\n");
	start_brk = brk_old();
	cur = start_brk;
	grow = 0;
	for (;;) {
		unsigned long old = (unsigned long) brk(cur + 4096);

		if (old != cur + 4096)
			break;
		cur += 4096;
		grow++;
		if (grow > 30000)
			break;
	}
	up_printf("stress: grew heap %ld pages, %ld KB\n", grow,
		grow * 4);
	/* bring it back down and confirm basic syscalls still work */
	brk(start_brk);
	pid = fork();
	if (pid < 0) {
		up_printf("stress: post-pressure fork failed\n");
		err = 1;
	} else if (!pid) {
		up_printf("stress: post-pressure child alive\n");
		_exit(0);
	} else
		waitpid(pid, &st, 0);
	up_printf("stress: phase 3 done\n");

	up_printf("stress: phase 4 pipe ping-pong x100\n");
	if (pipe(fds) < 0 || pipe(fds2) < 0)
		return 1;
	pid = fork();
	if (pid < 0)
		return 1;
	if (!pid) {
		close(fds[1]);
		close(fds2[0]);
		for (j = 0; j < 100; j++) {
			for (i = 0; i < 512; i++)
				ping[i] = (unsigned char) (j + i);
			if (read(fds[0], got, 512) != 512)
				_exit(1);
			if (memcmp(got, ping, 512))
				_exit(2);
			for (i = 0; i < 512; i++)
				pong[i] = (unsigned char) ((j + i) ^ 0xa5);
			if (write(fds2[1], pong, 512) != 512)
				_exit(3);
		}
		_exit(0);
	}
	close(fds[0]);
	close(fds2[1]);
	for (j = 0; j < 100; j++) {
		for (i = 0; i < 512; i++)
			ping[i] = (unsigned char) (j + i);
		if (write(fds[1], ping, 512) != 512)
			err = 1;
		for (i = 0; i < 512; i++)
			pong[i] = (unsigned char) ((j + i) ^ 0xa5);
		if (read(fds2[0], got, 512) != 512 ||
		    memcmp(got, pong, 512))
			err = 1;
	}
	waitpid(pid, &st, 0);
	up_printf("stress: phase 4 done\n");

	up_printf("stress: phase 5 COW hammer x8\n");
	for (i = 0; i < 16384; i++)
		cow[i] = (unsigned char) i;
	for (i = 0; i < 8; i++) {
		pid = fork();
		if (pid < 0)
			return 1;
		if (!pid) {
			for (j = 0; j < 16384; j++)
				cow[j] = (unsigned char) (j + i + 1);
			_exit(0);
		}
	}
	for (i = 0; i < 8; i++)
		waitpid(-1, &st, 0);
	for (i = 0; i < 16384; i++)
		if (cow[i] != (unsigned char) i) {
			err = 1;
			break;
		}
	sync();
	up_printf("stress test %s\n", err ? "FAILED" : "passed");
	return err ? 1 : 0;
}
