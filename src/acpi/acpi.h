#pragma once
#include <stdint.h>
#include "assert.h"

typedef struct {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} SDTHeader;

static_assert(sizeof(SDTHeader) == 36);

void* locate_acpi_table(void* rsdp, const char* signature);