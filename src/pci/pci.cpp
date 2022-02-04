#include "pci.hpp"
#include "../console/renderer.hpp"
#include "lookup.hpp"

void enumerateFunction(uint64_t deviceAddress, uint8_t function) {
	uint64_t functionAddress = deviceAddress + (function << 12);

	auto* header = reinterpret_cast<PCIHeader*>(functionAddress);
	if (header->vendorID == 0xFFFF) return;

	auto vendor = vendorLookup(header->vendorID);
	auto device = deviceLookup(header->deviceID);
	globalRenderer << Mode::Hex;

	if (vendor) {
		globalRenderer << vendor << " ";
	}
	else {
		globalRenderer << header->vendorID << ":";
	}
	if (device) {
		globalRenderer << device;
	}
	else {
		globalRenderer << header->deviceID;
	}

	globalRenderer << std::endl << Mode::Normal;
}

void enumerateDevice(uint64_t busAddress, uint8_t device) {
	uint64_t deviceAddress = busAddress + (device << 15);

	auto* header = reinterpret_cast<PCIHeader*>(deviceAddress);
	if (header->vendorID == 0xFFFF) return;

	for (uint8_t function = 0; function < 8; ++function) {
		enumerateFunction(deviceAddress, function);
	}
}

void enumerateBus(uint64_t baseAddress, uint64_t bus) {
	uint64_t busAddress = 0xffff800000000000 + baseAddress + (bus << 20);

	auto* header = reinterpret_cast<PCIHeader*>(busAddress);
	if (header->vendorID == 0xFFFF) return;

	for (uint8_t device = 0; device < 32; ++device) {
		enumerateDevice(busAddress, device);
	}
}

void enumeratePCI(MCFG* mcfg) {
	unsigned int entries = (mcfg->length - sizeof(MCFG)) / sizeof(DeviceConfig);

	for (unsigned int i = 0; i < entries; ++i) {
		auto* header = reinterpret_cast<DeviceConfig*>((uintptr_t) mcfg + sizeof(MCFG) + i * sizeof(PCIHeader));
		for (uint8_t bus = header->startBus; bus < header->endBus; ++bus) {
			enumerateBus(header->baseAddress, bus);
		}
	}
}