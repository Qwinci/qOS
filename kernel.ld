OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(start)

PHDRS {
	text    PT_LOAD     FLAGS((1 << 0) | (1 << 2));
	rodata  PT_LOAD     FLAGS((1 << 2));
	data    PT_LOAD     FLAGS((1 << 1) | (1 << 2));
}

SECTIONS {
	. = 0xffffffff80000000;

	.text : {
		*(.text .text.*)
	} :text

	.rodata : ALIGN(CONSTANT(MAXPAGESIZE)) {
		*(.rodata .rodata.*)
	} :rodata

	.data : ALIGN(CONSTANT(MAXPAGESIZE)) {
		*(.data .data.*)
	} :data

	.bss : {
		*(COMMON)
		*(.bss .bss.*)
	} :data

	kernel_end = .;
}