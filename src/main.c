#include "boot_info.h"
#include <stdbool.h>
#include "stdio.h"
#include "paging/memory.h"
#include "interrupts/idt.h"
#include "drivers/apic.h"
#include "drivers/pci.h"
#include "timers/timers.h"
#include "acpi/acpi.h"

__attribute__((noreturn)) void kmain(BootInfo boot_info) {
	initialize_printf(&boot_info.framebuffer, boot_info.font_start);
	printf("before mem\n");
	initialize_memory(&boot_info);
	printf("after mem\n");
	initialize_interrupts();
	if (!initialize_pci(boot_info.rsdp)) {
		printf("failed to initialize pci\n");
		while (true) __asm__ volatile("hlt");
	}

	initialize_apic(boot_info.rsdp);
	initialize_timers(boot_info.rsdp);
	lai_host_init(boot_info.rsdp);
	enumerate_pci();

	printf("hello world\n");

	//lai_enter_sleep(5);
	//printf("shutdown didn't work\n");

	while (true) __asm__("hlt");
}