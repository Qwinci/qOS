#pragma once
#include <cstdint>
#include "pageFrameAllocator.hpp"
#include "../utils/memory.hpp"

class PageTableManager {
public:
	PageTableManager() : PML4{static_cast<uint64_t*>(globalAllocator.allocatePage())} {
		memset(PML4, 0, 0x1000);
	}

	void mapMemory(uint64_t virtualAddress, uint64_t physicalAddress);
	void refresh();

private:
	uint64_t* PML4;
};