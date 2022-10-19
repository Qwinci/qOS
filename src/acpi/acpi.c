#include "acpi.h"
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "stdio.h"
#include "paging/memory.h"
#include <stdbool.h>

typedef struct {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t extended_checksum;
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

	bool bad_xsdt = false;
	if (rsdp_descriptor->revision != 0) {
		XSDT* xsdt = (XSDT*) (rsdp_descriptor->xsdt_address + 0xFFFF800000000000);
		size_t entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;
		for (size_t i = 0; i < entries; ++i) {
			if (xsdt->pointer[i] == 0) {
				bad_xsdt = true;
				printf("[INFO][ACPI] xsdt contains invalid entries. using rsdt\n");
				break;
			}
		}
	}

	if (rsdp_descriptor->revision == 0 || bad_xsdt) {
		RSDT* rsdt = (RSDT*) (rsdp_descriptor->rsdt_address + 0xFFFF800000000000);
		size_t entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;
		for (size_t i = 0; i < entries; ++i) {
			SDTHeader* header = (SDTHeader*) ((uint64_t) rsdt->pointer[i] + 0xFFFF800000000000);
			if (strncmp((const char*) header, signature, 4) == 0) return header;
		}
	}
	else {
		XSDT* xsdt = (XSDT*) (rsdp_descriptor->xsdt_address + 0xFFFF800000000000);
		size_t entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;
		for (size_t i = 0; i < entries; ++i) {
			SDTHeader* header = (SDTHeader*) (xsdt->pointer[i]);
			header = (SDTHeader*) ((uintptr_t) header + 0xFFFF800000000000);
			pmap(xsdt->pointer[i], xsdt->pointer[i] + 0xFFFF800000000000, PAGEFLAG_PRESENT | PAGEFLAG_RW);
			prefresh(xsdt->pointer[i] + 0xFFFF800000000000);
			if (strncmp((const char*) header, signature, 4) == 0) return header;
		}
	}
	return NULL;
}