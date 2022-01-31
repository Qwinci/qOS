#include "bootInfo.hpp"
#include "console/renderer.hpp"
#include "paging/pageFrameAllocator.hpp"
#include "paging/pageTableManager.hpp"

Renderer globalRenderer;
PageFrameAllocator globalAllocator;

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	globalRenderer = Renderer {bootInfo.frameBuffer, bootInfo.fontStart};
	globalRenderer.setColor(0x00FF00);

	globalAllocator = PageFrameAllocator {bootInfo.memoryMap};

	PageTableManager pageTableManager;

	for (size_t i = 0; i < globalAllocator.getTotalMemory(); i += 0x1000) {
		pageTableManager.mapMemory(i, i);
	}

	for (size_t i = bootInfo.frameBuffer.address; i < bootInfo.frameBuffer.address + (bootInfo.frameBuffer.width * bootInfo.frameBuffer.height * bootInfo.frameBuffer.bpp) + 0x1000; i += 0x1000) {
		pageTableManager.mapMemory(i, i);
	}

	pageTableManager.refresh();

	globalRenderer << "Hi from new page table!" << std::endl;

	while (true) {
		asm("hlt");
	}
}