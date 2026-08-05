# RISC-V user program entry
.section .text
.globl _start
_start:
	lw a0, 0(sp)		# argc
	addi a1, sp, 4		# argv
	addi a2, sp, 8		# envp
	call main
	call _exit
1:	j 1b
