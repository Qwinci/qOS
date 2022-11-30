#pragma once
#include "assert.h"
#include "paging/memory.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool initialize_pci(void* rsdp);
void enumerate_pci();

#define PCI_CAP_PM 0x1
#define PCI_CAP_MSI 0x5
#define PCI_CAP_MSI_X 0x11

typedef struct {
	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
} PciDeviceInfo;

typedef struct {
	uint16_t vendor_id;
	uint16_t device_id;
	volatile uint16_t command;
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
	volatile uint32_t BAR0;
	volatile uint32_t BAR1;
	volatile uint32_t BAR2;
	volatile uint32_t BAR3;
	volatile uint32_t BAR4;
	volatile uint32_t BAR5;
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

#define PCI_PM_MASK (0b11)
#define PCI_PM_D0 (0b00)
#define PCI_PM_D3 (0b11)
#define PCI_PM_NO_SOFT_RESET (1 << 3)

typedef struct {
	uint8_t id;
	uint8_t next;
	uint8_t version;
	uint8_t reserved1;
	uint32_t state;
} PciPmCapability;

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

typedef struct {
	uint8_t id;
	uint8_t next;
	struct {
		uint16_t table_size : 11;
		uint16_t reserved : 3;
		uint16_t func_mask : 1;
		uint16_t enable : 1;
	} msg_control;
	uint32_t bir : 3;
	uint32_t table_offset : 29;
	uint32_t pending_bit_bir : 3;
	uint32_t pending_bit_offset : 29;
} PciMsiXCapability;

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

#define PCI_CMD_IO_SPACE (1 << 0)
#define PCI_CMD_MEM_SPACE (1 << 1)
#define PCI_CMD_BUS_MASTER (1 << 2)
#define PCI_CMD_SPECIAL_CYCLES (1 << 3)
#define PCI_CMD_MEM_WRITE_AND_INVALIDATE (1 << 4)
#define PCI_CMD_VGA_PALETTE_SNOOP (1 << 5)
#define PCI_CMD_PARITY_ERROR_RESPONSE (1 << 6)
#define PCI_CMD_SERR_ENABLE (1 << 8)
#define PCI_CMD_FAST_BACK_BACK_ENABLE (1 << 9)
#define PCI_CMD_INTERRUPT_DISABLE (1 << 10)

PciMsiCapability* pci_get_msi_cap0(PCIDeviceHeader0* header);
PciMsiXCapability* pci_get_msi_x_cap0(PCIDeviceHeader0* header);
PciPmCapability* pci_get_pm_cap0(PCIDeviceHeader0* header);
PCIDeviceHeader* pci_get_dev_header(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun);

static inline uint32_t pci_get_bar_size(volatile uint32_t* bar) {
	uint32_t original_bar = *bar;
	*bar = 0xFFFFFFFF;
	uint32_t new_bar = *bar;
	uint32_t size = ~(new_bar & 0xFFFFFFF0) + 1;
	*bar = original_bar;
	return size;
}

static inline void* pci_map_bar32(volatile uint32_t* bar) {
	uint32_t size = pci_get_bar_size(bar);
	if (*bar & 1 << 3) {
		for (uint32_t i = 0; i < size; i += 0x1000) {
			pmap(
					(uintptr_t) *bar + i,
					to_virt((uintptr_t) *bar + i),
					PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_WRITE_THROUGH);
		}
	}
	else {
		for (uint32_t i = 0; i < size; i += 0x1000) {
			pmap(
					(uintptr_t) *bar + i,
					to_virt((uintptr_t) *bar + i),
					PAGEFLAG_PRESENT | PAGEFLAG_RW);
		}
	}
	return (void*) to_virt(*bar & 0xFFFFFFF0);
}

static inline void* pci_map_bar64(volatile uint32_t* bar0, volatile uint32_t* bar1) {
	uint32_t size = pci_get_bar_size(bar0);
	uintptr_t bar = (*bar0 & 0xFFFFFFF0) + ((uint64_t) (*bar1 & 0xFFFFFFFF) << 32);
	if (*bar0 & 1 << 3) {
		for (uint32_t i = 0; i < size; i += 0x1000) {
			pmap(
					(uintptr_t) bar + i,
					to_virt((uintptr_t) bar + i),
					PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_WRITE_THROUGH);
		}
	}
	else {
		for (uint32_t i = 0; i < size; i += 0x1000) {
			pmap(
					(uintptr_t) bar + i,
					to_virt((uintptr_t) bar + i),
					PAGEFLAG_PRESENT | PAGEFLAG_RW);
		}
	}
	return (void*) to_virt(bar);
}

static inline bool pci_is_bar64(uint32_t bar) {
	return (bar >> 1 & 0b11) == 2;
}

static inline uintptr_t pci_map_bar(volatile uint32_t* bar0, volatile uint32_t* bar1) {
	if (pci_is_bar64(*bar0)) return (uintptr_t) pci_map_bar64(bar0, bar1);
	else return (uintptr_t) pci_map_bar32(bar0);
}