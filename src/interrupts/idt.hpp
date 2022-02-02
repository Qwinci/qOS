#pragma once
#include <cstdint>

struct IDTR {
	uint16_t size;
	uint64_t offset;
} __attribute__((packed));

struct IDTDescriptor {
	uint16_t offset0;
	uint16_t selector;
	uint8_t ist;
	uint8_t typeAttributes;
	uint16_t offset1;
	uint32_t offset2;
	uint32_t reserved;
} __attribute__((packed));

void initializeInterrupts();