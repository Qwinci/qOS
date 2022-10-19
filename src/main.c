#include "boot_info.h"
#include <stdbool.h>
#include "stdio.h"
#include "paging/memory.h"
#include "interrupts/idt.h"
#include "drivers/apic.h"
#include "drivers/pci.h"
#include "drivers/sb16.h"
#include "timers/timers.h"

__attribute__((noreturn)) void kmain(BootInfo boot_info) {
	initialize_printf(&boot_info.framebuffer, boot_info.font_start);
	initialize_memory(&boot_info);
	initialize_interrupts();

	initialize_apic(boot_info.rsdp);
	initialize_timers(boot_info.rsdp);
	initialize_pci(boot_info.rsdp);
	//if (initialize_sb16()) printf("sb16 initialized\n");

	printf("hello world\n");

	while (true) __asm__("hlt");
}