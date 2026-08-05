.section .rootfs,"a",@progbits
.globl _binary_rootfs_img_start
_binary_rootfs_img_start:
.incbin "rootfs.img"
.globl _binary_rootfs_img_end
_binary_rootfs_img_end:
