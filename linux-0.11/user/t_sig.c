/*
 * t_sig.c - signal functional test: SIGALRM handler + restorer,
 * kill/SIGKILL status, blocking with sgetmask/ssetmask.
 */

#define __LIBRARY__
#include <unistd.h>
#include <signal.h>

#include "uprint.h"

_syscall0(int, getpid)
_syscall0(int, fork)
_syscall2(int, kill, int, pid, int, sig)
_syscall3(int, waitpid, int, pid, int *, stat, int, options)
_syscall1(int, alarm, int, sec)
_syscall0(int, pause)
_syscall0(int, sgetmask)
_syscall1(int, ssetmask, int, newmask)
_syscall3(int, sigaction, int, sig, struct sigaction *, act,
	struct sigaction *, old)
_syscall1(time_t, times, struct tms *, tp)

extern void sig_restorer(void);

static volatile int alarm_count;

static void on_alarm(int sig)
{
	alarm_count++;
}

static int setup_handler(int sig, void (* fn)(int))
{
	struct sigaction sa;

	sa.sa_handler = fn;
	sa.sa_mask = 0;
	sa.sa_flags = SA_NOMASK;
	sa.sa_restorer = sig_restorer;
	return sigaction(sig, &sa, 0);
}

static void busy_wait_ticks(int ticks)
{
	struct tms t;
	long start = times(&t);

	while (times(&t) - start < ticks)
		;
}

int main(void)
{
	int pid, st, err = 0;
	int oldmask;
	struct tms t;

	up_printf("sig test pid=%d\n", getpid());
	if (setup_handler(SIGALRM, on_alarm) < 0) {
		up_printf("sig: sigaction failed\n");
		return 1;
	}
	alarm(1);
	while (!alarm_count)
		pause();
	up_printf("sig: alarm fired, count=%d\n", alarm_count);

	/* kill(SIGKILL) -> wait status 9 */
	pid = fork();
	if (pid < 0)
		return 1;
	if (!pid) {
		busy_wait_ticks(300);
		_exit(42);
	}
	kill(pid, SIGKILL);
	waitpid(pid, &st, 0);
	if (st != 9) {
		up_printf("sig: SIGKILL status wrong: %x\n", st);
		err = 1;
	}

	/* blocked SIGALRM is not delivered until unmasked */
	alarm_count = 0;
	oldmask = ssetmask(1 << (SIGALRM - 1));
	alarm(1);
	busy_wait_ticks(250);
	if (alarm_count) {
		up_printf("sig: alarm fired while blocked!\n");
		err = 1;
	}
	ssetmask(oldmask);
	busy_wait_ticks(50);
	if (!alarm_count) {
		up_printf("sig: blocked alarm never delivered\n");
		err = 1;
	}
	up_printf("sig: mask test ok, count=%d\n", alarm_count);

	(void) times(&t);
	up_printf("sig test %s\n", err ? "FAILED" : "passed");
	return err ? 1 : 0;
}
