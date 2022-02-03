#include "bootInfo.hpp"
#include "console/renderer.hpp"
#include "paging/pageFrameAllocator.hpp"
#include "paging/pageTableManager.hpp"
#include "acpi/acpi.hpp"
#include "gdt/gdt.hpp"
#include "interrupts/idt.hpp"

Renderer globalRenderer;
PageFrameAllocator globalAllocator;

extern char kernelEnd[];

PageTableManager globalPageTableManager;

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	globalRenderer = Renderer {bootInfo.frameBuffer, bootInfo.fontStart};
	globalRenderer.setColor(0x00FF00);

	globalAllocator = PageFrameAllocator {bootInfo.memoryMap};

	globalPageTableManager = PageTableManager();

	for (size_t i = 0; i < bootInfo.memoryMap.size; ++i) {
		uintptr_t address = bootInfo.memoryMap.entries[i].address;
		for (size_t i2 = 0; i2 < bootInfo.memoryMap.entries[i].size;) {
			if (address & 0xFFF) {
				globalRenderer << "Page aligning address " << Mode::Hex << "0x" << address << "..." << Mode::Normal << std::endl;
				address &= 0xFFFFFFFFFFFFF000;
				globalPageTableManager.mapMemory(0xffff800000000000 +
				address + i2, address + i2);
			}
			else {
				globalPageTableManager.mapMemory(0xffff800000000000 +
				address + i2, address + i2);
				i2 += 0x1000;
			}
		}
	}

	for (size_t i = 0; i < 0x100000000; i += 0x1000) {
		globalPageTableManager.mapMemory(0xffff800000000000 + i, i);
	}

	uint64_t virtualAddress = bootInfo.kernelVirtualAddress;
	uint64_t physicalAddress = bootInfo.kernelPhysicalAddress;
	for (size_t i = 0; i < reinterpret_cast<uintptr_t>(kernelEnd) - bootInfo.kernelVirtualAddress;) {
		if (physicalAddress & 0xFFF) {
			physicalAddress &= 0xFFFFFFFFFFFFF000;
			virtualAddress &= 0xFFFFFFFFFFFFF000;
			globalPageTableManager.mapMemory(virtualAddress + i, physicalAddress + i);
		}
		else {
			globalPageTableManager.mapMemory(virtualAddress + i, physicalAddress + i);
			i += 0x1000;
		}
	}

	globalAllocator.changeMapping();
	globalPageTableManager.refresh();
	globalRenderer << "Kernel mapping changed" << std::endl;

	GDTDescriptor descriptor {sizeof(GDT) - 1, reinterpret_cast<uint64_t>(&gdt)};
	loadGDT(reinterpret_cast<GDT*>(&descriptor));
	initializeInterrupts();

	auto mcfg = findTable(static_cast<RSDP*>((void*)(bootInfo.rsdp)), "MCFG");

	while (true) {
		asm("hlt");
	}
}