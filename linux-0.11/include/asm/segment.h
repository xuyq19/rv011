#ifndef _ASM_SEGMENT_H
#define _ASM_SEGMENT_H

/*
 * RISC-V port: there are no x86 segments. "User space" accesses are
 * done by explicit page-table translation (see mm/memory.c).
 * For kernel tasks (current->pgdir == 0) accesses are physical/identity.
 */

extern unsigned long translate_user(unsigned long addr);
extern void uaccess_fault(void);

extern inline unsigned char get_fs_byte(const char * addr)
{
	unsigned long pa = translate_user((unsigned long) addr);
	if (pa == (unsigned long) -1)
		uaccess_fault();
	return *(volatile unsigned char *) pa;
}

extern inline unsigned short get_fs_word(const unsigned short * addr)
{
	unsigned long pa = translate_user((unsigned long) addr);
	if (pa == (unsigned long) -1)
		uaccess_fault();
	return *(volatile unsigned short *) pa;
}

extern inline unsigned long get_fs_long(const unsigned long * addr)
{
	unsigned long pa = translate_user((unsigned long) addr);
	if (pa == (unsigned long) -1)
		uaccess_fault();
	return *(volatile unsigned long *) pa;
}

extern inline void put_fs_byte(char val, char * addr)
{
	unsigned long pa = translate_user((unsigned long) addr);
	if (pa == (unsigned long) -1)
		uaccess_fault();
	*(volatile char *) pa = val;
}

extern inline void put_fs_word(short val, short * addr)
{
	unsigned long pa = translate_user((unsigned long) addr);
	if (pa == (unsigned long) -1)
		uaccess_fault();
	*(volatile short *) pa = val;
}

extern inline void put_fs_long(unsigned long val, unsigned long * addr)
{
	unsigned long pa = translate_user((unsigned long) addr);
	if (pa == (unsigned long) -1)
		uaccess_fault();
	*(volatile unsigned long *) pa = val;
}

extern inline unsigned long get_fs(void) { return 0; }
extern inline unsigned long get_ds(void) { return 0; }
extern inline void set_fs(unsigned long val) { }

#endif
