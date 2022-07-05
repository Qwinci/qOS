#include "boot_info.h"
#include <stdbool.h>
#include "stdio.h"
#include "paging/memory.h"
#include "interrupts/idt.h"

__attribute__((noreturn)) void kmain(BootInfo boot_info) {
	initialize_printf(&boot_info.framebuffer, boot_info.font_start);
	initialize_memory(&boot_info);
	initialize_interrupts();
	printf("hello world\n");
	*(uintptr_t*) 1 = 10;

	while (true) __asm__("hlt");
}