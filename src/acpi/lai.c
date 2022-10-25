#include "stdio.h"
#include <lai/core.h>
#include "paging/malloc.h"
#include "paging/memory.h"
#include "acpi.h"
#include "utils/io.h"
#include "drivers/pci.h"
#include "timers/timers.h"
#include "lai/helpers/sci.h"

static void* rsdp_ptr;
static uint8_t acpi_revision = 0;

void lai_host_init(void* rsdp) {
	rsdp_ptr = rsdp;
	acpi_rsdp_t* rsdp_table = (acpi_rsdp_t*) rsdp;
	lai_set_acpi_revision(rsdp_table->revision);
	lai_create_namespace();
	lai_enable_acpi(1);
	acpi_revision = rsdp_table->revision;
}

void laihost_log(int level, const char* msg) {
	if (level == LAI_DEBUG_LOG) printf("lai debug: %s\n", msg);
	else printf("lai warning: %s\n", msg);
}

__attribute__((noreturn)) void laihost_panic(const char* msg) {
	printf("lai error: %s\n", msg);
	while (1) __asm__ volatile("hlt");
}

void* laihost_malloc(size_t size) {
	return malloc(size);
}

void* laihost_realloc(void* ptr, size_t new_size, size_t old_size) {
	void* data = NULL;
	if (new_size > 0) {
		data = malloc(new_size);
		memcpy(data, ptr, old_size < new_size ? old_size : new_size);
	}

	if (ptr) free(ptr, old_size);
	return data;
}

void laihost_free(void* ptr, size_t size) {
	free(ptr, size);
}

void* laihost_map(size_t address, size_t count) {
	for (size_t i = 0; i < count; i += 0x1000) {
		pmap(address, address + 0xFFFF800000000000, PAGEFLAG_PRESENT | PAGEFLAG_RW);
	}
	return (void*) (address + 0xFFFF800000000000);
}

void laihost_unmap(void* ptr, size_t count) {
	for (size_t i = 0; i < count; i += 0x1000) {
		punmap((uintptr_t) ptr + i);
	}
}

typedef struct {
	uint8_t address_space;
	uint8_t bit_width;
	uint8_t bit_offset;
	uint8_t access_size;
	uint64_t address;
} GenericAddressStructure;

typedef struct {
	SDTHeader header;
	uint32_t firmware_ctrl;
	uint32_t dsdt;
	uint8_t reserved;

	uint8_t preferred_power_management_profile;
	uint16_t sci_interrupt;
	uint32_t smi_command_port;
	uint8_t acpi_enable;
	uint8_t acpi_disable;
	uint8_t s4bios_req;
	uint8_t p_state_control;
	uint32_t pm1a_event_block;
	uint32_t pm1b_event_block;
	uint32_t pm1a_control_block;
	uint32_t pm1b_control_block;
	uint32_t pm2_control_block;
	uint32_t pm_timer_block;
	uint32_t gpe0_block;
	uint32_t gpe1_block;
	uint8_t pm1_event_length;
	uint8_t pm1_control_length;
	uint8_t pm2_control_length;
	uint8_t pm_timer_length;
	uint8_t gpe0_length;
	uint8_t gpe1_length;
	uint8_t gpe1_base;
	uint8_t c_state_control;
	uint16_t worst_c2_latency;
	uint16_t worst_c3_latency;
	uint16_t flush_size;
	uint16_t flush_stride;
	uint8_t duty_offset;
	uint8_t duty_width;
	uint8_t day_alarm;
	uint8_t month_alarm;
	uint8_t century;

	uint16_t boot_architecture_flags;

	uint8_t reserved2;
	uint32_t flags;

	GenericAddressStructure reset_reg;

	uint8_t reset_value;
	uint8_t reserved3[3];

	uint64_t x_firmware_control;
	uint64_t x_dsdt;

	GenericAddressStructure x_pm1a_event_block;
	GenericAddressStructure x_pm1b_event_block;
	GenericAddressStructure x_pm1a_control_block;
	GenericAddressStructure x_pm1b_control_block;
	GenericAddressStructure x_pm2_control_block;
	GenericAddressStructure x_pm_timer_block;
	GenericAddressStructure x_gpe0_block;
	GenericAddressStructure x_gpe1_block;
} FADT;

void* laihost_scan(const char* sig, size_t index) {
	if (sig[0] == 'D' && sig[1] == 'S' && sig[2] == 'D' && sig[3] == 'T') {
		FADT* fadt = locate_acpi_table(rsdp_ptr, "FACP");
		if (acpi_revision != 0) {
			return (void*) (fadt->x_dsdt + 0xFFFF800000000000);
		}
		else return (void*) ((uintptr_t) fadt->dsdt + 0xFFFF800000000000);
	}
	return locate_nth_acpi_table(rsdp_ptr, sig, index);
}

void laihost_outb(uint16_t port, uint8_t value) {
	out1(port, value);
}

void laihost_outw(uint16_t port, uint16_t value) {
	out2(port, value);
}

void laihost_outd(uint16_t port, uint32_t value) {
	out4(port, value);
}

uint8_t laihost_inb(uint16_t port) {
	return in1(port);
}

uint16_t laihost_inw(uint16_t port) {
	return in2(port);
}

uint32_t laihost_ind(uint16_t port) {
	return in4(port);
}

uint8_t laihost_pci_readb(
		uint16_t seg,
		uint8_t bus,
		uint8_t slot,
		uint8_t fun,
		uint16_t offset) {
	PCIDeviceHeader* header = pci_get_dev_header(seg, bus, slot, fun);
	return *((volatile uint8_t*) header + offset);
}

uint16_t laihost_pci_readw(
		uint16_t seg,
		uint8_t bus,
		uint8_t slot,
		uint8_t fun,
		uint16_t offset) {
	PCIDeviceHeader* header = pci_get_dev_header(seg, bus, slot, fun);
	return *((volatile uint16_t*) header + offset);
}

uint32_t laihost_pci_readd(
		uint16_t seg,
		uint8_t bus,
		uint8_t slot,
		uint8_t fun,
		uint16_t offset) {
	PCIDeviceHeader* header = pci_get_dev_header(seg, bus, slot, fun);
	return *((volatile uint32_t*) header + offset);
}

void laihost_pci_writeb(
		uint16_t seg,
		uint8_t bus,
		uint8_t slot,
		uint8_t fun,
		uint16_t offset,
		uint8_t value) {
	PCIDeviceHeader* header = pci_get_dev_header(seg, bus, slot, fun);
	*((volatile uint8_t*) header + offset) = value;
}

void laihost_pci_writew(
		uint16_t seg,
		uint8_t bus,
		uint8_t slot,
		uint8_t fun,
		uint16_t offset,
		uint16_t value) {
	PCIDeviceHeader* header = pci_get_dev_header(seg, bus, slot, fun);
	*((volatile uint16_t*) header + offset) = value;
}

void laihost_pci_writed(
		uint16_t seg,
		uint8_t bus,
		uint8_t slot,
		uint8_t fun,
		uint16_t offset,
		uint32_t value) {
	PCIDeviceHeader* header = pci_get_dev_header(seg, bus, slot, fun);
	*((volatile uint32_t*) header + offset) = value;
}

void laihost_sleep(uint64_t ms) {
	udelay(ms * 1000);
}