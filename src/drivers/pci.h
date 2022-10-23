#pragma once
#include "assert.h"
#include <stdint.h>

void initialize_pci(void* rsdp);

#define PCI_CAP_MSI 0x5
#define PCI_CAP_MSI_X 0x11

typedef struct {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t command;
	uint16_t status;
	uint8_t revision;
	uint8_t prog_if;
	uint8_t subclass;
	uint8_t class;
	uint8_t cache_line_size;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t bist;
} PCIDeviceHeader;

static_assert(sizeof(PCIDeviceHeader) == 16);

typedef struct {
	PCIDeviceHeader header;
	uint32_t BAR0;
	uint32_t BAR1;
	uint32_t BAR2;
	uint32_t BAR3;
	uint32_t BAR4;
	uint32_t BAR5;
	uint32_t cardbus_cis_pointer;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_id;
	uint32_t expansion_rom_base;
	uint8_t capabilities_pointer;
	uint8_t reserved1;
	uint16_t reserved2;
	uint32_t reserved3;
	uint8_t interrupt_line;
	uint8_t interrupt_pin;
	uint8_t min_grant;
	uint8_t max_latency;
} PCIDeviceHeader0;

static_assert(sizeof(PCIDeviceHeader0) == sizeof(PCIDeviceHeader) + 48);

typedef struct {
	uint8_t id;
	uint8_t next;
	uint16_t msg_control;
	uint64_t msg_addr;
	uint16_t msg_data;
	uint16_t reserved;
	uint64_t mask;
	uint64_t pending;
} PciMsiCapability;

#define MSI_CTRL_ENABLE (1 << 0)
#define MSI_CTRL_MM_1_CAPABLE (0b000 << 1)
#define MSI_CTRL_MM_2_CAPABLE (0b001 << 1)
#define MSI_CTRL_MM_4_CAPABLE (0b010 << 1)
#define MSI_CTRL_MM_8_CAPABLE (0b011 << 1)
#define MSI_CTRL_MM_16_CAPABLE (0b100 << 1)
#define MSI_CTRL_MM_32_CAPABLE (0b101 << 1)
#define MSI_CTRL_MM_CAPABLE (0b111 << 1)
#define MSI_CTRL_MM_1_ENABLE (0b000 << 4)
#define MSI_CTRL_MM_2_ENABLE (0b001 << 4)
#define MSI_CTRL_MM_4_ENABLE (0b010 << 4)
#define MSI_CTRL_MM_8_ENABLE (0b011 << 4)
#define MSI_CTRL_MM_16_ENABLE (0b100 << 4)
#define MSI_CTRL_MM_32_ENABLE (0b101 << 4)
#define MSI_CTRL_64_BIT (1 << 7)
#define MSI_CTRL_PER_VECTOR_MASKING (1 << 8)