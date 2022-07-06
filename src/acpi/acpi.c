#include "acpi.h"
#include <stdint.h>
#include <stddef.h>
#include "string.h"

typedef struct {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t reserved[3];
} __attribute__((packed)) RSDPDescriptor;

typedef struct {
	SDTHeader header;
	uint32_t pointer[];
} RSDT;

typedef struct {
	SDTHeader header;
	uint64_t pointer[];
} XSDT;

void* locate_acpi_table(void* rsdp, const char* signature) {
	RSDPDescriptor* rsdp_descriptor = (RSDPDescriptor*) rsdp;
	if (rsdp_descriptor->revision == 0) {
		RSDT* rsdt = (RSDT*) (rsdp_descriptor->rsdt_address + 0xFFFF800000000000);
		size_t entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;
		for (size_t i = 0; i < entries; ++i) {
			SDTHeader* header = (SDTHeader*) (rsdt->pointer[i] + 0xFFFF800000000000);
			if (strncmp((const char*) header, signature, 4) == 0) return header;
		}
	}
	else {
		XSDT* xsdt = (XSDT*) (rsdp_descriptor->xsdt_address + 0xFFFF800000000000);
		size_t entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;
		for (size_t i = 0; i < entries; ++i) {
			SDTHeader* header = (SDTHeader*) (xsdt->pointer[i] + 0xFFFF800000000000);
			if (strncmp((const char*) header, signature, 4) == 0) return header;
		}
	}
	return NULL;
}