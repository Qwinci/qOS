#include "memory.hpp"
#include "pageFrameAllocator.hpp"
#include "pageTableManager.hpp"

PageFrameAllocator globalAllocator;

extern char kernelEnd[];
uintptr_t kernelVirtualAddress;

PageTableManager globalPageTableManager; // NOLINT(cert-err58-cpp)

void initializeMemory(BootInfo& bootInfo) {
	globalAllocator = PageFrameAllocator {bootInfo.memoryMap};

	globalPageTableManager = PageTableManager();

	for (size_t i = 0; i < bootInfo.memoryMap.size; ++i) {
		uintptr_t address = bootInfo.memoryMap.entries[i].address;
		for (size_t i2 = 0; i2 < bootInfo.memoryMap.entries[i].size;) {
			if (address & 0xFFF) {
				address &= 0xFFFFFFFFFFFFF000;
				if (bootInfo.memoryMap.entries[i].type != MemoryType::FrameBuffer) {
					globalPageTableManager.mapMemory(0xffff800000000000 +
					                                 address + i2, address + i2);
				}
				else {
					globalPageTableManager.mapMemory(0xffff800000000000 +
					                                 address + i2, address + i2, PageFlag::RW);
				}
			}
			else {
				if (bootInfo.memoryMap.entries[i].type != MemoryType::FrameBuffer) {
					globalPageTableManager.mapMemory(0xffff800000000000 +
					                                 address + i2, address + i2);
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
		globalPageTableManager.mapMemory(0xffff800000000000 + i, i);
	}

	uint64_t virtualAddress = bootInfo.kernelVirtualAddress;
	kernelVirtualAddress = virtualAddress;
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
	globalPageTableManager.changeOffset();
}