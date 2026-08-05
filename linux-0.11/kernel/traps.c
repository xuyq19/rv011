/*
 *  linux/kernel/traps.c - RISC-V exception and trap handling
 *
 *  All traps arrive in M-mode at trap_entry (kernel/trap.S), which
 *  builds a trapframe and calls trap_handler().
 */

#include <linux/head.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/mm.h>
#include <asm/system.h>
#include <asm/io.h>

volatile void do_exit(long code);

extern void trap_entry(void);
extern void do_timer(long cpl);
extern void do_signals(struct trapframe * tf);

#define NR_SYSCALLS 73
#define TIMER_INTERVAL (CLINT_TIMEBASE / HZ)

static inline unsigned long read_mtval(void)
{
	unsigned long v;
	__asm__ volatile ("csrr %0, mtval" : "=r" (v));
	return v;
}

static void die(char * str, struct trapframe * tf, unsigned long cause)
{
	printk("%s: cause=%lx mepc=%lx mtval=%lx\n\r",
		str, cause, tf->mepc, read_mtval());
	printk("pid: %d state: %ld\n\r", current->pid, current->state);
	do_exit(11);
}

static void exception_trap(struct trapframe * tf, unsigned long cause)
{
	if (!tf->from_user) {
		printk("kernel trap: cause=%lx mepc=%lx mtval=%lx ra=%lx sp=%lx\n\r",
			cause, tf->mepc, read_mtval(), tf->ra, tf->sp);
		panic("kernel trap");
	}
	switch (cause) {
	case 0:
		die("instruction address misaligned", tf, cause);
		break;
	case 1:
		die("instruction access fault", tf, cause);
		break;
	case 2:
		die("illegal instruction", tf, cause);
		break;
	case 3:
		die("breakpoint", tf, cause);
		break;
	case 4:
		die("load address misaligned", tf, cause);
		break;
	case 5:
		die("load access fault", tf, cause);
		break;
	case 6:
		die("store address misaligned", tf, cause);
		break;
	case 7:
		die("store access fault", tf, cause);
		break;
	default:
		die("trap", tf, cause);
		break;
	}
}

void trap_handler(struct trapframe * tf)
{
	unsigned long cause = tf->mcause;
	int from_user = tf->from_user;

	/* machine timer interrupt: mcause bit 31 set, code 7 */
	if ((cause & 0x80000000) && (cause & 0xff) == 7) {
		jiffies++;
		CLINT_MTIMECMP = CLINT_MTIME + TIMER_INTERVAL;
		do_timer(from_user);
		if (from_user)
			do_signals(tf);
		return;
	}
	switch (cause) {
	case 8:			/* ecall from U-mode */
	case 11:		/* ecall from M-mode (kernel lib) */
		{
			unsigned long nr = tf->a7;

			tf->mepc += 4;
			if (nr < NR_SYSCALLS) {
				tf->a0 = sys_call_table[nr](tf->a0,
					tf->a1, tf->a2);
			} else
				tf->a0 = -1;
			if (from_user)
				do_signals(tf);
		}
		break;
	case 12:		/* load page fault */
	case 15:		/* store page fault */
		page_fault_handler(tf, read_mtval());
		break;
	default:
		exception_trap(tf, cause);
		break;
	}
}

void trap_init(void)
{
	__asm__ volatile ("csrw mtvec, %0" :: "r" (trap_entry));
	/* no delegation: all traps stay in M-mode */
}
