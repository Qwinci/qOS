#include "pageTableManager.hpp"
#include "console/renderer.hpp"

void PageTableManager::mapMemory(uint64_t virtualAddress, uint64_t physicalAddress, PageFlag flags) {
	virtualAddress >>= 12;
	uint64_t PT_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PD_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PDP_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PML4_i = virtualAddress & 0x1FF;

	flags = flags | PageFlag::Present;

	uint64_t* PDP;
	if (((uint64_t*)((uintptr_t) PML4 + offset))[PML4_i] & PageFlag::Present) {
		PDP = reinterpret_cast<uint64_t*>(((uint64_t*)((uintptr_t) PML4 + offset))[PML4_i] & 0xFFFFFFFFFF000);
	}
	else {
		PDP = reinterpret_cast<uint64_t*>((uint64_t) globalAllocator.allocatePage() - offset);
		memset(reinterpret_cast<void*>((uintptr_t) PDP + offset), 0, 0x1000);
		((uint64_t*)((uintptr_t) PML4 + offset))[PML4_i] |= reinterpret_cast<uint64_t>(PDP) | flags;
	}

	uint64_t* PD;
	if (((uint64_t*)((uintptr_t) PDP + offset))[PDP_i] & PageFlag::Present) {
		PD = reinterpret_cast<uint64_t*>((((uint64_t*)((uintptr_t) PDP + offset))[PDP_i] & 0xFFFFFFFFFF000));
	}
	else {
		PD = reinterpret_cast<uint64_t*>((uintptr_t) globalAllocator.allocatePage() - offset);
		memset(reinterpret_cast<void*>((uintptr_t) PD + offset), 0, 0x1000);
		((uint64_t*)((uintptr_t) PDP + offset))[PDP_i] |= reinterpret_cast<uint64_t>(PD) | flags;
	}

	uint64_t* PT;
	if (((uint64_t*)((uintptr_t) PD + offset))[PD_i] & PageFlag::Present) {
		PT = reinterpret_cast<uint64_t*>((((uint64_t*)((uintptr_t) PD + offset))[PD_i] & 0xFFFFFFFFFF000));
	}
	else {
		PT = reinterpret_cast<uint64_t*>((uintptr_t) globalAllocator.allocatePage() - offset);
		memset(reinterpret_cast<void*>((uintptr_t) PT + offset), 0, 0x1000);
		((uint64_t*)((uintptr_t) PD + offset))[PD_i] |= reinterpret_cast<uint64_t>(PT) | flags;
	}

	((uint64_t*)((uintptr_t) PT + offset))[PT_i] |= physicalAddress | flags;
}

void PageTableManager::refresh() {
	asm("mov %0, %%cr3" : : "r"(PML4));
}

void PageTableManager::refreshPage(uint64_t address) {
	asm("invlpg (%0)" : : "r"(address));
}

void PageTableManager::changeOffset() {
	offset = 0xffff800000000000;
}

void PageTableManager::unmapMemory(uint64_t virtualAddress) {
	virtualAddress >>= 12;
	uint64_t PT_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PD_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PDP_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PML4_i = virtualAddress & 0x1FF;

	uint64_t* PDP;
	if (((uint64_t*)((uintptr_t) PML4 + offset))[PML4_i] & PageFlag::Present) {
		PDP = reinterpret_cast<uint64_t*>(((uint64_t*)((uintptr_t) PML4 + offset))[PML4_i] & 0xFFFFFFFFFF000);
	}
	else {
		return;
	}

	uint64_t* PD;
	if (((uint64_t*)((uintptr_t) PDP + offset))[PDP_i] & PageFlag::Present) {
		PD = reinterpret_cast<uint64_t*>((((uint64_t*)((uintptr_t) PDP + offset))[PDP_i] & 0xFFFFFFFFFF000));
	}
	else {
		return;
	}

	uint64_t* PT;
	if (((uint64_t*)((uintptr_t) PD + offset))[PD_i] & PageFlag::Present) {
		PT = reinterpret_cast<uint64_t*>((((uint64_t*)((uintptr_t) PD + offset))[PD_i] & 0xFFFFFFFFFF000));
	}
	else {
		return;
	}

	((uint64_t*)((uintptr_t) PT + offset))[PT_i] &= ~static_cast<uint8_t>(PageFlag::Present);
}
