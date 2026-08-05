/*
 * tools/mkfs.c - build a Minix v1 (14-bit, 1024-byte blocks) filesystem
 * image with a fixed set of files. Used to create the ramdisk rootfs.
 *
 * Usage: mkfs <image> <file...>
 *
 * Layout: block 0 boot, 1 superblock, 2 inode bitmap, 3 zone bitmap,
 *         4.. inode table, then data zones.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BLOCK 1024
#define NINODES 96
#define IMAGE_BLOCKS 2048
#define SUPER_MAGIC 0x137F

struct d_super_block {
	uint16_t s_ninodes;
	uint16_t s_nzones;
	uint16_t s_imap_blocks;
	uint16_t s_zmap_blocks;
	uint16_t s_firstdatazone;
	uint16_t s_log_zone_size;
	uint32_t s_max_size;
	uint16_t s_magic;
};

struct d_inode {
	uint16_t i_mode;
	uint16_t i_uid;
	uint32_t i_size;
	uint32_t i_time;
	uint8_t i_gid;
	uint8_t i_nlinks;
	uint16_t i_zone[9];
};

struct dir_entry {
	uint16_t inode;
	char name[14];
};

static unsigned char img[IMAGE_BLOCKS * BLOCK];
static int inode_blocks = (NINODES * sizeof(struct d_inode) + BLOCK - 1) / BLOCK;
static int first_data = 2 + 1 + 1 + 0;	/* set after inode_blocks computed */
static int nzones;
static unsigned char * imap = img + 2 * BLOCK;
static unsigned char * zmap = img + 3 * BLOCK;
static struct d_inode * inodes = (struct d_inode *)(img + 4 * BLOCK);
static int next_zone = 1;
static int next_inode = 1;

static void set_bit(unsigned char * map, int bit)
{
	map[bit >> 3] |= (1 << (bit & 7));
}

static int alloc_zone(void)
{
	int z = next_zone++;
	if (z > nzones) {
		fprintf(stderr, "mkfs: out of zones\n");
		exit(1);
	}
	set_bit(zmap, z);
	return z;
}

static int alloc_inode(void)
{
	int i = next_inode++;
	if (i > NINODES) {
		fprintf(stderr, "mkfs: out of inodes\n");
		exit(1);
	}
	set_bit(imap, i);
	return i;
}

/* i_zone[] stores block numbers; data zone 'z' lives at block
 * first_data + z - 1 */
static unsigned char * zone_ptr(int z)
{
	return img + (first_data + z - 1) * BLOCK;
}

static unsigned char * block_ptr(int block)
{
	return img + block * BLOCK;
}

static void add_dir_entry(struct d_inode * dir, int ino, const char * name)
{
	int i, off;

	for (i = 0; i < 7 && dir->i_zone[i]; i++) {
		unsigned char * block = block_ptr(dir->i_zone[i]);
		for (off = 0; off < BLOCK; off += sizeof(struct dir_entry)) {
			struct dir_entry * de = (struct dir_entry *)(block + off);
			if (de->inode == 0) {
				de->inode = ino;
				memset(de->name, 0, 14);
				strncpy(de->name, name, 14);
				dir->i_size += sizeof(struct dir_entry);
				return;
			}
		}
	}
	fprintf(stderr, "mkfs: directory full\n");
	exit(1);
}

static int add_dir(const char * name, int parent_ino)
{
	int ino = alloc_inode();
	struct d_inode * in = &inodes[ino - 1];
	int z = alloc_zone();
	struct dir_entry * de;

	memset(in, 0, sizeof(*in));
	in->i_mode = 0040755;
	in->i_uid = 0;
	in->i_gid = 0;
	in->i_nlinks = 2;
	in->i_zone[0] = first_data + z - 1;	/* block number */
	in->i_size = 2 * sizeof(struct dir_entry);
	memset(block_ptr(in->i_zone[0]), 0, BLOCK);
	de = (struct dir_entry *) block_ptr(in->i_zone[0]);
	de->inode = ino;
	strcpy(de->name, ".");
	de = (struct dir_entry *)(block_ptr(in->i_zone[0]) + sizeof(struct dir_entry));
	de->inode = parent_ino;
	strcpy(de->name, "..");
	(void) name;
	return ino;
}

static int add_file(const char * path, int mode,
	const unsigned char * data, int size)
{
	int ino = alloc_inode();
	struct d_inode * in = &inodes[ino - 1];
	int i;

	memset(in, 0, sizeof(*in));
	in->i_mode = mode;
	in->i_uid = 0;
	in->i_gid = 0;
	in->i_size = size;
	in->i_nlinks = 1;
	for (i = 0; i < size; i += BLOCK) {
		int z = alloc_zone();
		int n = size - i;
		if (n > BLOCK)
			n = BLOCK;
		memcpy(zone_ptr(z), data + i, n);
		/* minix v1 stores BLOCK numbers in i_zone[] (zone N =
		 * block first_data + N - 1); the zmap keeps zone numbers */
		in->i_zone[i / BLOCK] = first_data + z - 1;
	}
	if (size > 7 * BLOCK) {
		fprintf(stderr, "mkfs: %s too big for direct zones\n", path);
		exit(1);
	}
	return ino;
}

static int add_dev(const char * name, uint16_t dev)
{
	int ino = alloc_inode();
	struct d_inode * in = &inodes[ino - 1];

	memset(in, 0, sizeof(*in));
	in->i_mode = 0020666;	/* char device */
	in->i_uid = 0;
	in->i_gid = 0;
	in->i_nlinks = 1;
	in->i_zone[0] = dev;
	(void) name;
	return ino;
}

int main(int argc, char ** argv)
{
	struct d_super_block * sb;
	unsigned char * data;
	long size;
	FILE * f;
	int root, bin, dev, sh_ino = 0, hello_ino = 0, tty_ino, i;

	if (argc < 4) {
		fprintf(stderr, "usage: %s image file1 file2 ...\n", argv[0]);
		return 1;
	}
	first_data = 2 + 1 + 1 + inode_blocks;
	nzones = IMAGE_BLOCKS - first_data;
	memset(img, 0, sizeof(img));

	sb = (struct d_super_block *)(img + BLOCK);
	sb->s_ninodes = NINODES;
	sb->s_nzones = nzones;
	sb->s_imap_blocks = 1;
	sb->s_zmap_blocks = 1;
	sb->s_firstdatazone = first_data;
	sb->s_log_zone_size = 0;
	sb->s_max_size = 7*BLOCK + 512*BLOCK + 512*512*BLOCK;
	sb->s_magic = SUPER_MAGIC;

	root = add_dir("/", 1);
	bin = add_dir("bin", root);
	dev = add_dir("dev", root);
	add_dir_entry(&inodes[root - 1], bin, "bin");
	add_dir_entry(&inodes[root - 1], dev, "dev");

	for (i = 2; i < argc; i++) {
		const char * base;
		if (!(f = fopen(argv[i], "rb"))) {
			perror(argv[i]);
			return 1;
		}
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		data = malloc(size ? size : 1);
		if (fread(data, 1, size, f) != (size_t) size) {
			perror("fread");
			return 1;
		}
		fclose(f);
		base = strrchr(argv[i], '/');
		base = base ? base + 1 : argv[i];
		if (!strcmp(base, "sh.elf"))
			sh_ino = add_file(argv[i], 0100755, data, size);
		else if (!strcmp(base, "hello.elf"))
			hello_ino = add_file(argv[i], 0100755, data, size);
		else {
			fprintf(stderr, "mkfs: unknown file %s\n", argv[i]);
			return 1;
		}
		free(data);
	}
	tty_ino = add_dev("tty0", 0x0400);	/* char dev major 4 minor 0 */
	add_dir_entry(&inodes[bin - 1], sh_ino, "sh");
	add_dir_entry(&inodes[bin - 1], hello_ino, "hello");
	add_dir_entry(&inodes[dev - 1], tty_ino, "tty0");

	if (!(f = fopen(argv[1], "wb"))) {
		perror(argv[1]);
		return 1;
	}
	if (fwrite(img, 1, sizeof(img), f) != sizeof(img)) {
		perror("fwrite");
		return 1;
	}
	fclose(f);
	printf("mkfs: wrote %s (%d blocks, %d zones)\n",
		argv[1], IMAGE_BLOCKS, nzones);
	return 0;
}
