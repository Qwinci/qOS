#pragma once
#include <cstdint>

struct MADT {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oemID[6];
	char oemTableID[8];
	uint32_t oemRevision;
	uint32_t creatorID;
	uint32_t creatorRevision;

	uint32_t localAPICAddress;
	uint32_t flags;
} __attribute__((packed));

void initializeAPIC(MADT* madt);