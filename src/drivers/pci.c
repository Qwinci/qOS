#include "pci.h"
#include "acpi/acpi.h"
#include "stdio.h"
#include <stddef.h>
#include "paging/memory.h"
#include "pci_device_db.h"
#include "intel_82574.h"

typedef struct {
	SDTHeader header;
	uint64_t reserved;
} __attribute__((packed)) MCFG;

static_assert(sizeof(MCFG) == 44);

typedef struct {
	uint64_t base;
	uint16_t segment_group;
	uint8_t start_bus;
	uint8_t end_bus;
	uint32_t reserved;
} PCIEntry;

static_assert(sizeof(PCIEntry) == 16);

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

void enumerate_function(uintptr_t base, uint8_t function) {
	uintptr_t address = base + ((uintptr_t) function << 12);

	pmap(address - 0xFFFF800000000000, address, PAGEFLAG_PRESENT | PAGEFLAG_RW);
	prefresh(address);

	PCIDeviceHeader* header = (PCIDeviceHeader*) address;

	if (header->device_id == 0 || header->vendor_id == 0xFFFF) return;

	const char* vendor = get_vendor(header->vendor_id);
	const char* device = get_device(header->device_id, header->vendor_id);

	printf("[%h:%h] ", header->vendor_id, header->device_id);

	if (vendor) printf("%s ", vendor);
	else printf("%h ", header->vendor_id);

	if (device) printf("%s ", device);
	else printf("%h ", header->device_id);

	printf("class: %u8, subclass: %u8\n", header->class, header->subclass);

	PCIDeviceHeader0* header0 = (PCIDeviceHeader0*) header;

	if (header->vendor_id == 0x8086 && header->device_id == 0x10D3) {
		// intel 82574L gigabit network connection
		header->command |= PCI_CMD_IO_SPACE | PCI_CMD_MEM_SPACE | PCI_CMD_BUS_MASTER;
		initialize_intel_82574(header0);
	}
}

void enumerate_device(uintptr_t base, uint8_t slot) {
	uintptr_t address = base + ((uintptr_t) slot << 15);

	pmap(address - 0xFFFF800000000000, address, PAGEFLAG_PRESENT | PAGEFLAG_RW);
	prefresh(address);

	PCIDeviceHeader* header = (PCIDeviceHeader*) address;

	if (header->device_id == 0 || header->vendor_id == 0xFFFF) return;

	for (uint8_t function = 0; function < 8; ++function) {
		enumerate_function(address, function);
	}
}

void enumerate_bus(uintptr_t base, uint8_t bus) {
	uintptr_t address = base + ((uintptr_t) bus << 20);

	pmap(address - 0xFFFF800000000000, address, PAGEFLAG_PRESENT | PAGEFLAG_RW);
	prefresh(address);

	PCIDeviceHeader* header = (PCIDeviceHeader*) address;

	if (header->device_id == 0 || header->vendor_id == 0xFFFF) return;

	for (uint8_t slot = 0; slot < 32; ++slot) {
		enumerate_device(address, slot);
	}
}

void initialize_pci(void* rsdp) {
	MCFG* mcfg = (MCFG*) locate_acpi_table(rsdp, "MCFG");
	if (!mcfg) {
		printf("failed to locate MCFG table\n");
		return;
	}

	for (size_t i = 0; i < mcfg->header.length - sizeof(MCFG); i += sizeof(PCIEntry)) {
		PCIEntry* entry = (PCIEntry*) ((uintptr_t) mcfg + sizeof(MCFG) + i);
		for (uint8_t bus = entry->start_bus; bus < entry->end_bus; ++bus) {
			enumerate_bus(entry->base + 0xFFFF800000000000, bus);
		}
	}
}