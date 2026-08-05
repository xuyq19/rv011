/*
 *  linux/mm/memory.c - RISC-V Sv32 page table management
 *
 *  The kernel runs in M-mode (no translation). Each user task has its
 *  own Sv32 page directory; user pages are mapped U|R|W into it and the
 *  kernel reaches user memory by walking the page tables.
 */

#include <signal.h>
#include <string.h>

#include <asm/system.h>
#include <asm/memory.h>

#include <linux/sched.h>
#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/mm.h>

volatile void do_exit(long code);
void un_wp_page(unsigned long * table_entry);

#define PAGING_PAGES (RAM_SIZE>>12)
#define MAP_NR(addr) (((addr)-RAM_BASE)>>12)
#define USED 100

static unsigned char mem_map[PAGING_PAGES];

static inline void zero_page(unsigned long page)
{
	unsigned long * p = (unsigned long *) page;
	int i;

	for (i = 0; i < 1024; i++)
		p[i] = 0;
}

unsigned long get_free_page(void)
{
	int i;

	for (i = 0; i < PAGING_PAGES; i++)
		if (!mem_map[i]) {
			mem_map[i] = 1;
			zero_page(RAM_BASE + (i << 12));
			return RAM_BASE + (i << 12);
		}
	return 0;
}

void free_page(unsigned long addr)
{
	if (addr < RAM_BASE)
		return;
	if (addr >= RAM_END)
		panic("trying to free nonexistent page");
	if (!mem_map[MAP_NR(addr)])
		panic("trying to free free page");
	mem_map[MAP_NR(addr)]--;
}

/* return a pointer to the PTE for 'address', or NULL if no table exists */
unsigned long * pte_lookup(unsigned long pgdir, unsigned long address)
{
	unsigned long dir;

	if (!pgdir)
		return NULL;
	dir = ((unsigned long *) pgdir)[address >> 22];
	if (!(dir & PTE_V))
		return NULL;
	return (unsigned long *) pte_phys(dir) + ((address >> 12) & 0x3ff);
}

unsigned long put_page_pgdir(unsigned long pgdir, unsigned long page,
	unsigned long address)
{
	unsigned long dir, tmp;

	if (address >= USER_LIMIT) {
		printk("put_page: bad address %p\n\r", (void *) address);
		return 0;
	}
	if (page < RAM_BASE || page >= RAM_END) {
		printk("put_page: bad page %p\n\r", (void *) page);
		return 0;
	}
	dir = ((unsigned long *) pgdir)[address >> 22];
	if (!(dir & PTE_V)) {
		if (!(tmp = get_free_page()))
			return 0;
		((unsigned long *) pgdir)[address >> 22] = phys_ppn(tmp) | PTE_V;
		dir = phys_ppn(tmp) | PTE_V;
	}
	((unsigned long *) pte_phys(dir))[(address >> 12) & 0x3ff] =
		phys_ppn(page) | PTE_USER_RWX;
	return page;
}

unsigned long put_page(unsigned long page, unsigned long address)
{
	return put_page_pgdir(current->pgdir, page, address);
}

int free_page_tables(unsigned long pgdir)
{
	unsigned long * dir;
	unsigned long * pt;
	int i, j;

	if (!pgdir)
		return 0;
	dir = (unsigned long *) pgdir;
	for (i = 0; i < USER_PGD_ENTRIES; i++) {
		if (!(dir[i] & PTE_V))
			continue;
		pt = (unsigned long *) pte_phys(dir[i]);
		for (j = 0; j < 1024; j++)
			if (pt[j] & PTE_V)
				free_page(pte_phys(pt[j]));
		free_page((unsigned long) pt);
		dir[i] = 0;
	}
	free_page(pgdir);
	return 0;
}

/*
 * Copy the user portion of a page directory with copy-on-write:
 * both parent and child PTEs lose the W bit, page refcounts go up.
 */
int copy_page_tables(unsigned long from_pgdir, unsigned long to_pgdir)
{
	unsigned long * from_dir, * to_dir;
	unsigned long * from_pt, * to_pt;
	unsigned long this_page, new_table;
	int i, j;

	if (!from_pgdir || !to_pgdir)
		panic("copy_page_tables: bad pgdir");
	from_dir = (unsigned long *) from_pgdir;
	to_dir = (unsigned long *) to_pgdir;
	for (i = 0; i < USER_PGD_ENTRIES; i++) {
		if (!(from_dir[i] & PTE_V))
			continue;
		if (to_dir[i] & PTE_V)
			panic("copy_page_tables: already exist");
		if (!(new_table = get_free_page()))
			return -1;
		to_dir[i] = phys_ppn(new_table) | PTE_V;
		from_pt = (unsigned long *) pte_phys(from_dir[i]);
		to_pt = (unsigned long *) new_table;
		for (j = 0; j < 1024; j++) {
			this_page = from_pt[j];
			if (!(this_page & PTE_V))
				continue;
			this_page &= ~PTE_W;
			from_pt[j] = this_page;
			to_pt[j] = this_page;
			if (pte_phys(this_page) >= RAM_BASE)
				mem_map[MAP_NR(pte_phys(this_page))]++;
		}
	}
	return 0;
}

/*
 * Translate a user virtual address to a physical address by walking
 * the current task's page directory. Kernel tasks (pgdir == 0) use
 * physical/identity addressing.
 */
unsigned long translate_user(unsigned long addr)
{
	unsigned long * pte;

	if (!current->pgdir)
		return addr;
	if (addr >= USER_LIMIT)
		return (unsigned long) -1;
	pte = pte_lookup(current->pgdir, addr);
	if (!pte || !(*pte & PTE_V) || !(*pte & PTE_U))
		return (unsigned long) -1;
	return pte_phys(*pte) | (addr & 0xfff);
}

void uaccess_fault(void)
{
	printk("uaccess fault\n\r");
	do_exit(11);
}

/*
 * Make sure the page at 'address' is mapped and writable (COW it if
 * needed). Used for user buffers.
 */
void verify_area(void * addr, int size)
{
	unsigned long start = (unsigned long) addr;
	unsigned long * pte;

	if (!current->pgdir)
		return;			/* kernel task */
	if (start >= USER_LIMIT)
		panic("verify_area: bad address");
	size += start & 0xfff;
	start &= 0xfffff000;
	while (size > 0) {
		pte = pte_lookup(current->pgdir, start);
		if (!pte || !(*pte & PTE_V))
			get_empty_page(start);
		else if (!(*pte & PTE_W))
			un_wp_page(pte);
		size -= 4096;
		start += 4096;
	}
}

void un_wp_page(unsigned long * table_entry)
{
	unsigned long old_page, new_page;

	old_page = pte_phys(*table_entry);
	if (old_page >= RAM_BASE && mem_map[MAP_NR(old_page)] == 1) {
		*table_entry |= PTE_W;
		return;
	}
	if (!(new_page = get_free_page()))
		do_exit(11);
	if (old_page >= RAM_BASE)
		mem_map[MAP_NR(old_page)]--;
	*table_entry = phys_ppn(new_page) | PTE_USER_RW;
	copy_page(old_page, new_page);
}

void do_wp_page(unsigned long address)
{
	unsigned long * pte = pte_lookup(current->pgdir, address);

	if (pte && (*pte & PTE_V) && !(*pte & PTE_W))
		un_wp_page(pte);
	else
		do_no_page(address);
}

void get_empty_page(unsigned long address)
{
	unsigned long tmp;

	if (!(tmp = get_free_page()) || !put_page(tmp, address)) {
		free_page(tmp);		/* 0 is ok - ignored */
		do_exit(11);
	}
}

/* All executables are fully loaded at exec() time, so page faults only
 * need anonymous pages (stack, COW, bss). */
void do_no_page(unsigned long address)
{
	address &= 0xfffff000;
	get_empty_page(address);
}

void page_fault_handler(struct trapframe * tf, unsigned long address)
{
	unsigned long * pte;

	if (!tf->from_user || address >= USER_LIMIT) {
		printk("page fault: addr=%lx mepc=%lx\n\r", address, tf->mepc);
		do_exit(11);
	}
	address &= 0xfffff000;
	pte = pte_lookup(current->pgdir, address);
	if (pte && (*pte & PTE_V) && !(*pte & PTE_W))
		do_wp_page(address);
	else
		do_no_page(address);
}

void mem_init(long start_mem, long end_mem)
{
	int i;
	long start;

	start = (start_mem - RAM_BASE) >> 12;
	for (i = 0; i < PAGING_PAGES; i++)
		mem_map[i] = USED;
	end_mem -= start_mem;
	end_mem >>= 12;
	while (end_mem-- > 0)
		mem_map[start++] = 0;
}
