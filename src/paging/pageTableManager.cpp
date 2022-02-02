#include "pageTableManager.hpp"

#define F_PRESENT (1 << 0)
#define F_RW (1 << 1)

void PageTableManager::mapMemory(uint64_t virtualAddress, uint64_t physicalAddress) {
	virtualAddress >>= 12;
	uint64_t PT_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PD_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PDP_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PML4_i = virtualAddress & 0x1FF;

	unsigned int flags = F_PRESENT | F_RW;

	uint64_t* PDP;
	if (PML4[PML4_i] & F_PRESENT) {
		PDP = reinterpret_cast<uint64_t*>(PML4[PML4_i] & 0xFFFFFFFFFF000);
	}
	else {
		PDP = static_cast<uint64_t*>(globalAllocator.allocatePage());
		memset(PDP, 0, 0x1000);
		PML4[PML4_i] |= reinterpret_cast<uint64_t>(PDP) | flags;
	}

	uint64_t* PD;
	if (PDP[PDP_i] & F_PRESENT) {
		PD = reinterpret_cast<uint64_t*>(PDP[PDP_i] & 0xFFFFFFFFFF000);
	}
	else {
		PD = static_cast<uint64_t*>(globalAllocator.allocatePage());
		memset(PD, 0, 0x1000);
		PDP[PDP_i] |= reinterpret_cast<uint64_t>(PD) | flags;
	}

	uint64_t* PT;
	if (PD[PD_i] & F_PRESENT) {
		PT = reinterpret_cast<uint64_t*>(PD[PD_i] & 0xFFFFFFFFFF000);
	}
	else {
		PT = static_cast<uint64_t*>(globalAllocator.allocatePage());
		memset(PT, 0, 0x1000);
		PD[PD_i] |= reinterpret_cast<uint64_t>(PT) | flags;
	}

	PT[PT_i] |= physicalAddress | flags;
}

void PageTableManager::refresh() {
	asm("mov %0, %%cr3" : : "r"(PML4));
}
