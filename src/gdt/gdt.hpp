#pragma once
#include <cstdint>

struct GDTDescriptor {
	uint16_t size;
	uint64_t offset;
} __attribute__((packed));

struct GDTEntry {
	uint16_t limit0;
	uint16_t base0;
	uint8_t base1;
	uint8_t accessByte;
	uint8_t limit1AndFlags;
	uint8_t base2;
} __attribute__((packed));

struct GDT {
	GDTEntry kernelNull;
	GDTEntry kernelCode;
	GDTEntry kernelData;
	GDTEntry userNull;
	GDTEntry userCode;
	GDTEntry userData;
} __attribute__((packed));

extern GDT gdt;

extern "C" void loadGDT(GDT* gdt);