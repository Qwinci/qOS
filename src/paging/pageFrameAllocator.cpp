#include "pageFrameAllocator.hpp"
#include "bitmap.hpp"
#include "../console/renderer.hpp"
#include <cstddef>
#include "pageTableManager.hpp"

PageFrameAllocator::PageFrameAllocator(MemoryMap &memoryMap) {
	size_t bestSize = 0;
	uintptr_t bestEntryAddress{};

	for (int i = 0; i < memoryMap.size; ++i) {
		if (memoryMap.entries[i].type == MemoryType::Usable) {
			if (memoryMap.entries[i].size > bestSize) {
				bestSize = memoryMap.entries[i].size;
				bestEntryAddress = memoryMap.entries[i].address;
			}
		}
		totalMemory += memoryMap.entries[i].size;
	}

	bitmapSize = totalMemory / 0x1000 + 1;
	bitmap = Bitmap {bestEntryAddress, totalMemory / 0x1000 + 1};
	bitmapAddress = bestEntryAddress;

	lockPages(bestEntryAddress, bitmapSize / 64);

	lockPages(0, 0x100);

	for (int i = 0; i < memoryMap.size; ++i) {
		if (memoryMap.entries[i].type != MemoryType::Usable) {
			lockPages(memoryMap.entries[i].address, memoryMap.entries[i].size / 0x1000 + 1);
		}
	}
}

void *PageFrameAllocator::allocatePage() {
	for (size_t i = 0; i < bitmapSize; ++i) {
		if (!bitmap[i]) {
			bitmap[i] = true;
			return reinterpret_cast<void*>(i * 0x1000 + offset);
		}
	}
	return nullptr;
}

void *PageFrameAllocator::allocatePages(size_t count) {
	size_t foundCount = 0;
	size_t start = 0;
	for (size_t i = 0; i < bitmapSize; ++i) {
		if (!bitmap[i]) {
			start = i;
			foundCount += 1;
		}
		else {
			start = 0;
			foundCount = 0;
		}
	}

	if (start != 0 && foundCount == count) {
		for (size_t i = 0; i < count; ++i) {
			bitmap[start + i] = true;
		}
		return reinterpret_cast<void*>(start * 0x1000 + offset);
	}
	else {
		return nullptr;
	}
}

void PageFrameAllocator::freePage(void *address) {
	auto addr = reinterpret_cast<uintptr_t>(address) - offset;
	bitmap[addr / 0x1000] = false;
}

void PageFrameAllocator::freePages(void *address, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		freePage(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(address) + i * 0x1000));
	}
}

void PageFrameAllocator::lockPage(uintptr_t address) {
	bitmap[address / 0x1000] = true;
	reservedMemory += 0x1000;
}

void PageFrameAllocator::lockPages(uintptr_t address, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		lockPage(address + i * 0x1000);
	}
}

void PageFrameAllocator::changeMapping() {
	bitmap.changeBufferAddress(0xffff800000000000 + bitmapAddress);
	offset = 0xffff800000000000;
}
