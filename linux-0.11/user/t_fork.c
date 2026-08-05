/*
 * t_fork.c - fork/exec/wait/exit-code functional test.
 */

#define __LIBRARY__
#include <unistd.h>
#include <string.h>

#include "uprint.h"

_syscall0(int, fork)
_syscall0(int, getpid)
_syscall0(int, getppid)
_syscall3(int, execve, const char *, file, char **, argv, char **, envp)
_syscall3(int, waitpid, int, pid, int *, stat, int, options)
_syscall2(int, kill, int, pid, int, sig)

int main(void)
{
	int pid, st, i;

	up_printf("fork test pid=%d ppid=%d\n", getpid(), getppid());
	for (i = 0; i < 4; i++) {
		pid = fork();
		if (pid < 0) {
			up_printf("fork failed: %d\n", pid);
			return 1;
		}
		if (!pid) {
			up_printf("child %d pid=%d ppid=%d\n", i, getpid(),
				getppid());
			if (i == 3) {
				char * argv[2];

				argv[0] = "/bin/t_hello";
				argv[1] = 0;
				execve(argv[0], argv, 0);
				up_printf("exec failed\n");
			}
			_exit(10 + i);
		}
	}
	for (i = 0; i < 4; i++) {
		pid = waitpid(-1, &st, 0);
		up_printf("reaped pid=%d status=%x\n", pid, st);
	}
	up_printf("fork test done\n");
	return 0;
}
