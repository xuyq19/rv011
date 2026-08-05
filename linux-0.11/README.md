# Linux 0.11 for RISC-V (rv32) on QEMU

这是一个把 Linux 0.11 移植到 RISC-V（rv32imac，M-mode 裸机）的工程，
在 QEMU `virt` 机器上从 `-bios none` 直接启动，不使用 OpenSBI/SBI。

## 构建与运行

依赖：`riscv64-unknown-elf-gcc`（需 rv32imac_zicsr_zifencei/ilp32 多库）、
`qemu-system-riscv32`。

```sh
make -j8            # 生成 kernel.elf
make run            # qemu-system-riscv32 -M virt -m 128M -bios none -kernel kernel.elf -nographic
```

启动后进入一个极简 shell，支持 `/bin/<程序名>` 的查找执行：

```
sh# t_fork
sh# t_fs
sh# t_stress
sh# exit
```

## 实现概况

### 启动与运行环境

- `boot/head.s`：M-mode 入口，清零 BSS、建立引导栈、配置 PMP
  （QEMU 在未配置 PMP 时 S/U 模式访问全部失败）、进入 `main()`。
- 全部中断/异常留在 M-mode（未做 delegation），`mtvec` 指向统一入口。
- 16550 UART（0x10000000）轮询收发；定时器用 CLINT。
- 根文件系统是 2MB 的 Minix v1 ramdisk，通过 `rootfs.S` 的 `.incbin`
  直接嵌进内核镜像（不要改用 objcopy，会写 0 破坏数据）。

### 内存管理（Sv32）

- 每个用户任务有自己的 Sv32 页目录（物理页在 0x80000000+）。
- 内核运行在 M-mode（无地址翻译），访问用户内存通过
  `translate_user()` 手工查页表。
- PTE 编码必须是标准 Sv32：PPN 占 bits[31:10]，即
  `pte = (phys >> 12) << 10 | flags`。早期版本把物理地址直接放进 PTE，
  内核自查页表自洽、但硬件翻译失效（首次进用户态取指即 fault）。
- fork 采用写时复制（COW）；`brk` 增长堆。

### 陷阱与系统调用

- `kernel/trap.S`：用户态 trap 交换到内核栈顶，保存完整 trapframe（144B）；
  内核态 trap 在当前 sp 压帧（保证可嵌套）。
- ecall 用 a7 传系统调用号，a0-a2 传参；73 个系统调用与 0.11 对齐。
- **注意**：M-mode trap 入口会先用 t0/t1 做临时寄存器，因此用户态 ecall
  包装必须把 t0/t1 声明为 clobber（见 `include/unistd.h` 的 `_syscallN`
  宏），否则编译器会认为 ecall 后 t0/t1 仍有效，导致输出/逻辑错乱。
- 定时器中断处理里递增 `jiffies`（x86 版在汇编里做），否则
  `alarm()/times()` 永远不触发。

### 文件系统

- `tools/mkfs.c` 生成 Minix v1 镜像：块 0 引导块、块 1 超级块、
  块 2/3 inode/zone 位图、块 4+ inode 表、之后为数据区。
- 约定：`i_zone[]` 存**块号**（区 N = 块 firstdata+N-1），zmap 存区号。
- 移植中修正了若干原版依赖 x86 语义的问题：
  - `clear_bit` 返回"原本是否已清除"（x86 `btrl+setnb` 语义）；
  - `mount_root` 统计空闲区/inode 时只读位图，不再用 set_bit 把位图标满；
  - `sys_creat` 补上 `O_WRONLY`；
  - exec 后设置 `end_code`，保证 `brk` 边界正确。

### 用户程序

- `user/crt0.S`：ELF32 入口，映射到 0x10000，栈在 0x7fe00000 顶。
- `user/sh.c`：极简 shell（内置 `hello`/`exit`，其余命令按 `/bin/<cmd>` 执行）。
- 测试程序（`user/t_*.c`）：

| 程序 | 覆盖内容 |
|------|----------|
| t_fork | fork/exec/wait、退出码、pid/ppid |
| t_cow | COW 数据隔离、brk 堆增长 + fork |
| t_fs | 文件读写/lseek/硬链接/unlink/mkdir/chdir/stat/access/dup2/fcntl |
| t_pipe | 管道父子进程 4KB 传输 + 应答 |
| t_sig | SIGALRM handler + restorer、kill/SIGKILL、信号屏蔽 |
| t_sched | 12 个并发忙碌子进程的调度与退出 |
| t_stress | fork+exec×20、8 路并发 FS、120MB 堆压力、管道乒乓×100、COW×8 |

## 已知限制

- 单 hart、无虚拟内存换页、无浮点；`ftime/ptrace/acct` 等返回 -ENOSYS。
- UART 为轮询模式，无输入中断；终端输入在 shell read 时轮询。
- 根文件系统为内存盘，重启后改动丢失。

## 目录结构

```
boot/head.s          引导与 PMP
kernel/trap.S        trap 入口/返回、任务切换
kernel/traps.c       异常与系统调用分发
kernel/sched.c       调度、定时器、alarm
mm/memory.c          Sv32 页表、COW、用户地址翻译
fs/                  Minix v1 文件系统
kernel/blk_drv/      ramdisk 块驱动
kernel/chr_drv/      16550 控制台
tools/mkfs.c         rootfs 镜像生成
user/                shell、libc 封装、测试程序
```
