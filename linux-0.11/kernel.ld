OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
	. = 0x80000000;
	.text : {
		*(.text.init)
		*(.text .text.*)
	}
	.rodata : {
		*(.rodata .rodata.*)
	}
	.data : {
		*(.data .data.*)
	}
	.rootfs : {
		KEEP(*(.rootfs))
	}
	.bss : {
		_bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		_bss_end = .;
	}
	. = ALIGN(4096);
	_kernel_end = .;
	end = _kernel_end;
	/DISCARD/ : { *(.comment) *(.riscv.attributes) *(.note*) }
}
