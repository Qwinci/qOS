section .bruh

[bits 16]
global trampoline

trampoline:
	cli
	cld
	jmp 0:0x1040
ALIGN 16
_1010_gdtTable:
	dd 0, 0
	dd 0xFFFF, 0xCF9A00 ; flat code
	dd 0xFFFF, 0x8F9200 ; flat data
	dd 0x68, 0xCF8900 ; tss
_1030_gdtValue:
	dw _1030_gdtValue - _1010_gdtTable - 1
	dd 0x1010
	dd 0, 0
ALIGN 64
_1040:
	xor ax, ax
	mov ds, ax
	lgdt [0x1010]
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	jmp 8:0x1060
ALIGN 32
[bits 32]
_1060:
	mov ax, 16
	mov ds, ax
	mov ss, ax

	mov eax, 1
	cpuid
	shr ebx, 24
	mov edi, ebx

	; long mode enable
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	PRESENT equ 1 << 7
	NOT_SYS equ 1 << 4
	EXEC equ 1 << 3
	DC equ 1 << 2
	RW equ 1 << 1

	GRAN_4K equ 1 << 7
	SZ_32 equ 1 << 6
	LONG_MODE equ 1 << 5

GDT:
	.Null: equ $ - GDT
		dq 0
	.Code: equ $ - GDT
		dd 0xFFFF
		db 0
		db PRESENT | NOT_SYS | EXEC | RW
		db GRAN_4K | LONG_MODE | 0xF
		db 0
	.Data: equ $ - GDT
		dd 0xFFFF
		db 0
		db PRESENT | NOT_SYS | RW
		db GRAN_4K | SZ_32 | 0xF
		db 0
	.TSS: equ $ - GDT
		dd 0x68
		dd 0xCF8900
	.Pointer:
		dw $ - GDT - 1
		dq GDT

			lgdt [GDT.Pointer]

			mov eax, [0x2000]
			mov cr3, eax

			mov eax, cr0
			mov ebx, 1 << 31
			or eax, ebx ; enable paging
			mov cr0, eax

			jmp GDT.Code:Prepare64

[bits 64]
extern ap_start
extern works
Prepare64:
	mov [works], byte 1
	cli
	mov ax, GDT.Data
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp ap_start