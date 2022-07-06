#include "apic.h"
#include "acpi/acpi.h"
#include "stdio.h"
#include <stddef.h>
#include "paging/memory.h"

#define FLAG_ACTIVE_LOW (1 << 1)
#define FLAG_LEVEL_TRIGGERED (1 << 3)

void initialize_apic(void* rsdp) {
	__asm__("mov al, 0xFF; out 0xA1, al; out 0x21, al;" : : : "al");
	void* madt = locate_acpi_table(rsdp, "APIC");
	printf("%h\n", madt);

	if (!madt) {
		printf("failed to find madt table. falling back to pic\n");
		return;
	}

	//SDTHeader* header = (SDTHeader*) madt;
	//printf("%h\n", (uintptr_t) header);
	//printf("%h\n", madt);
	//if (madt == (void*) header) printf("dd\n");
	printf("%h\n", madt);
	printf("%h\n", madt);
	/*madt += sizeof(SDTHeader);

	uint64_t local_apic_address = *(uint32_t*) madt;
	madt += 4;
	uint32_t flags = *(uint32_t*) madt;
	madt += 4;

	size_t condition = header->length - sizeof(SDTHeader) - 8;
	printf("condition: %u64\n", condition);
	printf("address: 0x%h\n", header);*/

	/*for (size_t i = 0; i < condition;) {
		printf("%u64\n", i);
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
				i += 2;
				uint32_t io_apic_address = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t global_system_interrupt_base = *(uint32_t*) (madt + i);
				i += 4;
				break;
			}
			case 2: // io apic interrupt source override
			{
				uint8_t bus_source = *(uint8_t*) (madt + i);
				++i;
				uint8_t irq_source = *(uint8_t*) (madt + 1);
				++i;
				uint32_t global_system_interrupt = *(uint32_t*) (madt + i);
				i += 4;
				uint16_t flags2 = *(uint16_t*) (madt + i);
				i += 2;
				break;
			}
			case 3: // io apic non maskable interrupt source
			{
				uint8_t nmi_source = *(uint8_t*) (madt + i);
				i += 2;
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
				i += 2;
				uint64_t address = *(uint64_t*) (madt + i);
				local_apic_address = address;
				break;
			}
			case 9: // processor local x2apic
			{
				i += 2;
				uint32_t processor_local_x2apic_id = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t flags2 = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t acpi_id = *(uint32_t*) (madt + i);
				i += 4;
				break;
			}
			default:
				madt += length;
				break;

		}
	}*/
}