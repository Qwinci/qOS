#pragma once
#include <cstdint>

struct FrameBuffer {
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint16_t pitch;
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
	uint64_t size;
	MemoryMapEntry entries[75];
};

struct BootInfo {
	FrameBuffer frameBuffer;
	uintptr_t fontStart;
	uint64_t fontSize;
	MemoryMap memoryMap;
	uintptr_t kernelPhysicalAddress;
	uintptr_t kernelVirtualAddress;
	void* rsdp;
};