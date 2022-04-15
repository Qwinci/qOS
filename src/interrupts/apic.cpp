#include "apic.hpp"
#include <cstddef>
#include "../console/renderer.hpp"
#include "utils/io.hpp"
#include "utils/memory.hpp"

void lapicWrite(uintptr_t lapic, uint16_t reg, uint32_t value) {
	*(reinterpret_cast<uint32_t*>(lapic + reg)) = value;
}
uint32_t lapicRead(uintptr_t lapic, uint16_t reg) {
	return *(reinterpret_cast<uint32_t*>(lapic + reg));
}

void mdelay(uint16_t ms) {
	for (uint16_t i = 0; i < ms; ++i) {
		for (uint16_t micro = 0; micro < 1000; ++micro) {
			io_wait();
		}
	}
}
void udelay(uint16_t us) {
	for (uint16_t i = 0; i < us; ++i) {
		io_wait();
	}
}

extern "C" uint8_t works = 0;

[[noreturn]] extern "C" void ap_start(int apicID) {
	asm volatile("mov $0x3000, %rsp");
	globalRenderer << "I started!" << std::endl;
	//works = 1;
	while(true) asm("hlt");
}

extern uintptr_t kernelVirtualAddress;

void initializeAPIC(MADT* madt) {
	uint8_t bsp = 0;
	uint64_t cpuCount = 0;
	uint64_t lapicAddress = madt->localAPICAddress + 0xFFFF800000000000;
	asm("mov $1, %%eax; cpuid; shrl $24, %%ebx;" : "=b"(bsp));

	lapicWrite(lapicAddress, 0xF0, 0xFF | 1 << 8);

	uint8_t lapicIDs[8] {};

	for (auto i = sizeof(MADT); i < madt->length;) {
		uint8_t entryType = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i));
		uint8_t recordLength = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i + 1));
		i += 2;

		if (entryType == 0) { // processor local APIC
			uint8_t acpiProcessorID = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i + 1));
			uint8_t apicID = *(reinterpret_cast<uint8_t*>((uintptr_t) madt + i + 1));
			uint32_t flags = *(reinterpret_cast<uint32_t*>((uintptr_t) madt + i + 1));

			if (apicID != bsp) {
				if (!(flags & 1)) {
					if (flags & 1 << 1) {
						lapicIDs[cpuCount++] = apicID;
					}
				}
				else {
					lapicIDs[cpuCount++] = apicID;
				}
			}
		}
		if (entryType == 1) { // IO APIC
		}
		else if (entryType == 2) { // IO APIC interrupt source override
		}
		i += recordLength - 2;
	}

	globalRenderer << "Found " << cpuCount << " cpus." << std::endl;
	globalRenderer << "Starting application processor startup sequence..." << std::endl;
	memcpy((void*)(0xffff800000000000 + 0x1000), (void*) (kernelVirtualAddress), 0x1000);

	for (size_t i = 0; i < cpuCount; ++i) {
		uint32_t init = lapicRead(lapicAddress, 0x300);
		init |= 5 << 8; // INIT
		init |= 1 << 12; // set status to 1
		init |= 1 << 14; // assert
		uint32_t target = lapicIDs[i] & 0b1111 << 24;
		lapicWrite(lapicAddress, 0x310, target);
		lapicWrite(lapicAddress, 0x300, init);
		while (lapicRead(lapicAddress, 0x300) & 1 << 12) {
			asm("pause" : : : "memory");
		}

		globalRenderer << "init IPI assert success" << std::endl;

		init = lapicRead(lapicAddress, 0x300);
		init |= 1 << 12; // set status to 1
		init &= ~(1 << 14); // deassert
		init |= 1 << 15; // deassert
		lapicWrite(lapicAddress, 0x310, target);
		lapicWrite(lapicAddress, 0x300, init);
		while (lapicRead(lapicAddress, 0x300) & 1 << 12) {
			asm("pause" : : : "memory");
		}

		globalRenderer << "init IPI deassert success" << std::endl;

		mdelay(10);

		for (uint8_t c = 0; c < 2; ++c) {
			works = 0;
			uint32_t startup = 0;
			startup |= 1; // start from page 1
			startup |= 6 << 8; // STARTUP
			startup |= 1 << 12; // set status to 1
			startup |= 1 << 14;
			lapicWrite(lapicAddress, 0x310, target);
			lapicWrite(lapicAddress, 0x300, startup);
			udelay(200);
			while (lapicRead(lapicAddress, 0x300) & 1 << 12) {
				asm("pause" : : : "memory");
			}

			globalRenderer << "init STARTUP success" << std::endl;
		}
		while (!works) {
			asm volatile("pause" : : : "memory");
		}
		globalRenderer << "works: " << works << std::endl;
	}
}