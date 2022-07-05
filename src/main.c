#include "boot_info.h"
#include <stdbool.h>
#include "stdio.h"
#include "paging/memory.h"

__attribute__((noreturn)) void kmain(BootInfo boot_info) {
	initialize_printf(&boot_info.framebuffer, boot_info.font_start);
	initialize_memory(&boot_info);
	printf("hello world\n");

	while (true) __asm__("hlt");
}