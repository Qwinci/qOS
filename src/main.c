#include "boot_info.h"
#include <stdbool.h>
#include "stdio.h"
#include "paging/memory.h"
#include "interrupts/idt.h"
#include "drivers/apic.h"

__attribute__((noreturn)) void kmain(BootInfo boot_info) {
	initialize_printf(&boot_info.framebuffer, boot_info.font_start);
	initialize_memory(&boot_info);
	initialize_interrupts();

	initialize_apic(boot_info.rsdp);
	//printf("%d %d\n", 29, 29);
	//printf("%d\n", 123);
	//printf("%d\n", 10);
	//*(uintptr_t*) 1 = 10;
	printf("hello world\n");

	while (true) __asm__("hlt");
}