/*
 *  linux/init/main.c - RISC-V port
 *
 *  Kernel entry from boot/head.s. Task 0 is the idle task; init is a
 *  kernel-mode task that execs /bin/sh (which becomes the first user
 *  process). The root filesystem is a ramdisk embedded in the kernel.
 */

#define __LIBRARY__
#include <unistd.h>

#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/system.h>
#include <asm/io.h>

#include <stddef.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>

static char printbuf[1024];

extern int vsprintf();
extern void init(void);
extern void blk_dev_init(void);
extern void chr_dev_init(void);
extern void tty_init(void);
extern void mem_init(long start, long end);
extern long rd_init(long mem_start, int length);
extern void buffer_init(long buffer_end);
extern void trap_init(void);
extern void sched_init(void);
extern void mount_root(void);
extern int kernel_thread(void (*fn)(void));
extern int sys_pause(void);
extern void uart_init(void);

static long memory_end = 0;
static long buffer_memory_end = 0;
static long main_memory_start = 0;

void main(void)		/* This really IS void, no error here. */
{
	ROOT_DEV = 0x0101;	/* ram disk: major 1, minor 1 */
	uart_init();
	memory_end = RAM_END;
	memory_end &= 0xfffff000;
	buffer_memory_end = PAGE_ALIGN((unsigned long) _kernel_end) +
		1024*1024;
	if (buffer_memory_end > memory_end)
		buffer_memory_end = memory_end;
	main_memory_start = buffer_memory_end;
	rd_init(0, 0);		/* wire up the ramdisk driver */
	mem_init(main_memory_start,memory_end);
	trap_init();
	blk_dev_init();
	chr_dev_init();
	tty_init();
	startup_time = 0;	/* no RTC on QEMU virt */
	sched_init();
	buffer_init(buffer_memory_end);
	mount_root();
	sti();
	kernel_thread(init);
	for (;;)
		pause();
}

static int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

static char * argv[] = { "/bin/sh", NULL };
static char * envp[] = { "HOME=/", NULL };

static void shell(void)
{
	close(0);
	close(1);
	close(2);
	setsid();
	(void) open("/dev/tty0",O_RDWR,0);
	(void) dup(0);
	(void) dup(0);
	execve("/bin/sh",argv,envp);
	_exit(2);
}

void init(void)
{
	int pid,i;

	(void) open("/dev/tty0",O_RDWR,0);
	(void) dup(0);
	(void) dup(0);
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);
	while (1) {
		if ((pid=kernel_thread(shell))<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();
	}
}
