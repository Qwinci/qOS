#pragma once
#include "../bootInfo.hpp"
#include "bitmap.hpp"

class PageFrameAllocator {
public:
	explicit PageFrameAllocator(MemoryMap& memoryMap);
	PageFrameAllocator() = default;

	void* allocatePage();
	void* allocatePages(size_t count);
	void freePage(void* address);
	void freePages(void* address, size_t count);
	void lockPage(uintptr_t address);
	void lockPages(uintptr_t address, size_t count);
	[[nodiscard]] size_t getTotalMemory() const {return totalMemory;}
	void changeMapping();
private:
	Bitmap bitmap{};
	uintptr_t bitmapAddress{};
	uintptr_t offset{};
	size_t bitmapSize{};
	size_t totalMemory{0};
	size_t reservedMemory{0};
};

extern PageFrameAllocator globalAllocator;