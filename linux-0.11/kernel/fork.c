/*
 *  linux/kernel/fork.c - RISC-V port
 *
 *  Fork copies the task structure, the kernel-stack trap frame and the
 *  user page tables (copy-on-write). kernel_thread() creates kernel-mode
 *  tasks (used by init before execve).
 */

#include <errno.h>
#include <string.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

long last_pid=0;

int find_empty_process(void);

int copy_process(int nr)
{
	struct task_struct *p;
	struct trapframe *tf, *ptf;
	unsigned long new_pgdir;
	int i;
	struct file *f;

	p = (struct task_struct *) get_free_page();
	if (!p)
		return -EAGAIN;
	task[nr] = p;
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
	p->state = TASK_UNINTERRUPTIBLE;
	p->pid = last_pid;
	p->father = current->pid;
	p->counter = p->priority;
	p->signal = 0;
	p->alarm = 0;
	p->leader = 0;		/* process leadership doesn't inherit */
	p->utime = p->stime = 0;
	p->cutime = p->cstime = 0;
	p->start_time = jiffies;
	if (!current->pgdir)
		panic("fork from kernel task");
	if (!(new_pgdir = get_free_page())) {
		task[nr] = NULL;
		free_page((long) p);
		return -EAGAIN;
	}
	memset((char *) new_pgdir, 0, PAGE_SIZE);
	p->pgdir = new_pgdir;
	/* copy the user trap frame; child returns 0 from fork */
	ptf = (struct trapframe *)((char *) current + PAGE_SIZE - TF_SIZE);
	tf = (struct trapframe *)((char *) p + PAGE_SIZE - TF_SIZE);
	memcpy(tf, ptf, TF_SIZE);
	tf->a0 = 0;		/* a0 = 0 */
	tf->mepc += 4;		/* skip the ecall */
	tf->from_user = 1;
	p->kctx.ra = (long) ret_from_trap;
	p->kctx.sp = (long) tf;
	if (copy_page_tables(current->pgdir, new_pgdir)) {
		task[nr] = NULL;
		free_page(new_pgdir);
		free_page((long) p);
		return -EAGAIN;
	}
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;
	if (current->pwd)
		current->pwd->i_count++;
	if (current->root)
		current->root->i_count++;
	if (current->executable)
		current->executable->i_count++;
	p->state = TASK_RUNNING;	/* do this last, just in case */
	return last_pid;
}

int sys_fork(void)
{
	int nr;

	if ((nr = find_empty_process()) < 0)
		return -EAGAIN;
	return copy_process(nr);
}

/* create a kernel-mode task running fn (used by init before exec) */
int kernel_thread(void (*fn)(void))
{
	struct task_struct *p;
	int nr, i;
	struct file *f;

	if ((nr = find_empty_process()) < 0)
		return -EAGAIN;
	p = (struct task_struct *) get_free_page();
	if (!p)
		return -EAGAIN;
	task[nr] = p;
	*p = *current;
	p->state = TASK_UNINTERRUPTIBLE;
	p->pid = last_pid;
	p->father = current->pid;
	p->counter = p->priority;
	p->signal = 0;
	p->alarm = 0;
	p->leader = 0;
	p->utime = p->stime = 0;
	p->cutime = p->cstime = 0;
	p->start_time = jiffies;
	p->pgdir = 0;			/* kernel task */
	p->kctx.ra = (long) fn;
	p->kctx.sp = (long) ((char *) p + PAGE_SIZE);
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;
	if (current->pwd)
		current->pwd->i_count++;
	if (current->root)
		current->root->i_count++;
	if (current->executable)
		current->executable->i_count++;
	p->state = TASK_RUNNING;
	return last_pid;
}

int find_empty_process(void)
{
	int i;

	repeat:
		if ((++last_pid)<0) last_pid=1;
		for(i=0 ; i<NR_TASKS ; i++)
			if (task[i] && task[i]->pid == last_pid) goto repeat;
	for(i=1 ; i<NR_TASKS ; i++)
		if (!task[i])
			return i;
	return -EAGAIN;
}
