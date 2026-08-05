# Linux 0.11 for RISC-V (rv32) - QEMU virt, bare-metal M-mode
#
# Tools: riscv64-unknown-elf-gcc (rv32imac/ilp32 multilib), QEMU.

CROSS = riscv64-unknown-elf-
CC = $(CROSS)gcc
LD = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy

MARCH = -march=rv32imac_zicsr_zifencei -mabi=ilp32
KCFLAGS = -O2 -Wall -fno-pic -mno-relax -ffreestanding -fgnu89-inline \
	-fno-builtin -nostdinc -Iinclude $(MARCH)
LDFLAGS = -nostdlib -mno-relax $(MARCH)

KOBJS = \
	boot/head.o \
	init/main.o \
	kernel/sched.o \
	kernel/trap.o \
	kernel/traps.o \
	kernel/fork.o \
	kernel/panic.o \
	kernel/printk.o \
	kernel/vsprintf.o \
	kernel/sys.o \
	kernel/exit.o \
	kernel/signal.o \
	kernel/mktime.o \
	mm/memory.o \
	kernel/blk_drv/ll_rw_blk.o \
	kernel/blk_drv/ramdisk.o \
	kernel/chr_drv/tty_io.o \
	kernel/chr_drv/console.o \
	kernel/chr_drv/tty_ioctl.o \
	fs/bitmap.o fs/block_dev.o fs/buffer.o fs/char_dev.o fs/exec.o \
	fs/fcntl.o fs/file_dev.o fs/file_table.o fs/inode.o fs/ioctl.o \
	fs/namei.o fs/open.o fs/pipe.o fs/read_write.o fs/stat.o \
	fs/super.o fs/truncate.o \
	lib/ctype.o lib/_exit.o lib/open.o lib/close.o lib/errno.o \
	lib/write.o lib/dup.o lib/setsid.o lib/execve.o lib/wait.o \
	lib/string.o lib/malloc.o lib/pause.o lib/sync.o

all: kernel.elf rootfs.img

kernel.elf: $(KOBJS) rootfs.o kernel.ld
	$(LD) $(LDFLAGS) -T kernel.ld -o $@ $(KOBJS) rootfs.o

%.o: %.c
	$(CC) $(KCFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(KCFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(KCFLAGS) -c -o $@ $<

# ---- user programs ----
UCFLAGS = $(KCFLAGS)
USER_LIBS = lib/_exit.o lib/errno.o lib/write.o lib/read.o lib/fork.o \
	lib/execve.o lib/wait.o

user/crt0.o: user/crt0.S
	$(CC) $(UCFLAGS) -c -o $@ $<

user/sh.elf: user/crt0.o user/sh.o $(USER_LIBS)
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^

user/hello.elf: user/crt0.o user/hello.o $(USER_LIBS)
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^

user/t_hello.elf: user/crt0.o user/t_hello.o $(USER_LIBS)
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^

# test programs: inline syscall wrappers, so no lib stubs except
# _exit/errno; uprint for printing; sigrestore for the signal test.
TESTS = t_fork t_cow t_fs t_pipe t_sched t_stress
TLIBS = user/crt0.o user/uprint.o lib/_exit.o lib/errno.o

user/t_fork.elf: user/crt0.o user/t_fork.o user/uprint.o \
		lib/_exit.o lib/errno.o
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^
user/t_cow.elf: user/crt0.o user/t_cow.o user/uprint.o \
		lib/_exit.o lib/errno.o
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^
user/t_fs.elf: user/crt0.o user/t_fs.o user/uprint.o \
		lib/_exit.o lib/errno.o
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^
user/t_pipe.elf: user/crt0.o user/t_pipe.o user/uprint.o \
		lib/_exit.o lib/errno.o
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^
user/t_sig.elf: user/crt0.o user/t_sig.o user/uprint.o user/sigrestore.o \
		lib/_exit.o lib/errno.o
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^
user/t_sched.elf: user/crt0.o user/t_sched.o user/uprint.o \
		lib/_exit.o lib/errno.o
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^
user/t_stress.elf: user/crt0.o user/t_stress.o user/uprint.o \
		lib/_exit.o lib/errno.o
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^
user/t_up.elf: user/crt0.o user/t_up.o user/uprint.o \
		lib/_exit.o lib/errno.o
	$(LD) $(LDFLAGS) -static -nostdlib -Wl,-e_start -o $@ $^

# ---- root filesystem (minix v1) ----
tools/mkfs: tools/mkfs.c
	gcc -O2 -Wall -o $@ $<

ROOTFS_PROGS = user/sh.elf user/hello.elf user/t_hello.elf \
	user/t_fork.elf user/t_cow.elf user/t_fs.elf user/t_pipe.elf \
	user/t_sig.elf user/t_sched.elf user/t_stress.elf user/t_up.elf

rootfs.img: $(ROOTFS_PROGS) tools/mkfs
	tools/mkfs rootfs.img $(ROOTFS_PROGS)

rootfs.S: rootfs.img
	printf '.section .rootfs,"a",@progbits\n.globl _binary_rootfs_img_start\n_binary_rootfs_img_start:\n.incbin "rootfs.img"\n.globl _binary_rootfs_img_end\n_binary_rootfs_img_end:\n' > $@

rootfs.o: rootfs.S
	$(CC) $(KCFLAGS) -c -o $@ $<

run: kernel.elf
	qemu-system-riscv32 -M virt -m 128M -bios none -kernel kernel.elf \
		-nographic -no-reboot

clean:
	rm -f $(KOBJS) rootfs.o kernel.elf rootfs.img user/sh.o user/hello.o \
		user/crt0.o user/sh.elf user/hello.elf tools/mkfs rootfs.S \
		user/uprint.o user/sigrestore.o user/t_hello.o \
		user/t_fork.o user/t_cow.o user/t_fs.o user/t_pipe.o \
		user/t_sig.o user/t_sched.o user/t_stress.o \
		user/t_hello.elf user/t_fork.elf user/t_cow.elf \
		user/t_fs.elf user/t_pipe.elf user/t_sig.elf \
		user/t_sched.elf user/t_stress.elf user/t_up.elf \
		user/t_up.o

.PHONY: all run clean
