#ifndef _ASM_SYSTEM_H
#define _ASM_SYSTEM_H

/*
 * RISC-V (rv32, M-mode kernel) system macros.
 * Interrupt enable/disable maps to mstatus.MIE.
 */
#define sti() __asm__ volatile ("csrsi mstatus, 8" ::: "memory")
#define cli() __asm__ volatile ("csrci mstatus, 8" ::: "memory")
#define nop() __asm__ volatile ("nop"::)

#define iret() __asm__ volatile ("mret"::)

/* x86 compatibility no-ops: no IDT/GDT/segments on RISC-V */
#define set_intr_gate(n,addr) ((void)0)
#define set_trap_gate(n,addr) ((void)0)
#define set_system_gate(n,addr) ((void)0)
#define set_tss_desc(n,addr) ((void)0)
#define set_ldt_desc(n,addr) ((void)0)

struct task_struct;
extern void switch_to(struct task_struct * next);
extern void ret_from_trap(void);

#endif
