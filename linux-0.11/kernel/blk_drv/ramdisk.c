/*
 *  linux/kernel/blk_drv/ramdisk.c - RISC-V port
 *
 *  The root filesystem image is linked into the kernel (rootfs.o,
 *  section .rootfs). The ramdisk driver serves blocks from it.
 */

#include <string.h>

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/memory.h>

#define MAJOR_NR 1
#include "blk.h"

extern char _binary_rootfs_img_start[];
extern char _binary_rootfs_img_end[];

char	*rd_start;
int	rd_length = 0;

void do_rd_request(void)
{
	int	len;
	char	*addr;

	INIT_REQUEST;
	addr = rd_start + (CURRENT->sector << 9);
	len = CURRENT->nr_sectors << 9;
	if ((MINOR(CURRENT->dev) != 1) || (addr+len > rd_start+rd_length)) {
		end_request(0);
		goto repeat;
	}
	if (CURRENT-> cmd == WRITE) {
		(void ) memcpy(addr,
			      CURRENT->buffer,
			      len);
	} else if (CURRENT->cmd == READ) {
		(void) memcpy(CURRENT->buffer, 
			      addr,
			      len);
	} else
		panic("unknown ramdisk-command");
	end_request(1);
	goto repeat;
}

/*
 * Wire up the ramdisk driver. The image is already in the kernel image,
 * so no memory needs to be reserved.
 */
long rd_init(long mem_start, int length)
{
	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	rd_start = _binary_rootfs_img_start;
	rd_length = _binary_rootfs_img_end - _binary_rootfs_img_start;
	printk("Ram disk: %d bytes\n\r", rd_length);
	return 0;
}
