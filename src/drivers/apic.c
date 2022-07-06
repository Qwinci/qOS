#include "apic.h"
#include "acpi/acpi.h"
#include "stdio.h"
#include <stddef.h>
#include "paging/memory.h"

#define FLAG_ACTIVE_LOW (1 << 1)
#define FLAG_LEVEL_TRIGGERED (1 << 3)

void write_io_apic_register(uintptr_t base, uint8_t reg, uint32_t value) {
	*(volatile uint32_t*) (base) = reg; // IOREGSEL
	*(volatile uint32_t*) (base + 0x10) = value;
}

uint32_t read_io_apic_register(uintptr_t base, uint8_t reg) {
	*(volatile uint32_t*) (base) = reg; // IOREGSEL
	return *(volatile uint32_t*) (base + 0x10);
}

void initialize_apic(void* rsdp) {
	__asm__("mov al, 0xFF; out 0xA1, al; out 0x21, al;" : : : "al");
	void* madt = locate_acpi_table(rsdp, "APIC");

	if (!madt) {
		printf("failed to find madt table. falling back to pic\n");
		return;
	}

	SDTHeader* header = (SDTHeader*) madt;
	madt += sizeof(SDTHeader);

	uint64_t local_apic_address = *(uint32_t*) madt + 0xFFFF800000000000;
	madt += 4;
	uint32_t flags = *(uint32_t*) madt;
	madt += 4;

	for (size_t i = (uintptr_t) header; i < (uintptr_t) header + header->length; i += 0x1000) {
		pmap((i - 0xFFFF800000000000) / 0x1000 * 0x1000, i / 0x1000 * 0x1000, PAGEFLAG_PRESENT | PAGEFLAG_RW);
		prefresh(i);
	}

	uintptr_t io_apic_base = 0;

	for (size_t i = 0; i < header->length - sizeof(SDTHeader) - 8;) {
		uint8_t type = *(uint8_t*) (madt + i);
		uint8_t length = *(uint8_t*) (madt + i + 1);
		i += 2;
		printf("type: %d, size: %u64\n", type, length);
		switch (type) {
			case 0: // processor local apic
			{
				uint8_t acpi_processor_id = *(uint8_t*) (madt + i);
				++i;
				uint8_t apic_id = *(uint8_t*) (madt + i);
				++i;
				uint32_t flags2 = *(uint32_t*) (madt + i);
				i += 4;
				break;
			}
			case 1: // io apic
			{
				uint8_t io_apic_id = *(uint8_t*) (madt + i);
				i += 2; // reserved
				uint32_t io_apic_address = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t global_system_interrupt_base = *(uint32_t*) (madt + i);
				i += 4;
				io_apic_base = io_apic_address + 0xFFFF800000000000;
				break;
			}
			case 2: // io apic interrupt source override
			{
				uint8_t bus_source = *(uint8_t*) (madt + i);
				++i;
				uint8_t irq_source = *(uint8_t*) (madt + i);
				++i;
				uint32_t global_system_interrupt = *(uint32_t*) (madt + i);
				i += 4;
				uint16_t flags2 = *(uint16_t*) (madt + i);
				i += 2;
				if (irq_source == 1) printf("%s:%d: fixme\n", __FILE_NAME__, __LINE__);
				break;
			}
			case 3: // io apic non maskable interrupt source
			{
				uint8_t nmi_source = *(uint8_t*) (madt + i);
				i += 2; // reserved
				uint16_t flags2 = *(uint16_t*) (madt + i);
				i += 2;
				uint32_t global_system_interrupt = *(uint32_t*) (madt + i);
				i += 4;
				break;
			}
			case 4: // local apic non maskable interrupts
			{
				uint8_t acpi_processor_id = *(uint8_t*) (madt + i);
				++i;
				uint16_t flags2 = *(uint16_t*) (madt + i);
				i += 2;
				uint8_t lint = *(uint8_t*) (madt + i);
				++i;
				break;
			}
			case 5: // local apic address override
			{
				// reserved
				i += 2;
				uint64_t address = *(uint64_t*) (madt + i);
				i += 8;
				local_apic_address = address + 0xffff800000000000;
				break;
			}
			case 9: // processor local x2apic
			{
				// reserved
				i += 2;
				uint32_t processor_local_x2apic_id = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t flags2 = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t acpi_id = *(uint32_t*) (madt + i);
				i += 4;
				break;
			}
		}
	}

	uint8_t bsp_id;
	__asm__ __volatile__("mov eax, 1; cpuid; shr ebx, 24" : "=b"(bsp_id));

	uint64_t entry = 0x20; // vector
	entry |= 0 << 8; // fixed mode
	entry |= 0 << 11; // physical destination
	entry |= 0 << 13; // active high
	entry |= 0 << 15; // edge
	entry &= ~(1 << 16); // mask
	entry |= (uint64_t) bsp_id << 56; // destination
	uint32_t entry_low = entry & 0xFFFFFFFF;
	uint32_t entry_high = (entry >> 32) & 0xFFFFFFFF;

	write_io_apic_register(io_apic_base, 0x12, entry_low);
	write_io_apic_register(io_apic_base, 0x13, entry_high);
	*(volatile uint32_t*) (local_apic_address + 0xF0) = 0xFF;
}