/* Signal restorer: return from a user signal handler via sys_sigreturn. */
.section .text
.globl sig_restorer
sig_restorer:
	li a7, 72		# __NR_sigreturn
	ecall
1:	j 1b
