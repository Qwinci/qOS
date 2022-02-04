#pragma once
#include <cstdint>

struct MCFG {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oemID[6];
	char oemTableID[8];
	uint32_t oemRevision;
	uint32_t creatorID;
	uint32_t creatorRevision;
	uint64_t reserved;
} __attribute__((packed));

struct DeviceConfig {
	uint64_t baseAddress;
	uint16_t segment;
	uint8_t startBus;
	uint8_t endBus;
	uint32_t reserved;
} __attribute__((packed));

struct PCIHeader {
	uint16_t vendorID;
	uint16_t deviceID;
	uint16_t command;
	uint16_t status;
	uint8_t revisionID;
	uint8_t progIF;
	uint8_t Subclass;
	uint8_t Class;
	uint8_t cacheLineSize;
	uint8_t latencyTimer;
	uint8_t type;
	uint8_t BIST;
} __attribute__((packed));

struct PCIHeader0 {
	PCIHeader header;
	uint32_t BAR0;
	uint32_t BAR1;
	uint32_t BAR2;
	uint32_t BAR3;
	uint32_t BAR4;
	uint32_t BAR5;
	uint32_t cardbusCISPointer;
	uint16_t subsystemVendorID;
	uint16_t subsystemID;
	uint32_t expansionROMAddress;
	uint16_t capabilities;
	uint16_t reserved0;
	uint32_t reserved1;
	uint8_t interruptLine;
	uint8_t interruptPIN;
	uint8_t minGrant;
	uint8_t maxLatency;
} __attribute__((packed));

void enumeratePCI(MCFG* mcfg);