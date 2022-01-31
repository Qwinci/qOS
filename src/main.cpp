#include "bootInfo.hpp"
#include "console/renderer.hpp"

Renderer globalRenderer;

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	/*for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			*((uint32_t*)(bootInfo.frameBuffer.address + bootInfo.frameBuffer.bpp / 8 * bootInfo.frameBuffer.width * y +
			              bootInfo.frameBuffer.bpp / 8 * x)) = 0x00FF00;
		}
	}*/
	globalRenderer = Renderer {bootInfo.frameBuffer, bootInfo.fontStart};
	globalRenderer.setColor(0x00FF00);

	globalRenderer << 'n' << '\n';
	globalRenderer << "hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!\n";
	globalRenderer << 'd' << Mode::Binary << 10 << "\n" << Mode::Hex << 84895 << Mode::Normal;

	while (true) {
		asm("hlt");
	}
}