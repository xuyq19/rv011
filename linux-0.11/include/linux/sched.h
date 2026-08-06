#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64
#define HZ 100

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <signal.h>

#if (NR_OPEN > 32)
#error "Currently the close-on-exec-flags are in one word, max 32 files/proc"
#endif

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE		3
#define TASK_STOPPED		4

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern int copy_page_tables(unsigned long from_pgdir, unsigned long to_pgdir);
extern int free_page_tables(unsigned long pgdir);

extern void sched_init(void);
extern void schedule(void);
extern void trap_init(void);
extern void panic(const char * str);
extern int tty_write(unsigned minor,char * buf,int count);

typedef int (*fn_ptr)();

/*
 * RISC-V port: software task context (callee-saved registers).
 * Offsets are hardcoded in kernel/trap.S - keep in sync.
 */
struct kernel_context {
	long ra;		/* 0 */
	long sp;		/* 4 */
	long s0;		/* 8 */
	long s1;		/* 12 */
	long s2;		/* 16 */
	long s3;		/* 20 */
	long s4;		/* 24 */
	long s5;		/* 28 */
	long s6;		/* 32 */
	long s7;		/* 36 */
	long s8;		/* 40 */
	long s9;		/* 44 */
	long s10;		/* 48 */
	long s11;		/* 52 */
};

/* trap frame layout - keep in sync with kernel/trap.S */
#define TF_MSTATUS 0
#define TF_MEPC 4
#define TF_X1 8
#define TF_X2 12
#define TF_X10 44
#define TF_X17 72
#define TF_MCAUSE 132
#define TF_FROMUSER 136
#define TF_F0 140
#define TF_FCSR 396
#define TF_SIZE 400

struct trapframe {
	unsigned long mstatus;	/* 0 */
	unsigned long mepc;	/* 4 */
	unsigned long ra;	/* 8  x1 */
	unsigned long sp;	/* 12 x2 */
	unsigned long gp;	/* 16 x3 */
	unsigned long tp;	/* 20 x4 */
	unsigned long t0;	/* 24 x5 */
	unsigned long t1;	/* 28 x6 */
	unsigned long t2;	/* 32 x7 */
	unsigned long s0;	/* 36 x8 */
	unsigned long s1;	/* 40 x9 */
	unsigned long a0;	/* 44 x10 */
	unsigned long a1;	/* 48 x11 */
	unsigned long a2;	/* 52 x12 */
	unsigned long a3;	/* 56 x13 */
	unsigned long a4;	/* 60 x14 */
	unsigned long a5;	/* 64 x15 */
	unsigned long a6;	/* 68 x16 */
	unsigned long a7;	/* 72 x17 */
	unsigned long s2;	/* 76 x18 */
	unsigned long s3;	/* 80 x19 */
	unsigned long s4;	/* 84 x20 */
	unsigned long s5;	/* 88 x21 */
	unsigned long s6;	/* 92 x22 */
	unsigned long s7;	/* 96 x23 */
	unsigned long s8;	/* 100 x24 */
	unsigned long s9;	/* 104 x25 */
	unsigned long s10;	/* 108 x26 */
	unsigned long s11;	/* 112 x27 */
	unsigned long t3;	/* 116 x28 */
	unsigned long t4;	/* 120 x29 */
	unsigned long t5;	/* 124 x30 */
	unsigned long t6;	/* 128 x31 */
	unsigned long mcause;	/* 132 */
	unsigned long from_user;/* 136 */
	unsigned long long f[32]; /* 140 .. 396: FP registers f0-f31 (64-bit) */
	unsigned long fcsr;	/* 396: FP control/status */
};

struct task_struct {
/* these are hardcoded - don't touch */
	struct kernel_context kctx;	/* offset 0, used by switch_to */
	unsigned long pgdir;		/* offset 56: user page directory */
	long state;			/* -1 unrunnable, 0 runnable, >0 stopped */
	long counter;
	long priority;
	long signal;
	struct sigaction sigaction[32];
	long blocked;	/* bitmap of masked signals */
/* various fields */
	int exit_code;
	unsigned long start_code,end_code,end_data,brk,start_stack;
	long pid,father,pgrp,session,leader;
	unsigned short uid,euid,suid;
	unsigned short gid,egid,sgid;
	long alarm;
	long utime,stime,cutime,cstime,start_time;
	unsigned short used_math;
/* file system info */
	int tty;		/* -1 if no tty, so it must be signed */
	unsigned short umask;
	struct m_inode * pwd;
	struct m_inode * root;
	struct m_inode * executable;
	unsigned long close_on_exec;
	struct file * filp[NR_OPEN];
};

/*
 * INIT_TASK is used to set up the first task table. Task 0 is the idle
 * task; it never runs user code, so pgdir is 0 and kctx is zeroed.
 */
#define INIT_TASK \
/* kctx, pgdir */	{ {0,}, 0, \
/* state etc */	0,15,15, \
/* signals */	0,{{},},0, \
/* ec,brk... */	0,0,0,0,0,0, \
/* pid etc.. */	0,-1,0,0,0, \
/* uid etc */	0,0,0,0,0,0, \
/* alarm */	0,0,0,0,0,0, \
/* math */	0, \
/* fs info */	-1,0022,NULL,NULL,NULL,0, \
/* filp */	{NULL,}, \
}

extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern long volatile jiffies;
extern long startup_time;

#define CURRENT_TIME (startup_time+jiffies/HZ)

extern void add_timer(long jiffies, void (*fn)(void));
extern void sleep_on(struct task_struct ** p);
extern void interruptible_sleep_on(struct task_struct ** p);
extern void wake_up(struct task_struct ** p);
extern void do_timer(long cpl);
extern void do_signals(struct trapframe * tf);
extern int sys_fork(void);
extern int kernel_thread(void (*fn)(void));
extern int sys_pause(void);
extern void ret_from_trap(void);
extern void switch_to(struct task_struct * next);

#define PAGE_ALIGN(n) (((n)+0xfff)&0xfffff000)

#endif
