#ifndef _ASM_IO_H
#define _ASM_IO_H

/*
 * RISC-V MMIO access. In M-mode everything is a physical address,
 * so "ports" are memory addresses.
 */
#define outb(value,port) (*(volatile unsigned char *)(port) = (unsigned char)(value))
#define outb_p(value,port) outb(value,port)
#define outw(value,port) (*(volatile unsigned short *)(port) = (unsigned short)(value))
#define outl(value,port) (*(volatile unsigned long *)(port) = (unsigned long)(value))

#define inb(port) (*(volatile unsigned char *)(port))
#define inb_p(port) inb(port)
#define inw(port) (*(volatile unsigned short *)(port))
#define inl(port) (*(volatile unsigned long *)(port))

#endif
