#ifndef _MM_H
#define _MM_H

#include <linux/head.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

/* Sv32 PTE bits */
#define PTE_V 1
#define PTE_R 2
#define PTE_W 4
#define PTE_X 8
#define PTE_U 16
#define PTE_G 32
#define PTE_A 64
#define PTE_D 128
/* Sv32 PTE encoding: PPN (22 bits) lives in bits [31:10], i.e.
 * pte = (phys_page >> 12) << 10 | flags. */
#define PTE_PPN_SHIFT 10
#define PTE_PPN_MASK  0x3fffffUL

static inline unsigned long pte_phys(unsigned long pte)
{
	return ((pte >> PTE_PPN_SHIFT) & PTE_PPN_MASK) << 12;
}

static inline unsigned long phys_ppn(unsigned long phys)
{
	return (phys >> 12) << PTE_PPN_SHIFT;
}

#define PTE_USER_RW (PTE_V|PTE_R|PTE_W|PTE_U|PTE_A|PTE_D)
#define PTE_USER_RX (PTE_V|PTE_R|PTE_X|PTE_U|PTE_A|PTE_D)
#define PTE_USER_RWX (PTE_V|PTE_R|PTE_W|PTE_X|PTE_U|PTE_A|PTE_D)

#define USER_PGD_ENTRIES (USER_LIMIT >> 22)

struct trapframe;

extern unsigned long get_free_page(void);
extern unsigned long put_page(unsigned long page, unsigned long address);
extern unsigned long put_page_pgdir(unsigned long pgdir, unsigned long page,
	unsigned long address);
extern void free_page(unsigned long addr);
extern int copy_page_tables(unsigned long from_pgdir, unsigned long to_pgdir);
extern int free_page_tables(unsigned long pgdir);
extern unsigned long translate_user(unsigned long addr);
extern void write_verify(unsigned long address);
extern void get_empty_page(unsigned long address);
extern void do_wp_page(unsigned long address);
extern void do_no_page(unsigned long address);
extern void page_fault_handler(struct trapframe * tf, unsigned long address);

#endif
