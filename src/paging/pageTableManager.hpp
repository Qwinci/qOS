#pragma once
#include <cstdint>
#include "pageFrameAllocator.hpp"
#include "../utils/memory.hpp"

enum class PageFlag {
	None = 0,
	Present = 1 << 0,
	RW = 1 << 1,
	User = 1 << 2,
	WriteThrough = 1 << 3,
	CacheDisable = 1 << 4,
	HugePage = 1 << 7,
	Global = 1 << 8,
};
inline static PageFlag operator|(PageFlag a, PageFlag b) {
	return static_cast<PageFlag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline static uint64_t operator|(uint64_t a, PageFlag b) {
	return a | static_cast<uint8_t>(b);
}
inline static uint8_t operator&(uint64_t a, PageFlag b) {
	return a & static_cast<uint8_t>(b);
}

class PageTableManager {
public:
	PageTableManager() : PML4{static_cast<uint64_t*>(globalAllocator.allocatePage())} {
		memset(PML4, 0, 0x1000);
	}

	void mapMemory(uint64_t virtualAddress, uint64_t physicalAddress,
				   PageFlag flags = PageFlag::None);
	void refresh();
	void refreshPage(uint64_t address);
	void changeOffset();

private:
	uint64_t* PML4;
	uint64_t offset{0};
};

extern PageTableManager globalPageTableManager;