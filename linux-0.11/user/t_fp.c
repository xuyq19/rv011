/*
 * t_fp.c - floating-point smoke test (compiled with rv32imafdc/ilp32d).
 *
 * Standalone: raw ecalls only (Linux 0.11 syscall numbers, write=4,
 * exit=1). Runs a deterministic double loop and calls write() every 100
 * iterations, so the FP registers have to survive many traps. The
 * recurrence x = x*0.5 + 1.0 converges to exactly 2.0 (0x4000000000000000),
 * so the final bit pattern is deterministic: 0x4000000000000000 (2.0).
 */

static void wr(const char * s, int n)
{
	register long a7 __asm__("a7") = 4;	/* __NR_write */
	register long a0 __asm__("a0") = 1;	/* stdout */
	register long a1 __asm__("a1") = (long) s;
	register long a2 __asm__("a2") = n;

	__asm__ volatile ("ecall"
		: "+r" (a7), "+r" (a0), "+r" (a1), "+r" (a2)
		:: "memory", "t0", "t1");
}

static void prhex(unsigned long v)
{
	char b[9];
	int i;

	for (i = 7; i >= 0; i--) {
		unsigned d = v & 15;
		b[i] = (char) (d < 10 ? '0' + d : 'a' + d - 10);
		v >>= 4;
	}
	b[8] = 0;
	wr(b, 8);
}

void _start(void)
{
	union {
		double d;
		unsigned long long u;
	} uu;
	volatile double x = 0.0;
	int i;

	for (i = 0; i < 2000; i++) {
		x = x * 0.5 + 1.0;
		if ((i % 100) == 0)
			wr(".", 1);	/* trap: FP state must survive */
	}
	uu.d = x;
	wr("\nfp x=", 6);
	prhex((unsigned long) (uu.u >> 32));
	prhex((unsigned long) (uu.u & 0xffffffff));
	wr("\nfp ok\n", 7);

	{
		register long a7 __asm__("a7") = 1;	/* __NR_exit */
		register long a0 __asm__("a0") = 0;
		__asm__ volatile ("ecall" : "+r" (a7), "+r" (a0)
			:: "memory", "t0", "t1");
	}
	for (;;) ;
}
