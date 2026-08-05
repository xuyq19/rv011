/*
 *  linux/kernel/chr_drv/console.c - RISC-V port
 *
 *  The QEMU "virt" machine console is a 16550 UART at 0x10000000.
 *  Output is polled; input is polled from tty_read via con_poll().
 */

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/system.h>

#define UART_THR  (UART_BASE + 0)
#define UART_RBR  (UART_BASE + 0)
#define UART_FCR  (UART_BASE + 2)
#define UART_LCR  (UART_BASE + 3)
#define UART_LSR  (UART_BASE + 5)

void uart_init(void)
{
	outb(0x03, UART_LCR);		/* 8N1 */
	outb(0x01, UART_FCR);		/* FIFO enable */
}

void uart_putc(char c)
{
	while (!(inb(UART_LSR) & 0x20))
		;
	outb(c, UART_THR);
}

int uart_getc(void)
{
	if (inb(UART_LSR) & 0x01)
		return inb(UART_RBR) & 0xff;
	return -1;
}

void uart_puts(char * s)
{
	while (*s)
		uart_putc(*s++);
}

void con_write(struct tty_struct * tty)
{
	cli();
	while (!EMPTY(tty->write_q)) {
		char c;

		GETCH(tty->write_q, c);
		uart_putc(c);
	}
	sti();
	wake_up(&tty->write_q.proc_list);
}

/* poll the UART and feed the console tty (called from tty_read) */
void con_poll(void)
{
	struct tty_struct * tty = &tty_table[0];
	int c;

	cli();
	while ((c = uart_getc()) >= 0)
		if (!FULL(tty->read_q))
			PUTCH(c, tty->read_q);
	if (!EMPTY(tty->read_q))
		copy_to_cooked(tty);
	sti();
}

void con_init(void)
{
	uart_init();
}
