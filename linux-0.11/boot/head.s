# linux/boot/head.s - RISC-V boot entry (QEMU virt, M-mode, -bios none)
#
# QEMU loads the kernel at 0x80000000 and starts the hart in M-mode.
# No SBI/OpenSBI is used: this is a self-contained port, in the spirit
# of Linux 0.11.

.section .text.init,"ax",@progbits
.globl _start
_start:
	# a0 = mhartid (ignored; single-hart port)
	# zero .bss
	la t0, _bss_start
	la t1, _bss_end
	beq t0, t1, 2f
1:	sw zero, 0(t0)
	addi t0, t0, 4
	bltu t0, t1, 1b
2:	# stack + scratch
	la sp, _stack_top
	csrw mscratch, sp
	# configure PMP: allow S/U-mode access to all of memory
	# (QEMU denies S/U accesses when no PMP entry matches)
	li t0, -1
	csrw pmpaddr0, t0
	li t0, 0x0f		# TOR, R|W|X
	csrw pmpcfg0, t0
	sfence.vma
	call main
3:	wfi
	j 3b

.section .bss
.align 12
.globl _stack_bottom
_stack_bottom:
	.skip 16384
.globl _stack_top
_stack_top:
