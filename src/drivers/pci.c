#include "pci.h"
#include "acpi/acpi.h"
#include "stdio.h"
#include <stddef.h>
#include "paging/memory.h"
#include "pci_device_db.h"
#include "intel_82574.h"
#include "usb.h"

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

static uint16_t current_segment;
static uint8_t current_bus;
static uint8_t current_device;

void enumerate_function(uintptr_t base, uint8_t function) {
	uintptr_t address = base + ((uintptr_t) function << 12);

	pmap(
			to_phys(address),
			address,
			PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_CACHE_DISABLE);
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

	PciDeviceInfo info = {
			.segment = current_segment,
			.bus = current_bus,
			.device = current_device,
			.function = function
			};

	/*if (header->vendor_id == 0x8086 && header->device_id == 0x10D3) {
		// intel 82574L gigabit network connection
		header->command |= PCI_CMD_IO_SPACE | PCI_CMD_MEM_SPACE | PCI_CMD_BUS_MASTER;
		initialize_intel_82574(header0);
	}*/
	// usb controller

	if (header->class == 0xC && header->subclass == 0x3) {
		// 0x0 = UHCI, 0x10 = OHCI, 0x20 = EHCI, 0x30 = XHCI
		if (header->prog_if == 0x0) {
			initialize_usb_uhci(header0, info);
		}
		else if (header->prog_if == 0x10) {

		}
		else if (header->prog_if == 0x20) {

		}
		else if (header->prog_if >= 0x30 && header->prog_if < 0x40) {
			initialize_usb_xhci(header0);
		}
	}
}

void enumerate_device(uintptr_t base, uint8_t slot) {
	current_device = slot;
	uintptr_t address = base + ((uintptr_t) slot << 15);

	pmap(
			to_phys(address),
			address,
			PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_CACHE_DISABLE);
	prefresh(address);

	PCIDeviceHeader* header = (PCIDeviceHeader*) address;

	if (header->device_id == 0 || header->vendor_id == 0xFFFF) return;

	uint8_t max_function = 8;
	if ((header->header_type & 1 << 7) == 0) {
		max_function = 1;
	}

	// usb controller
	if (header->class == 0xC && header->subclass == 0x3) {
		for (int8_t function = (int8_t) (max_function - 1); function >= 0; --function) {
			enumerate_function(address, function);
		}
		return;
	}

	for (uint8_t function = 0; function < max_function; ++function) {
		enumerate_function(address, function);
	}
}

void enumerate_bus(uintptr_t base, uint8_t bus) {
	current_bus = bus;
	uintptr_t address = base + ((uintptr_t) bus << 20);

	pmap(
			to_phys(address),
			address,
			PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_CACHE_DISABLE);
	prefresh(address);

	PCIDeviceHeader* header = (PCIDeviceHeader*) address;

	if (header->device_id == 0 || header->vendor_id == 0xFFFF) return;

	for (uint8_t slot = 0; slot < 32; ++slot) {
		enumerate_device(address, slot);
	}
}

static MCFG* mcfg;

bool initialize_pci(void* rsdp) {
	mcfg = (MCFG*) locate_acpi_table(rsdp, "MCFG");
	if (!mcfg) {
		printf("failed to locate MCFG table\n");
		return false;
	}
	return true;
}

void enumerate_pci() {
	for (size_t i = 0; i < mcfg->header.length - sizeof(MCFG); i += sizeof(PCIEntry)) {
		PCIEntry* entry = (PCIEntry*) ((uintptr_t) mcfg + sizeof(MCFG) + i);
		current_segment = entry->segment_group;
		for (uint8_t bus = entry->start_bus; bus < entry->end_bus; ++bus) {
			enumerate_bus(to_virt(entry->base), bus);
		}
	}
}

PCIDeviceHeader* pci_get_dev_header(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun) {
	for (size_t i = 0; i < mcfg->header.length - sizeof(MCFG); i += sizeof(PCIEntry)) {
		PCIEntry* entry = (PCIEntry*) ((uintptr_t) mcfg + sizeof(MCFG) + i);
		if (entry->segment_group == seg) {
			PCIDeviceHeader* header = (PCIDeviceHeader*)
					(to_virt(entry->base) +
							((uintptr_t) bus << 20) +
							((uintptr_t) slot << 15) +
							((uintptr_t) fun << 12));
			return header;
		}
	}
	return NULL;
}

PciMsiCapability* pci_get_msi_cap0(PCIDeviceHeader0* header) {
	if ((header->header.status & 1 << 4) == 0) return NULL;
	uint8_t offset = header->capabilities_pointer & 0b11111100;
	while (true) {
		uint8_t id = *(uint8_t*) ((uintptr_t) header + offset);
		if (id == PCI_CAP_MSI) {
			PciMsiCapability* msi = (PciMsiCapability*) ((uintptr_t) header + offset);
			return msi;
		}
		uint8_t next = *(uint8_t*) ((uintptr_t) header + offset + 1);
		offset = next;
		if (!next) break;
	}
	return NULL;
}

PciMsiXCapability* pci_get_msi_x_cap0(PCIDeviceHeader0* header) {
	if ((header->header.status & 1 << 4) == 0) return NULL;
	uint8_t offset = header->capabilities_pointer & 0b11111100;
	while (true) {
		uint8_t id = *(uint8_t*) ((uintptr_t) header + offset);
		if (id == PCI_CAP_MSI_X) {
			PciMsiXCapability* msi_x = (PciMsiXCapability*) ((uintptr_t) header + offset);
			return msi_x;
		}
		uint8_t next = *(uint8_t*) ((uintptr_t) header + offset + 1);
		offset = next;
		if (!next) break;
	}
	return NULL;
}

PciPmCapability* pci_get_pm_cap0(PCIDeviceHeader0* header) {
	if ((header->header.status & 1 << 4) == 0) return NULL;
	uint8_t offset = header->capabilities_pointer & 0b11111100;
	while (true) {
		uint8_t id = *(uint8_t*) ((uintptr_t) header + offset);
		if (id == PCI_CAP_PM) {
			PciPmCapability* pm = (PciPmCapability*) ((uintptr_t) header + offset);
			return pm;
		}
		uint8_t next = *(uint8_t*) ((uintptr_t) header + offset + 1);
		offset = next;
		if (!next) break;
	}
	return NULL;
}