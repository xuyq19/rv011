#ifndef _HEAD_H
#define _HEAD_H

/*
 * RISC-V port: QEMU "virt" machine constants.
 * The kernel runs in M-mode; addresses are physical.
 */

#define RAM_BASE        0x80000000UL
#define RAM_SIZE        (128*1024*1024)
#define RAM_END         (RAM_BASE + RAM_SIZE)

#define UART_BASE       0x10000000UL

#define CLINT_BASE      0x02000000UL
#define CLINT_MTIME     (*(volatile unsigned long *)(CLINT_BASE + 0xBFF8))
#define CLINT_MTIMECMP  (*(volatile unsigned long *)(CLINT_BASE + 0x4000))
#define CLINT_TIMEBASE  10000000UL

#define USER_LIMIT      0x80000000UL   /* top of user virtual address space */

extern char _bss_start[];
extern char _bss_end[];
extern char _kernel_end[];

#endif
