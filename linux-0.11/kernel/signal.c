/*
 *  linux/kernel/signal.c - RISC-V port
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <signal.h>

volatile void do_exit(int error_code);

int sys_sgetmask()
{
	return current->blocked;
}

int sys_ssetmask(int newmask)
{
	int old=current->blocked;

	current->blocked = newmask & ~(1<<(SIGKILL-1));
	return old;
}

static inline void save_old(char * from,char * to)
{
	int i;

	verify_area(to, sizeof(struct sigaction));
	for (i=0 ; i< sizeof(struct sigaction) ; i++) {
		put_fs_byte(*from,to);
		from++;
		to++;
	}
}

static inline void get_new(char * from,char * to)
{
	int i;

	for (i=0 ; i< sizeof(struct sigaction) ; i++)
		*(to++) = get_fs_byte(from++);
}

int sys_signal(int signum, long handler, long restorer)
{
	struct sigaction tmp;

	if (signum<1 || signum>32 || signum==SIGKILL)
		return -1;
	tmp.sa_handler = (void (*)(int)) handler;
	tmp.sa_mask = 0;
	tmp.sa_flags = SA_ONESHOT | SA_NOMASK;
	tmp.sa_restorer = (void (*)(void)) restorer;
	handler = (long) current->sigaction[signum-1].sa_handler;
	current->sigaction[signum-1] = tmp;
	return handler;
}

int sys_sigaction(int signum, const struct sigaction * action,
	struct sigaction * oldaction)
{
	struct sigaction tmp;

	if (signum<1 || signum>32 || signum==SIGKILL)
		return -1;
	tmp = current->sigaction[signum-1];
	get_new((char *) action,
		(char *) (signum-1+current->sigaction));
	if (oldaction)
		save_old((char *) &tmp,(char *) oldaction);
	if (current->sigaction[signum-1].sa_flags & SA_NOMASK)
		current->sigaction[signum-1].sa_mask = 0;
	else
		current->sigaction[signum-1].sa_mask |= (1<<(signum-1));
	return 0;
}

static void do_signal(struct trapframe * tf, long signr);

/* deliver the first pending, unblocked signal */
void do_signals(struct trapframe * tf)
{
	long sig, pending;

	if (!current->signal)
		return;
	pending = current->signal & ~current->blocked;
	if (!pending)
		return;
	sig = 1;
	while (!(pending & 1)) {
		pending >>= 1;
		sig++;
	}
	current->signal &= ~(1 << (sig - 1));
	do_signal(tf, sig);
}

static void do_signal(struct trapframe * tf, long signr)
{
	struct sigaction * sa = current->sigaction + signr - 1;
	unsigned long sa_handler = (unsigned long) sa->sa_handler;
	unsigned long old_sp, old_a0, old_pc, restorer;
	unsigned long * sp;

	if (sa_handler == 1)		/* SIG_IGN */
		return;
	if (!sa_handler) {
		if (signr == SIGCHLD)
			return;
		do_exit(signr);
	}
	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;
	restorer = (unsigned long) sa->sa_restorer;
	old_sp = tf->sp;
	old_a0 = tf->a0;
	old_pc = tf->mepc;
	sp = (unsigned long *)(old_sp - 20);
	verify_area(sp, 20);
	put_fs_long(restorer, sp + 0);
	put_fs_long(signr, sp + 1);
	put_fs_long(old_a0, sp + 2);
	put_fs_long(old_pc, sp + 3);
	put_fs_long(old_sp, sp + 4);
	tf->sp = (unsigned long) sp;
	tf->a0 = signr;
	tf->ra = restorer;
	tf->mepc = sa_handler;
	current->blocked |= sa->sa_mask;
}

int sys_sigreturn(void)
{
	struct trapframe * tf;
	unsigned long * sp;

	tf = (struct trapframe *)((char *) current + PAGE_SIZE - TF_SIZE);
	sp = (unsigned long *) tf->sp;
	tf->a0 = get_fs_long(sp + 2);
	tf->mepc = get_fs_long(sp + 3);
	tf->sp = get_fs_long(sp + 4);
	return tf->a0;
}
