#include "bootInfo.hpp"
#include "console/renderer.hpp"
#include "paging/pageFrameAllocator.hpp"
#include "paging/pageTableManager.hpp"
#include "acpi/acpi.hpp"

Renderer globalRenderer;
PageFrameAllocator globalAllocator;

extern char kernelEnd[];

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	globalRenderer = Renderer {bootInfo.frameBuffer, bootInfo.fontStart};
	globalRenderer.setColor(0x00FF00);

	globalAllocator = PageFrameAllocator {bootInfo.memoryMap};

	PageTableManager pageTableManager;

	auto mcfg = findTable(static_cast<RSDP*>(bootInfo.rsdp), "MCFG");

	globalRenderer << "Hi!" << std::endl;

	/*for (size_t i = bootInfo.frameBuffer.address; i < bootInfo.frameBuffer.address +
	(bootInfo.frameBuffer.width * bootInfo.frameBuffer.height * (bootInfo.frameBuffer.bpp / 8)); i += 0x1000) {
		pageTableManager.mapMemory(i, i);
	}

	for (size_t i = 0; i < 0xFFFF; i += 0x1000) {
		pageTableManager.mapMemory(i, i);
	}

	for (size_t i = 0; i < reinterpret_cast<uintptr_t>(kernelEnd) - bootInfo.kernelVirtualAddress; i += 0x1000) {
		pageTableManager.mapMemory(bootInfo.kernelVirtualAddress + i, bootInfo.kernelPhysicalAddress + i);
	}

	pageTableManager.refresh();*/

	//globalRenderer << "Hi from new page table!" << std::endl;

	while (true) {
		asm("hlt");
	}
}