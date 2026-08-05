#ifndef _ELF_H
#define _ELF_H

/* Minimal ELF32 definitions for the RISC-V exec loader */

#define ELF_MAGIC 0x464c457fUL
#define EM_RISCV 243
#define ET_EXEC 2
#define PT_LOAD 1

#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define EV_CURRENT 1

typedef struct {
	unsigned char e_ident[16];
	unsigned short e_type;
	unsigned short e_machine;
	unsigned long e_version;
	unsigned long e_entry;
	unsigned long e_phoff;
	unsigned long e_shoff;
	unsigned long e_flags;
	unsigned short e_ehsize;
	unsigned short e_phentsize;
	unsigned short e_phnum;
	unsigned short e_shentsize;
	unsigned short e_shnum;
	unsigned short e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	unsigned long p_type;
	unsigned long p_offset;
	unsigned long p_vaddr;
	unsigned long p_paddr;
	unsigned long p_filesz;
	unsigned long p_memsz;
	unsigned long p_flags;
	unsigned long p_align;
} Elf32_Phdr;

#endif
