# linux/kernel/trap.S - RISC-V trap entry/exit and context switch
#
# task_struct layout (see include/linux/sched.h):
#   0   kctx.ra
#   4   kctx.sp
#   8   kctx.s0 ... 52 kctx.s11
#   56  pgdir
#
# trapframe layout (140 bytes, see struct trapframe):
#   0 mstatus, 4 mepc, 8..128 x1..x31, 132 mcause, 136 from_user

.equ KCTX_RA,    0
.equ KCTX_SP,    4
.equ KCTX_S0,    8
.equ KCTX_S1,    12
.equ KCTX_S2,    16
.equ KCTX_S3,    20
.equ KCTX_S4,    24
.equ KCTX_S5,    28
.equ KCTX_S6,    32
.equ KCTX_S7,    36
.equ KCTX_S8,    40
.equ KCTX_S9,    44
.equ KCTX_S10,   48
.equ KCTX_S11,   52
.equ TASK_PGDIR, 56

.equ TF_MSTATUS,  0
.equ TF_MEPC,     4
.equ TF_X1,       8
.equ TF_X2,       12
.equ TF_X3,       16
.equ TF_X4,       20
.equ TF_X5,       24
.equ TF_X6,       28
.equ TF_X7,       32
.equ TF_X8,       36
.equ TF_X9,       40
.equ TF_X10,      44
.equ TF_X11,      48
.equ TF_X12,      52
.equ TF_X13,      56
.equ TF_X14,      60
.equ TF_X15,      64
.equ TF_X16,      68
.equ TF_X17,      72
.equ TF_X18,      76
.equ TF_X19,      80
.equ TF_X20,      84
.equ TF_X21,      88
.equ TF_X22,      92
.equ TF_X23,      96
.equ TF_X24,      100
.equ TF_X25,      104
.equ TF_X26,      108
.equ TF_X27,      112
.equ TF_X28,      116
.equ TF_X29,      120
.equ TF_X30,      124
.equ TF_X31,      128
.equ TF_MCAUSE,   132
.equ TF_FROMUSER, 136
.equ TF_F0,       140
.equ TF_FCSR,     396
.equ TF_SIZE,     400

.section .text
.globl trap_entry
.globl ret_from_trap
.globl switch_to

trap_entry:
	csrr t0, mstatus
	srli t0, t0, 11
	andi t0, t0, 3
	bnez t0, 1f
	# came from user mode: swap in kernel stack (mscratch)
	csrrw sp, mscratch, sp
	li t0, 1
	j 2f
1:	li t0, 0
2:	addi sp, sp, -TF_SIZE
	# enable FP (mstatus.FS = Dirty) before saving mstatus, so the
	# fsw/flw context save below is legal and the saved mstatus keeps
	# FS enabled for user mode.
	li t1, 0x6000
	csrs mstatus, t1
	csrr t1, mstatus
	sw t1, TF_MSTATUS(sp)
	csrr t1, mepc
	sw t1, TF_MEPC(sp)
	sw x1, TF_X1(sp)
	beqz t0, 3f
	# user mode: user sp is in mscratch
	csrr t1, mscratch
	sw t1, TF_X2(sp)
	j 4f
3:	# kernel mode: saved sp = sp + TF_SIZE
	mv t1, sp
	addi t1, t1, TF_SIZE
	sw t1, TF_X2(sp)
4:	sw x3, TF_X3(sp)
	sw x4, TF_X4(sp)
	sw x5, TF_X5(sp)
	sw x6, TF_X6(sp)
	sw x7, TF_X7(sp)
	sw x8, TF_X8(sp)
	sw x9, TF_X9(sp)
	sw x10, TF_X10(sp)
	sw x11, TF_X11(sp)
	sw x12, TF_X12(sp)
	sw x13, TF_X13(sp)
	sw x14, TF_X14(sp)
	sw x15, TF_X15(sp)
	sw x16, TF_X16(sp)
	sw x17, TF_X17(sp)
	sw x18, TF_X18(sp)
	sw x19, TF_X19(sp)
	sw x20, TF_X20(sp)
	sw x21, TF_X21(sp)
	sw x22, TF_X22(sp)
	sw x23, TF_X23(sp)
	sw x24, TF_X24(sp)
	sw x25, TF_X25(sp)
	sw x26, TF_X26(sp)
	sw x27, TF_X27(sp)
	sw x28, TF_X28(sp)
	sw x29, TF_X29(sp)
	sw x30, TF_X30(sp)
	sw x31, TF_X31(sp)
	# save FP state (f0-f31, fcsr) with 64-bit stores; FS is Dirty.
	fsd f0, TF_F0(sp)
	fsd f1, TF_F0+8(sp)
	fsd f2, TF_F0+16(sp)
	fsd f3, TF_F0+24(sp)
	fsd f4, TF_F0+32(sp)
	fsd f5, TF_F0+40(sp)
	fsd f6, TF_F0+48(sp)
	fsd f7, TF_F0+56(sp)
	fsd f8, TF_F0+64(sp)
	fsd f9, TF_F0+72(sp)
	fsd f10, TF_F0+80(sp)
	fsd f11, TF_F0+88(sp)
	fsd f12, TF_F0+96(sp)
	fsd f13, TF_F0+104(sp)
	fsd f14, TF_F0+112(sp)
	fsd f15, TF_F0+120(sp)
	fsd f16, TF_F0+128(sp)
	fsd f17, TF_F0+136(sp)
	fsd f18, TF_F0+144(sp)
	fsd f19, TF_F0+152(sp)
	fsd f20, TF_F0+160(sp)
	fsd f21, TF_F0+168(sp)
	fsd f22, TF_F0+176(sp)
	fsd f23, TF_F0+184(sp)
	fsd f24, TF_F0+192(sp)
	fsd f25, TF_F0+200(sp)
	fsd f26, TF_F0+208(sp)
	fsd f27, TF_F0+216(sp)
	fsd f28, TF_F0+224(sp)
	fsd f29, TF_F0+232(sp)
	fsd f30, TF_F0+240(sp)
	fsd f31, TF_F0+248(sp)
	csrr t3, fcsr		# t3 is already saved; keep t0 (from_user) intact
	sw t3, TF_FCSR(sp)
	csrr t1, mcause
	sw t1, TF_MCAUSE(sp)
	sw t0, TF_FROMUSER(sp)
	mv a0, sp
	call trap_handler
	# fall through to ret_from_trap

ret_from_trap:
	# keep the frame pointer in mscratch
	csrw mscratch, sp
	lw t0, TF_MSTATUS(sp)
	csrw mstatus, t0
	lw t0, TF_MEPC(sp)
	csrw mepc, t0
	lw t0, TF_X1(sp)	# ra
	sw t0, TF_MSTATUS(sp)	# stash ra in the consumed slot
	lw t0, TF_X2(sp)	# sp
	sw t0, TF_MEPC(sp)	# stash sp in the consumed slot
	lw x3, TF_X3(sp)
	lw x4, TF_X4(sp)
	lw x5, TF_X5(sp)
	lw x6, TF_X6(sp)
	lw x7, TF_X7(sp)
	lw x8, TF_X8(sp)
	lw x9, TF_X9(sp)
	lw x10, TF_X10(sp)
	lw x11, TF_X11(sp)
	lw x12, TF_X12(sp)
	lw x13, TF_X13(sp)
	lw x14, TF_X14(sp)
	lw x15, TF_X15(sp)
	lw x16, TF_X16(sp)
	lw x17, TF_X17(sp)
	lw x18, TF_X18(sp)
	lw x19, TF_X19(sp)
	lw x20, TF_X20(sp)
	lw x21, TF_X21(sp)
	lw x22, TF_X22(sp)
	lw x23, TF_X23(sp)
	lw x24, TF_X24(sp)
	lw x25, TF_X25(sp)
	lw x26, TF_X26(sp)
	lw x27, TF_X27(sp)
	lw x28, TF_X28(sp)
	lw x29, TF_X29(sp)
	lw x30, TF_X30(sp)
	# x31 (t6) left as the only scratch register
	csrr t6, mscratch	# frame pointer
	lw x1, TF_MSTATUS(t6)	# ra
	lw x2, TF_MEPC(t6)	# sp (user sp, or pre-trap sp for kernel traps)
	# point SATP at the current task's page directory. Harmless in M-mode
	# (M-mode is always bare); needed for the return to user mode.
	la t5, current
	lw t5, 0(t5)
	lw t5, TASK_PGDIR(t5)
	srli t5, t5, 12
	lui t6, 0x80000		# Sv32 MODE bit
	or t5, t5, t6
	csrw satp, t5
	sfence.vma
	# mscratch = kernel stack top for the next trap entry
	csrr t6, mscratch
	addi t6, t6, TF_SIZE
	csrw mscratch, t6
	# restore x30/x31 from the frame
	csrr t6, mscratch
	addi t6, t6, -TF_SIZE
	# restore FP state first (t6 is still the frame pointer; restoring
	# x31 would clobber t6)
	fld f0, TF_F0(t6)
	fld f1, TF_F0+8(t6)
	fld f2, TF_F0+16(t6)
	fld f3, TF_F0+24(t6)
	fld f4, TF_F0+32(t6)
	fld f5, TF_F0+40(t6)
	fld f6, TF_F0+48(t6)
	fld f7, TF_F0+56(t6)
	fld f8, TF_F0+64(t6)
	fld f9, TF_F0+72(t6)
	fld f10, TF_F0+80(t6)
	fld f11, TF_F0+88(t6)
	fld f12, TF_F0+96(t6)
	fld f13, TF_F0+104(t6)
	fld f14, TF_F0+112(t6)
	fld f15, TF_F0+120(t6)
	fld f16, TF_F0+128(t6)
	fld f17, TF_F0+136(t6)
	fld f18, TF_F0+144(t6)
	fld f19, TF_F0+152(t6)
	fld f20, TF_F0+160(t6)
	fld f21, TF_F0+168(t6)
	fld f22, TF_F0+176(t6)
	fld f23, TF_F0+184(t6)
	fld f24, TF_F0+192(t6)
	fld f25, TF_F0+200(t6)
	fld f26, TF_F0+208(t6)
	fld f27, TF_F0+216(t6)
	fld f28, TF_F0+224(t6)
	fld f29, TF_F0+232(t6)
	fld f30, TF_F0+240(t6)
	fld f31, TF_F0+248(t6)
	lw t0, TF_FCSR(t6)
	csrw fcsr, t0
	lw x30, TF_X30(t6)
	lw x31, TF_X31(t6)
	mret

switch_to:
	# a0 = next task
	la t0, current
	lw t1, 0(t0)
	sw ra, KCTX_RA(t1)
	sw sp, KCTX_SP(t1)
	sw s0, KCTX_S0(t1)
	sw s1, KCTX_S1(t1)
	sw s2, KCTX_S2(t1)
	sw s3, KCTX_S3(t1)
	sw s4, KCTX_S4(t1)
	sw s5, KCTX_S5(t1)
	sw s6, KCTX_S6(t1)
	sw s7, KCTX_S7(t1)
	sw s8, KCTX_S8(t1)
	sw s9, KCTX_S9(t1)
	sw s10, KCTX_S10(t1)
	sw s11, KCTX_S11(t1)
	sw a0, 0(t0)		# current = next
	lw ra, KCTX_RA(a0)
	lw sp, KCTX_SP(a0)
	lw s0, KCTX_S0(a0)
	lw s1, KCTX_S1(a0)
	lw s2, KCTX_S2(a0)
	lw s3, KCTX_S3(a0)
	lw s4, KCTX_S4(a0)
	lw s5, KCTX_S5(a0)
	lw s6, KCTX_S6(a0)
	lw s7, KCTX_S7(a0)
	lw s8, KCTX_S8(a0)
	lw s9, KCTX_S9(a0)
	lw s10, KCTX_S10(a0)
	lw s11, KCTX_S11(a0)
	# mscratch = next kernel stack top (task struct page + PAGE_SIZE)
	lui t0, 1
	add t0, a0, t0
	csrw mscratch, t0
	ret
