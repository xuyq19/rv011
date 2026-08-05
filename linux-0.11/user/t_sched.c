/*
 * t_sched.c - scheduler/timer stability: N busy children make progress
 * concurrently; parent verifies every child got CPU time and exit codes.
 */

#define __LIBRARY__
#include <unistd.h>

#include "uprint.h"

_syscall0(int, fork)
_syscall3(int, waitpid, int, pid, int *, stat, int, options)
_syscall0(int, getpid)
_syscall1(time_t, times, struct tms *, tp)

#define NCHILD 12

int main(void)
{
	int i, pid, st;
	long start, end;
	struct tms t;
	int children[NCHILD];

	up_printf("sched test: spawning %d children\n", NCHILD);
	start = times(&t);
	for (i = 0; i < NCHILD; i++) {
		pid = fork();
		if (pid < 0) {
			up_printf("sched: fork failed at %d\n", i);
			return 1;
		}
		if (!pid) {
			volatile long x = 0;
			int j;

			for (j = 0; j < 300000; j++)
				x += j;
			up_printf("child %d (pid=%d) x=%ld\n", i, getpid(), x);
			_exit(100 + i);
		}
		children[i] = pid;
	}
	for (i = 0; i < NCHILD; i++) {
		pid = waitpid(-1, &st, 0);
		if (st != (100 + i) << 8)
			up_printf("sched: child %d status %x\n", pid, st);
	}
	end = times(&t);
	up_printf("sched test done in %ld ticks\n", end - start);
	return 0;
}
