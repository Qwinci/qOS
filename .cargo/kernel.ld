ENTRY(_start)

SECTIONS {
	. = 0xffffffff80000000;

	.text : {
		*(.text .text.*)
	} :text

	.rodata : ALIGN(0x1000) {
		*(.rodata .rodata.*)
	} :rodata

	.data : ALIGN(0x1000) {
		*(.data .data.*)
	}

	.bss : {
		*(COMMON)
		*(.bss .bss.*)
	} :data
}