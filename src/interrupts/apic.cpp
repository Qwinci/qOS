#include "apic.hpp"
#include <cstddef>
#include "../console/renderer.hpp"

void cpuWriteIoApic(uint32_t address, uint32_t reg, uint32_t value)
{
	auto volatile *ioapic = (uint32_t volatile *) (address + 0xffff800000000000);
	ioapic[0] = (reg & 0xff);
	ioapic[4] = value;
}

void initializeAPIC(MADT* madt) {
	// Dual legacy PIC installed
	if (madt->flags == 1) {
		// todo
		globalRenderer << "Remember to disable the PIC!" << std::endl;
		asm("mov $0xFF, %al");
		asm("out %al, $0xA1");
		asm("out %al, $0x21");
	}

	uint8_t bsp = 0;
	uint32_t ioApicAddress = 0;

	for (auto i = reinterpret_cast<size_t>(madt); i < madt->length - sizeof(MADT);) {
		uint8_t entryType = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i));
		uint8_t recordLength = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i + 1));
		i += 2;

		if (entryType == 0) { // processor local APIC
			uint8_t acpiProcessorID = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i));
			i += 1;
			uint8_t apicID = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i));
			i += 1;
			uint32_t flags = *(reinterpret_cast
<uint32_t*>((uintptr_t) madt + i));
			i += 4;
		}
		if (entryType == 1) { // IO APIC
			uint8_t ioApicID = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i));
			i += 1;
			ioApicAddress = *(reinterpret_cast<uint32_t*>((uintptr_t) madt + i));
			i += 4;
			uint32_t globalSystemInterruptBase = *(reinterpret_cast<uint32_t*>((uintptr_t) madt + i));
			i += 4;
		}
		else if (entryType == 2) { // IO APIC interrupt source override
			uint8_t busSource = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i));
			i += 1;
			uint8_t irqSource = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i));
			i += 1;
			uint32_t globalSystemInterrupt = *(reinterpret_cast<uint32_t*>((uintptr_t) madt + i));
			i += 4;
			uint16_t flags = *(reinterpret_cast<uint16_t*>((uintptr_t) madt + i));
			i += 2;

			if (irqSource == 1) {
				// todo
				globalRenderer << "Remember to use this override!" << std::endl;
			}
		}
		else {
			i += recordLength - 2;
		}
	}

	// set spurious interrupt register and enable APIC
	*(reinterpret_cast<uint32_t*>(madt->localAPICAddress + 0xF0 + 0xffff800000000000)) = 0xFF;

	cpuWriteIoApic(ioApicAddress, 0x13, 0);
	cpuWriteIoApic(ioApicAddress, 0x12, 0x20);
}