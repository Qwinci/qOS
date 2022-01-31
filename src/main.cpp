#include "bootInfo.hpp"

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			*((uint32_t*)(bootInfo.frameBuffer.address + bootInfo.frameBuffer.bpp / 8 * bootInfo.frameBuffer.width * y +
			              bootInfo.frameBuffer.bpp / 8 * x)) = 0x00FF00;
		}
	}
	while (true) {
		asm("hlt");
	}
}