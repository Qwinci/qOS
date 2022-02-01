#include "pageTableManager.hpp"

#define F_PRESENT (1 << 0)
#define F_RW (1 << 1)

void setAddress(uint64_t* entry, uint64_t address) {
	address &= 0xFFFFFFFFFF;
	*entry &= 0xFFF0000000000FFF;
	*entry |= address << 12;
}
uint64_t getAddress(uint64_t entry) {
	return (entry & 0x000FFFFFFFFFF000) >> 12;
}
void PageTableManager::mapMemory(uint64_t virtualAddress, uint64_t physicalAddress) {
	virtualAddress >>= 12;
	uint64_t PT_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PD_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PDP_i = virtualAddress & 0x1FF;
	virtualAddress >>= 9;
	uint64_t PML4_i = virtualAddress & 0x1FF;

	uint64_t* PDP;
	if (PML4[PML4_i] & F_PRESENT) {
		PDP = reinterpret_cast<uint64_t*>(getAddress(PML4[PML4_i]));
	}
	else {
		PDP = static_cast<uint64_t*>(globalAllocator.allocatePage());
		memset(PDP, 0, 0x1000);
		setAddress(PML4 + PML4_i, reinterpret_cast<uint64_t>(PDP));
		PML4[PML4_i] |= F_PRESENT;
		PML4[PML4_i] |= F_RW;
	}

	uint64_t* PD;
	if (PDP[PDP_i] & F_PRESENT) {
		PD = reinterpret_cast<uint64_t*>(getAddress(PDP[PDP_i]));
	}
	else {
		PD = static_cast<uint64_t*>(globalAllocator.allocatePage());
		memset(PD, 0, 0x1000);
		setAddress(PDP + PDP_i, reinterpret_cast<uint64_t>(PD));
		PDP[PDP_i] |= F_PRESENT;
		PDP[PDP_i] |= F_RW;
	}

	uint64_t* PT;
	if (PD[PD_i] & F_PRESENT) {
		PT = reinterpret_cast<uint64_t*>(getAddress(PD[PD_i]));
	}
	else {
		PT = static_cast<uint64_t*>(globalAllocator.allocatePage());
		memset(PT, 0, 0x1000);
		setAddress(PD + PD_i, reinterpret_cast<uint64_t>(PT));
		PD[PD_i] |= F_PRESENT;
		PD[PD_i] |= F_RW;
	}

	setAddress(PT + PT_i, physicalAddress);
	PT[PT_i] |= F_PRESENT;
	PT[PT_i] |= F_RW;
}

void PageTableManager::refresh() {
	asm("mov %0, %%cr3" : : "r"(PML4));
}
