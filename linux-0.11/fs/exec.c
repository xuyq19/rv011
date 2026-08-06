/*
 *  linux/fs/exec.c - RISC-V port
 *
 *  ELF32 (RISC-V) executable loader. Segments are fully loaded at
 *  exec() time; the stack/argument pages are mapped near the top of
 *  the user address space.
 */

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <elf.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/segment.h>

extern int sys_close(int fd);

#define MAX_ARG_PAGES 32
#define STACK_TOP  (0x7fe00000UL)
#define STACK_BASE (STACK_TOP - MAX_ARG_PAGES*PAGE_SIZE)

/*
 * create_tables() parses the env- and arg-strings in new user
 * memory and creates the pointer tables from them, and puts their
 * addresses on the "stack", returning the new stack pointer value.
 */
static unsigned long * create_tables(char * p,int argc,int envc)
{
	unsigned long *argv,*envp;
	unsigned long * sp;

	sp = (unsigned long *) (0xfffffffc & (unsigned long) p);
	sp -= envc+1;
	envp = sp;
	sp -= argc+1;
	argv = sp;
	put_fs_long((unsigned long)envp,--sp);
	put_fs_long((unsigned long)argv,--sp);
	put_fs_long((unsigned long)argc,--sp);
	while (argc-->0) {
		put_fs_long((unsigned long) p,argv++);
		while (get_fs_byte(p++)) /* nothing */ ;
	}
	put_fs_long(0,argv);
	while (envc-->0) {
		put_fs_long((unsigned long) p,envp++);
		while (get_fs_byte(p++)) /* nothing */ ;
	}
	put_fs_long(0,envp);
	return sp;
}

/*
 * count() counts the number of arguments/envelopes
 */
static int count(char ** argv)
{
	int i=0;
	char ** tmp;

	if (tmp = argv)
		while (get_fs_long((unsigned long *) (tmp++)))
			i++;

	return i;
}

/*
 * copy_strings() copies argument/envelope strings from user memory
 * to free pages in kernel mem, ready to be placed at the top of the
 * new user memory.
 */
static unsigned long copy_strings(int argc,char ** argv,unsigned long *page,
		unsigned long p)
{
	char *tmp, *pag = NULL;
	int len, offset = 0;

	if (!p)
		return 0;	/* bullet-proofing */
	while (argc-- > 0) {
		if (!(tmp = (char *)get_fs_long(((unsigned long *)argv)+argc)))
			panic("argc is wrong");
		len=0;		/* remember zero-padding */
		do {
			len++;
		} while (get_fs_byte(tmp++));
		if (p-len < 0)		/* this shouldn't happen - 128kB */
			return 0;
		while (len) {
			--p; --tmp; --len;
			if (--offset < 0) {
				offset = p % PAGE_SIZE;
				if (!(pag = (char *) page[p/PAGE_SIZE]) &&
				    !(pag = (char *) (page[p/PAGE_SIZE] =
				      get_free_page())))
					return 0;
			}
			*(pag + offset) = get_fs_byte(tmp);
		}
	}
	return p;
}

/* read 'len' bytes starting at file offset 'off' into 'buf' */
static int read_file(struct m_inode * inode, unsigned long off,
	char * buf, unsigned long len)
{
	while (len) {
		unsigned long block = off >> BLOCK_SIZE_BITS;
		unsigned long coff = off & (BLOCK_SIZE - 1);
		unsigned long n = BLOCK_SIZE - coff;
		struct buffer_head * bh;

		if (n > len)
			n = len;
		if (!(bh = bread(inode->i_dev, bmap(inode, block))))
			return -EIO;
		memcpy(buf, bh->b_data + coff, n);
		brelse(bh);
		buf += n;
		off += n;
		len -= n;
	}
	return 0;
}

static int load_segment(struct m_inode * inode, Elf32_Phdr * ph,
	unsigned long pgdir)
{
	unsigned long va = ph->p_vaddr;
	unsigned long vaddr = ph->p_vaddr & 0xfffff000;
	unsigned long vend = (ph->p_vaddr + ph->p_memsz + 0xfff) & 0xfffff000;
	unsigned long file_end = ph->p_offset + ph->p_filesz;

	while (vaddr < vend) {
		unsigned long page = get_free_page();
		unsigned long foff = ph->p_offset + (vaddr - va);

		if (!page)
			return -ENOMEM;
		if (foff < file_end) {
			unsigned long n = file_end - foff;

			if (n > PAGE_SIZE)
				n = PAGE_SIZE;
			if (read_file(inode, foff, (char *) page, n)) {
				free_page(page);
				return -EIO;
			}
		}
		if (!put_page_pgdir(pgdir, page, vaddr)) {
			free_page(page);
			return -ENOMEM;
		}
		vaddr += PAGE_SIZE;
	}
	return 0;
}

int do_execve(char * filename, char ** argv, char ** envp)
{
	struct m_inode * inode;
	Elf32_Ehdr eh;
	Elf32_Phdr ph;
	unsigned long page[MAX_ARG_PAGES];
	int i, argc, envc, retval;
	int from_kernel;
	unsigned long p = PAGE_SIZE*MAX_ARG_PAGES-4;
	unsigned long new_pgdir;
	struct trapframe * tf;

	from_kernel = !current->pgdir;
	for (i=0 ; i<MAX_ARG_PAGES ; i++)	/* clear page-table */
		page[i]=0;
	if (!(inode=namei(filename)))		/* get executables inode */
		return -ENOENT;
	argc = count(argv);
	envc = count(envp);
	if (!S_ISREG(inode->i_mode)) {	/* must be regular file */
		retval = -EACCES;
		goto exec_error2;
	}
	i = inode->i_mode;
	if (current->euid == inode->i_uid)
		i >>= 6;
	else if (current->egid == inode->i_gid)
		i >>= 3;
	if (!(i & 1) &&
	    !((inode->i_mode & 0111) && suser())) {
		retval = -ENOEXEC;
		goto exec_error2;
	}
	if (read_file(inode, 0, (char *) &eh, sizeof(eh))) {
		retval = -EACCES;
		goto exec_error2;
	}
	if (eh.e_ident[0] != 0x7f || eh.e_ident[1] != 'E' ||
	    eh.e_ident[2] != 'L' || eh.e_ident[3] != 'F' ||
	    eh.e_type != ET_EXEC || eh.e_machine != EM_RISCV ||
	    eh.e_phnum == 0 || eh.e_phentsize != sizeof(Elf32_Phdr) ||
	    eh.e_entry >= USER_LIMIT) {
		retval = -ENOEXEC;
		goto exec_error2;
	}
	p = copy_strings(envc,envp,page,p);
	p = copy_strings(argc,argv,page,p);
	if (!p) {
		retval = -ENOMEM;
		goto exec_error2;
	}
	p += STACK_BASE;	/* convert to an absolute user address */
	/* build the new address space before committing */
	if (!(new_pgdir = get_free_page())) {
		retval = -ENOMEM;
		goto exec_error2;
	}
	memset((char *) new_pgdir, 0, PAGE_SIZE);
	for (i=0 ; i<MAX_ARG_PAGES ; i++)
		if (page[i]) {
			if (!put_page_pgdir(new_pgdir, page[i],
			    STACK_BASE + i*PAGE_SIZE)) {
				retval = -ENOMEM;
				goto free_new;
			}
			page[i] = 0;
		}
	for (i=0 ; i<eh.e_phnum ; i++) {
		if (read_file(inode, eh.e_phoff + i*sizeof(ph),
		    (char *) &ph, sizeof(ph))) {
			retval = -EIO;
			goto free_new;
		}
		if (ph.p_type != PT_LOAD)
			continue;
		if (ph.p_vaddr >= USER_LIMIT ||
		    ph.p_vaddr + ph.p_memsz > STACK_BASE) {
			retval = -ENOEXEC;
			goto free_new;
		}
		if (load_segment(inode, &ph, new_pgdir)) {
			retval = -ENOMEM;
			goto free_new;
		}
	}
/* OK, This is the point of no return */
	if (current->executable)
		iput(current->executable);
	current->executable = NULL;
	for (i=0 ; i<32 ; i++)
		current->sigaction[i].sa_handler = NULL;
	for (i=0 ; i<NR_OPEN ; i++)
		if ((current->close_on_exec>>i)&1)
			sys_close(i);
	current->close_on_exec = 0;
	free_page_tables(current->pgdir);
	current->pgdir = new_pgdir;
	p = (unsigned long) create_tables((char *) p, argc, envc);
	current->start_code = 0;
	current->end_code = current->end_data = current->brk = 0;
	for (i=0 ; i<eh.e_phnum ; i++) {
		if (read_file(inode, eh.e_phoff + i*sizeof(ph),
		    (char *) &ph, sizeof(ph)))
			break;
		if (ph.p_type != PT_LOAD)
			continue;
		if (ph.p_vaddr < current->start_code)
			current->start_code = ph.p_vaddr;
		if (ph.p_vaddr + ph.p_memsz > current->end_code)
			current->end_code = ph.p_vaddr + ph.p_memsz;
		if (ph.p_vaddr + ph.p_memsz > current->end_data) {
			current->end_data = ph.p_vaddr + ph.p_memsz;
			current->brk = current->end_data;
		}
	}
	iput(inode);
	/* build the user trap frame */
	tf = (struct trapframe *)((char *) current + PAGE_SIZE - TF_SIZE);
	memset(tf, 0, TF_SIZE);
	tf->mstatus = 0x6088;		/* MPP=0, MPIE=1, MIE=1, FS=Dirty */
	tf->mepc = eh.e_entry;
	tf->sp = p;			/* user sp */
	tf->from_user = 1;
	current->start_stack = p & 0xfffff000;
	if (from_kernel) {
		/* abandon the kernel call stack and enter user mode */
		current->kctx.ra = (long) ret_from_trap;
		current->kctx.sp = (long) tf;
		__asm__ volatile ("mv sp, %0; j ret_from_trap"
			:: "r" (tf) : "memory");
		for (;;) ;
	}
	return 0;
free_new:
	free_page_tables(new_pgdir);
exec_error2:
	iput(inode);
exec_error1:
	for (i=0 ; i<MAX_ARG_PAGES ; i++)
		if (page[i])
			free_page(page[i]);
	return retval;
}

int sys_execve(char * filename, char ** argv, char ** envp)
{
	return do_execve(filename, argv, envp);
}
