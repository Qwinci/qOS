#include "bootInfo.hpp"
#include "console/renderer.hpp"
#include "paging/pageFrameAllocator.hpp"
#include "paging/pageTableManager.hpp"
#include "gdt/gdt.hpp"
#include "interrupts/idt.hpp"
#include "interrupts/pic.hpp"
#include "utils/io.hpp"

Renderer globalRenderer;
PageFrameAllocator globalAllocator;

extern char kernelEnd[];

PageTableManager globalPageTableManager;

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	globalRenderer = Renderer {bootInfo.frameBuffer, bootInfo.fontStart};
	globalRenderer.setColor(0x00FF00);

	globalAllocator = PageFrameAllocator {bootInfo.memoryMap};

	globalPageTableManager = PageTableManager();

	for (size_t i = 0; i < bootInfo.memoryMap.size; ++i) {
		uintptr_t address = bootInfo.memoryMap.entries[i].address;
		for (size_t i2 = 0; i2 < bootInfo.memoryMap.entries[i].size;) {
			if (address & 0xFFF) {
				address &= 0xFFFFFFFFFFFFF000;
				if (bootInfo.memoryMap.entries[i].type != MemoryType::FrameBuffer) {
					globalPageTableManager.mapMemory(0xffff800000000000 +
					                                 address + i2, address + i2, PageFlag::None);
				}
				else {
					globalPageTableManager.mapMemory(0xffff800000000000 +
					                                 address + i2, address + i2, PageFlag::RW);
				}
			}
			else {
				if (bootInfo.memoryMap.entries[i].type != MemoryType::FrameBuffer) {
					globalPageTableManager.mapMemory(0xffff800000000000 +
					                                 address + i2, address + i2, PageFlag::None);
				}
				else {
					globalPageTableManager.mapMemory(0xffff800000000000 +
					                                 address + i2, address + i2, PageFlag::RW);
				}
				i2 += 0x1000;
			}
		}
	}

	for (size_t i = 0; i < 0x100000000; i += 0x1000) {
		globalPageTableManager.mapMemory(0xffff800000000000 + i, i, PageFlag::None);
	}

	uint64_t virtualAddress = bootInfo.kernelVirtualAddress;
	uint64_t physicalAddress = bootInfo.kernelPhysicalAddress;
	for (size_t i = 0; i < reinterpret_cast<uintptr_t>(kernelEnd) - bootInfo.kernelVirtualAddress;) {
		if (physicalAddress & 0xFFF) {
			physicalAddress &= 0xFFFFFFFFFFFFF000;
			virtualAddress &= 0xFFFFFFFFFFFFF000;
			globalPageTableManager.mapMemory(virtualAddress + i, physicalAddress + i, PageFlag::None);
		}
		else {
			globalPageTableManager.mapMemory(virtualAddress + i, physicalAddress + i, PageFlag::None);
			i += 0x1000;
		}
	}

	globalAllocator.changeMapping();
	globalPageTableManager.refresh();
	globalRenderer << "Kernel mapping changed" << std::endl;
	globalPageTableManager.changeOffset();

	GDTDescriptor descriptor {sizeof(GDT) - 1, reinterpret_cast<uint64_t>(&gdt)};
	loadGDT(reinterpret_cast<GDT*>(&descriptor));
	initializeInterrupts();

	//auto madt = findTable(static_cast<RSDP*>(bootInfo.rsdp), "APIC");
	//initializeAPIC(static_cast<MADT*>(madt));

	asm("cli");
	initializePIC();
	out1(PIC1_DATA, 0b11111101);
	out1(PIC2_DATA, 0b11111111);
	asm("sti");

	//auto mcfg = findTable(static_cast<RSDP*>(bootInfo.rsdp), "MCFG");
	//enumeratePCI(static_cast<MCFG*>(mcfg));
	//globalRenderer << "PCI enumeration done" << std::endl;

	while (true) {
		asm("hlt");
	}
}