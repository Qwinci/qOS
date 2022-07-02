#include "limine/limine.h"
#include <stddef.h>
#include "boot_info.h"

struct limine_framebuffer_request framebuffer_request = {
		.id = LIMINE_FRAMEBUFFER_REQUEST
};

struct limine_memmap_request memmap_request = {
		.id = LIMINE_MEMMAP_REQUEST
};

struct limine_rsdp_request rsdp_request = {
		.id = LIMINE_RSDP_REQUEST
};

struct limine_kernel_address_request kernel_address_request = {
		.id = LIMINE_KERNEL_ADDRESS_REQUEST
};

struct limine_module_request module_request = {
		.id = LIMINE_MODULE_REQUEST
};

extern __attribute__((noreturn)) void kmain(BootInfo boot_info);

static inline int strcmp(const char* str1, const char* str2) {
	for (; *str1 && *str2; ++str1, ++str2) {
		if (*str1 < *str2) return -1;
		else if (*str1 > *str2) return 1;
	}
	if (!*str1 && !*str2) return 0;
	return *str1 ? 1 : -1;
}

void start() {
	struct limine_framebuffer* fb = *framebuffer_request.response->framebuffers;
	for (size_t i = 0; i < memmap_request.response->entry_count; ++i) {
		struct limine_memmap_entry* entry = memmap_request.response->entries[i];
		switch (entry->type) {
			case LIMINE_MEMMAP_USABLE:
				entry->type = MEMORYTYPE_USABLE;
				break;
			case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
				entry->type = MEMORYTYPE_ACPI_RECLAIMABLE;
				break;
			case LIMINE_MEMMAP_ACPI_NVS:
				entry->type = MEMORYTYPE_ACPI_NVS;
				break;
			case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
				entry->type = MEMORYTYPE_BOOTLOADER_REACLAIMABLE;
				break;
			case LIMINE_MEMMAP_FRAMEBUFFER:
				entry->type = MEMORYTYPE_FRAMEBUFFER;
				break;
			case LIMINE_MEMMAP_RESERVED:
			case LIMINE_MEMMAP_BAD_MEMORY:
			case LIMINE_MEMMAP_KERNEL_AND_MODULES:
				entry->type = MEMORYTYPE_UNKNOWN;
		}
	}

	uintptr_t font = 0;
	for (size_t i = 0; i < module_request.response->module_count; ++i) {
		struct limine_file* module = module_request.response->modules[i];
		if (strcmp(module->cmdline, "Tamsyn8x16r.psf") == 0) {
			font = (uintptr_t) module->address;
			break;
		}
	}

	BootInfo boot_info;
	Framebuffer framebuffer = {.address = fb->address, .bpp = fb->bpp, .pitch = fb->pitch, .width = fb->width, .height = fb->height};
	boot_info.framebuffer = framebuffer;
	MemoryMap memory_map = {.entries = (MemoryEntry**) memmap_request.response->entries, .entry_count = memmap_request.response->entry_count};
	boot_info.memory_map = memory_map;
	boot_info.rsdp = rsdp_request.response->address;
	boot_info.kernel_virtual_address = kernel_address_request.response->virtual_base;
	boot_info.kernel_physical_address = kernel_address_request.response->physical_base;
	boot_info.font_start = font;

	kmain(boot_info);
}