OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(start)

PHDRS
{
	null    PT_NULL     FLAGS(0);
	text    PT_LOAD     FLAGS((1 << 0) | (1 << 2));
	rodata  PT_LOAD     FLAGS((1 << 2));
	data    PT_LOAD     FLAGS((1 << 1) | (1 << 2));
}

SECTIONS
{
	. = 0xffffffff80000000;

	.text : {
		*(.text .text.*)
	} :text

	. += CONSTANT(MAXPAGESIZE);

	.stivale2hdr : {
		KEEP(*(.stivate2hdr))
	} :rodata

	.rodata : {
		*(.rodata .rodata.*)
	} :rodata

	. += CONSTANT(MAXPAGESIZE);

	.data : {
		*(.data .data.*)
	} :data

	kernelEnd = .;

	.bss : {
		*(COMMON)
		*(.bss .bss.*)
	} :data
}