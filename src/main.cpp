#include "bootInfo.hpp"
#include "console/font.hpp"
#include "console/renderer.hpp"

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	/*for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			*((uint32_t*)(bootInfo.frameBuffer.address + bootInfo.frameBuffer.bpp / 8 * bootInfo.frameBuffer.width * y +
			              bootInfo.frameBuffer.bpp / 8 * x)) = 0x00FF00;
		}
	}*/
	int xOff = 0;
	constexpr int SCALE = 4;
	constexpr uint32_t FGCOLOR = 0x00FF00;
	constexpr uint32_t BGCOLOR = 0;

	const char string[] = R"(!"#)";
	uint64_t stringLength = sizeof(string) / sizeof(*string);

	for (int i = 0; i < stringLength; ++i) {
		for (int y = 0; y < 16 * SCALE; ++y) {
			for (int x = 0; x < 8 * SCALE; ++x) {
				bool solid = font[(string[i] - 32) * 16 + y / SCALE] & 1 << (7 - x / SCALE);
				*((uint32_t*)(bootInfo.frameBuffer.address + bootInfo.frameBuffer.bpp / 8 * bootInfo.frameBuffer.width * y +
				              bootInfo.frameBuffer.bpp / 8 * x + xOff * SCALE)) = solid ? FGCOLOR : BGCOLOR;
			}
		}
		xOff += 8 * SCALE;
	}

	while (true) {
		asm("hlt");
	}
}