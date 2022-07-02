#include "boot_info.h"
#include <stdint.h>
#include <stdbool.h>

__attribute__((noreturn)) void kmain(BootInfo boot_info) {
	Framebuffer fb = boot_info.framebuffer;
	for (uint32_t y = 0; y < 100; ++y) {
		for (uint32_t x = 0; x < 100; ++x) {
			*(uint32_t*) (fb.address + fb.pitch * y + x * fb.bpp / 8) = 0xFF0000;
		}
	}

	while (true) __asm__("hlt");
}