#pragma once
#include <stdint.h>
#include <stddef.h>

static inline const char* get_vendor(uint16_t id) {
	const char* name;
	switch (id) {
		case 0x10DE:
			name = "NVIDIA Corporation";
			break;
		case 0x10EC:
			name = "Realtek Semiconductor Co., Ltd.";
			break;
		case 0x1234:
			name = "QEMU";
			break;
		case 0x14C3:
			name = "MEDIATEK Corp.";
			break;
		case 0x1C5C:
			name = "SK hynix";
			break;
		case 0x8086:
			name = "Intel Corporation";
			break;
		default:
			name = NULL;
	}
	return name;
}

static inline const char* get_device(uint16_t id, uint16_t vendor_id) {
	const char* name;
	if (vendor_id == 0x8086) {
		switch (id) {
			case 0x10D3:
				name = "82574L Gigabit Network Connection";
				break;
			case 0x2918:
				name = "82801IB (ICH9) LPC Interface Controller";
				break;
			case 0x2922:
				name = "82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode]";
				break;
			case 0x2930:
				name = "82801I (ICH9 Family) SMBus Controller";
				break;
			case 0x2934:
				name = "82801I (ICH9 Family) USB UHCI Controller #1";
				break;
			case 0x2935:
				name = "82801I (ICH9 Family) USB UHCI Controller #2";
				break;
			case 0x2936:
				name = "82801I (ICH9 Family) USB UHCI Controller #3";
				break;
			case 0x293A:
				name = "82801I (ICH9 Family) USB2 EHCI Controller #1";
				break;
			case 0x29C0:
				name = "82G33/G31/P35/P31 Express DRAM Controller";
				break;
			case 0x9A09:
				name = "11th Gen Core Processor PCIe Controller";
				break;
			case 0x9A11:
				name = "GNA Scoring Accelerator module";
				break;
			case 0x9A14:
				name = "11th Gen Core Processor Host Bridge/DRAM Registers";
				break;
			case 0x9A49:
				name = "UHD Graphics";
				break;
			case 0xA082:
				name = "Tiger Lake-LP LPC Controller";
				break;
			case 0xA0A3:
				name = "Tiger Lake-LP SMBus Controller";
				break;
			case 0xA0A4:
				name = "Tiger Lake-LP SPI Controller";
				break;
			case 0xA0B0:
				name = "Tiger Lake-LP PCI Express Root Port #9";
				break;
			case 0xA0B1:
				name = "Tiger Lake-LP PCI Express Root Port #10";
				break;
			case 0xA0BC:
				name = "Tiger Lake-LP PCI Express Root Port #5";
				break;
			case 0xA0C5:
				name = "Tiger Lake-LP Serial IO I2C Controller #4";
				break;
			case 0xA0C6:
				name = "Tiger Lake-LP Serial IO I2C Controller #5";
				break;
			case 0xA0C8:
				name = "Tiger Lake-LP Smart Sound Technology Audio Controller";
				break;
			case 0xA0D3:
				name = "Tiger Lake-LP SATA Controller";
				break;
			case 0xA0E0:
				name = "Tiger Lake-LP Management Engine Interface";
				break;
			case 0xA0E8:
				name = "Tiger Lake-LP Serial IO I2C Controller #0";
				break;
			case 0xA0EB:
				name = "Tiger Lake-LP Serial IO I2C Controller #3";
				break;
			case 0xA0ED:
				name = "Tiger Lake-LP USB 3.2 Gen 2x1 xHCI Host Controller";
				break;
			case 0xA0EF:
				name = "Tiger Lake-LP Shared SRAM";
				break;
			default:
				name = NULL;
		}
	}
	else if (vendor_id == 0x1234) {
		if (id == 0x1111) {
			name = "Virtual Video Controller";
		}
		else name = NULL;
	}
	else if (vendor_id == 0x10DE) {
		if (id == 0x1C94) {
			name = "GP107M [GeForce MX350]";
		}
		else name = NULL;
	}
	else if (vendor_id == 0x1C5C) {
		if (id == 0x1339) {
			name = "BC511";
		}
		else name = NULL;
	}
	else if (vendor_id == 0x10EC) {
		if (id == 0x8168) {
			name = "RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller";
		}
		else name = NULL;
	}
	else if (vendor_id == 0x14C3) {
		if (id == 0x7961) {
			name = "MT7921 802.11ax PCI Express Wireless Network Adapter";
		}
		else name = NULL;
	}
	else name = NULL;
	return name;
}