#pragma once
#include <cstdint>

static inline const char* vendorLookup(uint16_t vendorID) {
	switch (vendorID) {
		case 0x8086:
			return "Intel Corporation";
		case 0x1234:
			return "Qemu";
		default:
			return nullptr;
	}
}

static inline const char* deviceLookup(uint16_t deviceID) {
	switch (deviceID) {
		case 0x1111:
			return "Virtual GPU";
		case 0x29c0:
			return "82G33/G31/P35/P31 Express DRAM Controller";
		case 0x10d3:
			return "82574L Gigabit Network Connection";
		case 0x2918:
			return "82801IB (ICH9) LPC Interface Controller";
		case 0x2922:
			return "82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode]";
		case 0x2930:
			return "82801I (ICH9 Family) SMBus Controller";
		case 0x2934:
			return "82801I (ICH9 Family) USB UHCI Controller #1";
		case 0x2935:
			return "82801I (ICH9 Family) USB UHCI Controller #2";
		case 0x2936:
			return "82801I (ICH9 Family) USB UHCI Controller #3";
		case 0x293a:
			return "82801I (ICH9 Family) USB2 EHCI Controller #1";
		default:
			return nullptr;
	}
}