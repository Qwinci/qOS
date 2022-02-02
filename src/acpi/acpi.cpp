#include "acpi.hpp"
#include "../utils/string.hpp"
#include "../console/renderer.hpp"

void* findTable(RSDP* rsdp, const char* signature) {
	if (rsdp->revision == 0) {
		RSDT* rsdt = reinterpret_cast<RSDT*>(0xffff800000000000 + rsdp->rsdtAddress);
		unsigned int entries = (rsdt->header.length - sizeof(SDTHeader)) / 4;

		for (unsigned int i = 0; i < entries; ++i) {
			auto* header = reinterpret_cast<SDTHeader*>(0xffff800000000000 + rsdt->addresses[i]);
			globalRenderer << header->signature[0] << header->signature[1] << header->signature[2] << header->signature[3] << std::endl;
			if (strncmp(header->signature, signature, 4)) {
				return header;
			}
		}
	}
	else {
		XSDT* xsdt = reinterpret_cast<XSDT*>(0xffff800000000000 + rsdp->xsdtAddress);
		unsigned int entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;

		for (unsigned int i = 0; i < entries; ++i) {
			auto* header = reinterpret_cast<SDTHeader*>(0xffff800000000000 + xsdt->addresses[i]);
			if (strncmp(header->signature, signature, 4)) {
				return header;
			}
		}
	}

	globalRenderer << "ACPI table " << signature << " wasn't found." << std::endl;
	return nullptr;
}