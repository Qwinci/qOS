#pragma once
#include <cstdint>

struct RSDP {
	char signature[8];
	uint8_t checksum;
	char oemID[6];
	uint8_t revision;
	uint32_t rsdtAddress;
	// 2.0
	uint32_t length;
	uint64_t xsdtAddress;
	uint8_t extendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed));

struct SDTHeader {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oemID[6];
	char oemTableID[8];
	uint32_t oemRevision;
	uint32_t creatorID;
	uint32_t creatorRevision;
};

struct RSDT {
	SDTHeader header;
	uint32_t addresses[];
};
struct XSDT {
	SDTHeader header;
	uint64_t addresses[];
};

void* findTable(RSDP* rsdp, const char* signature);