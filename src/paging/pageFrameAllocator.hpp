#pragma once
#include "../bootInfo.hpp"
#include "bitmap.hpp"

class PageFrameAllocator {
public:
	explicit PageFrameAllocator(MemoryMap& memoryMap);
	PageFrameAllocator() = default;

	void* allocatePage();
	void freePage(void* address);
	void lockPage(uintptr_t address);
	void lockPages(uintptr_t address, size_t count);
	[[nodiscard]] size_t getTotalMemory() const {return totalMemory;}
private:
	Bitmap bitmap{};
	size_t bitmapSize{};
	size_t totalMemory{0};
	size_t usedMemory{0};
	size_t reservedMemory{0};
};

extern PageFrameAllocator globalAllocator;