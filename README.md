# rv011 — Linux 0.11 移植到 RISC-V

把经典 Linux 0.11（1991）移植到 RISC-V（rv32imac）并跑在 QEMU `virt`
机器上的实验项目。无 OpenSBI，内核在 M-mode 裸机启动；用户态 U-mode
使用 Sv32 分页。启动后有一个带 `cd/pwd/echo/ls` 内置命令的极简 shell，
并附带完整的功能测试与压力测试程序。

## 仓库结构

```
linux-0.11/          移植后的内核源码（boot/head.s、kernel/、mm/、fs/、user/ 等）
  docs/设计文档.md   架构设计与移植踩坑记录
  docs/测试指南.md   功能/压力测试使用说明
```

## 快速开始

依赖：`riscv64-unknown-elf-gcc`（rv32imac_zicsr_zifencei/ilp32）、
`qemu-system-riscv32`。

```sh
cd linux-0.11
make -j8
make run
```

启动到 `sh#` 后：

```
sh# ls              →  bin  dev
sh# cd bin
sh# ls              →  sh  hello  t_hello  t_fork  t_cow  t_fs  t_pipe ...
sh# t_stress        →  综合压力测试（约 30 秒）
sh# exit
```

## 实现要点

- **启动**：M-mode 入口、PMP 配置、Sv32 分页、CLINT 定时器、16550 UART。
- **内存**：每个用户任务独立 Sv32 页目录，COW fork，内核经
  `translate_user()` 访问用户内存。
- **系统调用**：73 个与 0.11 对齐，ecall 用 a7 传调用号。
- **文件系统**：Minix v1 rootfs（2MB ramdisk）直接 `.incbin` 进内核镜像。
- **shell**：内置 `cd/pwd/echo/ls/exit`，其余命令按 `/bin/<cmd>` 执行，
  支持多行粘贴与退格（DEL/BS）。

更详细的设计与排坑说明见
[设计文档](linux-0.11/docs/设计文档.md)，
[修改点记录](linux-0.11/docs/修改点.md)；
测试方法见[测试指南](linux-0.11/docs/测试指南.md)。

## 测试状态

功能测试（fork/COW/文件系统/管道/信号/调度）与压力测试（综合五阶段、
120MB 堆压力、8 路并发文件系统、管道乒乓、COW 冲突）全部通过。
