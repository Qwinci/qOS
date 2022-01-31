#pragma once
#include <cstdint>

struct FrameBuffer {
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uintptr_t address;
};
enum class MemoryType {
	Usable,
	FrameBuffer,
	Kernel,
	Unknown
};
struct MemoryMapEntry {
	uint64_t size;
	uint64_t address;
	MemoryType type;
};
struct MemoryMap {
	uint64_t count;
	MemoryMapEntry* entries;
};

struct BootInfo {
	FrameBuffer frameBuffer;
	MemoryMap memoryMap;
	void* rsdp;
};