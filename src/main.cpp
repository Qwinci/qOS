#include "bootInfo.hpp"
#include "console/renderer.hpp"
#include "paging/pageFrameAllocator.hpp"
#include "paging/pageTableManager.hpp"

Renderer globalRenderer;
PageFrameAllocator globalAllocator;

extern uintptr_t kernelEnd;

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	globalRenderer = Renderer {bootInfo.frameBuffer, bootInfo.fontStart};
	globalRenderer.setColor(0x00FF00);

	globalAllocator = PageFrameAllocator {bootInfo.memoryMap};

	PageTableManager pageTableManager;

	for (size_t i = bootInfo.frameBuffer.address; i < bootInfo.frameBuffer.address +
	(bootInfo.frameBuffer.width * bootInfo.frameBuffer.height * (bootInfo.frameBuffer.bpp / 8)); i += 0x1000) {
		pageTableManager.mapMemory(i, i);
	}

	globalRenderer << kernelEnd << std::endl;

	for (size_t i = 0; i < 0xFFFF; i += 0x1000) {
		pageTableManager.mapMemory(i, i);
	}

	for (size_t i = 0; i < kernelEnd; i += 0x1000) {
		pageTableManager.mapMemory(bootInfo.kernelVirtualAddress + i, bootInfo.kernelPhysicalAddress + i);
	}

	pageTableManager.refresh();

	globalRenderer << "Hi from new page table!" << std::endl;

	while (true) {
		asm("hlt");
	}
}