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
		XSDT* xsdt = (XSDT*) to_virt(rsdp_descriptor->xsdt_address);
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
		RSDT* rsdt = (RSDT*) to_virt(rsdp_descriptor->rsdt_address);
		size_t entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;
		for (size_t i = 0; i < entries; ++i) {
			SDTHeader* header = (SDTHeader*) to_virt(rsdt->pointer[i]);
			if (strncmp((const char*) header, signature, 4) == 0) return header;
		}
	}
	else {
		XSDT* xsdt = (XSDT*) to_virt(rsdp_descriptor->xsdt_address);
		size_t entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;
		for (size_t i = 0; i < entries; ++i) {
			SDTHeader* header = (SDTHeader*) to_virt(xsdt->pointer[i]);
			pmap(xsdt->pointer[i], to_virt(xsdt->pointer[i]), PAGEFLAG_PRESENT | PAGEFLAG_RW);
			prefresh(to_virt(xsdt->pointer[i]));
			if (strncmp((const char*) header, signature, 4) == 0) return header;
		}
	}
	return NULL;
}

void* locate_nth_acpi_table(void* rsdp, const char* signature, size_t index) {
	size_t found_index = 0;
	RSDPDescriptor* rsdp_descriptor = (RSDPDescriptor*) rsdp;

	bool bad_xsdt = false;
	if (rsdp_descriptor->revision != 0) {
		XSDT* xsdt = (XSDT*) to_virt(rsdp_descriptor->xsdt_address);
		size_t entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;
		for (size_t i = 0; i < entries; ++i) {
			if (xsdt->pointer[i] == 0) {
				bad_xsdt = true;
				break;
			}
		}
	}

	if (rsdp_descriptor->revision == 0 || bad_xsdt) {
		RSDT* rsdt = (RSDT*) to_virt(rsdp_descriptor->rsdt_address);
		size_t entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;
		for (size_t i = 0; i < entries; ++i) {
			SDTHeader* header = (SDTHeader*) to_virt(rsdt->pointer[i]);
			if (strncmp((const char*) header, signature, 4) == 0) {
				if (found_index == index) return header;
				else ++found_index;
			}
		}
	}
	else {
		XSDT* xsdt = (XSDT*) to_virt(rsdp_descriptor->xsdt_address);
		size_t entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;
		for (size_t i = 0; i < entries; ++i) {
			SDTHeader* header = (SDTHeader*) to_virt(xsdt->pointer[i]);
			pmap(xsdt->pointer[i], to_virt(xsdt->pointer[i]), PAGEFLAG_PRESENT | PAGEFLAG_RW);
			prefresh(to_virt(xsdt->pointer[i]));
			if (strncmp((const char*) header, signature, 4) == 0) {
				if (found_index == index) return header;
				else ++found_index;
			}
		}
	}
	return NULL;
}