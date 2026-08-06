/*
 * extprog.c - standalone rv32 static program compiled OUTSIDE the tree.
 * Uses Linux 0.11 syscall numbers directly (write=4, exit=1).
 */

void _start(void)
{
	const char msg[] = "hello from external program\n";
	register long a7 __asm__("a7") = 4;	/* __NR_write */
	register long a0 __asm__("a0") = 1;	/* stdout */
	register long a1 __asm__("a1") = (long) msg;
	register long a2 __asm__("a2") = (long) (sizeof(msg) - 1);

	__asm__ volatile ("ecall"
		: "+r" (a7), "+r" (a0), "+r" (a1), "+r" (a2)
		:: "memory", "t0", "t1");

	a7 = 1;			/* __NR_exit */
	a0 = 0;
	__asm__ volatile ("ecall" : "+r" (a7), "+r" (a0)
		:: "memory", "t0", "t1");
	for (;;) ;
}
