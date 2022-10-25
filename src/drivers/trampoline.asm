[bits 16]
global smp_trampoline_start
global smp_trampoline_end

%define INFO_START smp_trampoline_end - smp_trampoline_start + 0x1000

struc BootInfo
	.done: resb 1
	.page_table: resd 1
	.apic_id: resb 1
	.entry: resq 1
	.stack: resq 1
endstruc

smp_trampoline_start:
	; disable interrupt and nmi
	cli
	in al, 0x70
	and al, 0x7F
	out 0x70, al
	in al, 0x71

	; load page table
	mov eax, dword [INFO_START + BootInfo.page_table]
	mov cr3, eax

	; enable PAE and PGE
	mov eax, cr4
	or eax, 1 << 5 | 1 << 7
	mov cr4, eax

	; set up long mode
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	; enable paging, protected mode and write protect
	mov eax, cr0
	or eax, 1 << 31 | 1 << 0 | 1 << 16
	mov cr0, eax

	lgdt [gdt.ptr - smp_trampoline_start + 0x1000]

	jmp 0x8:long_mode - smp_trampoline_start + 0x1000
gdt:
	.null: dq 0
	.kernel_code:
		dd 0
		db 0, 0x9A, 0xA << 4, 0
	.kernel_data:
		dd 0
		db 0, 0x92, 0xC << 4, 0
	.user_code:
		dd 0
		db 0, 0xFA, 0xA << 4, 0
	.user_data:
		dd 0
		db 0, 0xF2, 0xC << 4, 0
	.ptr:
		dw $ - gdt - 1
		dd gdt - smp_trampoline_start + 0x1000

[bits 64]
long_mode:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov byte [INFO_START + BootInfo.done], 1
	xor rdi, rdi
	mov dil, byte [INFO_START + BootInfo.apic_id]
	mov rsp, qword [INFO_START + BootInfo.stack]
	mov rax, qword [INFO_START + BootInfo.entry]
	jmp rax
smp_trampoline_end: