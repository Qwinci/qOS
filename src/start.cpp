#include <cstddef>
#include "../thirdparty/stivale/stivale.hpp"
#include "bootInfo.hpp"
#include "utils/string.hpp"

static uint8_t stack[8192];

static stivale2_header_tag_framebuffer framebuffer_hdr_tag {
		.tag = {
				.identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
				.next = 0
		},
		.framebuffer_width = 0,
		.framebuffer_height = 0,
		.framebuffer_bpp = 0
};

__attribute__((section(".stivale2hdr"), used))
static stivale2_header stivale_hdr {
		.entry_point = 0,
		.stack = (uintptr_t) stack + sizeof(stack),
		.flags = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
		.tags = (uintptr_t) &framebuffer_hdr_tag
};

void* stivale2_get_tag(stivale2_struct* stivale2_struct, uint64_t id) {
	auto* current_tag = reinterpret_cast<stivale2_tag*>(stivale2_struct->tags);
	while (true) {
		if (!current_tag) return nullptr;

		if (current_tag->identifier == id) {
			return current_tag;
		}

		current_tag = reinterpret_cast<stivale2_tag*>(current_tag->next);
	}
}

extern int kernelStart(BootInfo bootInfo);

extern "C" __attribute__((used)) void start(stivale2_struct* stivale2_struct) {

	auto memoryMap_ = reinterpret_cast<stivale2_struct_tag_memmap*>(stivale2_get_tag(stivale2_struct,
	                                                                                STIVALE2_STRUCT_TAG_MEMMAP_ID));
	auto frameBuffer = reinterpret_cast<stivale2_struct_tag_framebuffer*>(stivale2_get_tag(stivale2_struct,
	                                                                                       STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID));
	auto rsdp = reinterpret_cast<stivale2_struct_tag_rsdp*>(stivale2_get_tag(stivale2_struct,
	                                                                         STIVALE2_STRUCT_TAG_RSDP_ID));
	auto modules = reinterpret_cast<stivale2_struct_tag_modules*>(stivale2_get_tag(stivale2_struct,
																				   STIVALE2_STRUCT_TAG_MODULES_ID));

	auto kernelBaseAddress = reinterpret_cast<stivale2_struct_tag_kernel_base_address*>
			(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID));

	auto kernelFileAddress = reinterpret_cast<stivale2_struct_tag_kernel_file*>
		(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_FILE_ID));

	MemoryMap memoryMap {};
	memoryMap.size = memoryMap_->entries;
	for (size_t i = 0; i < memoryMap_->entries; ++i) {
		MemoryMapEntry entry{};
		entry.size = memoryMap_->memmap[i].length;
		entry.address = memoryMap_->memmap[i].base;
		switch (memoryMap_->memmap[i].type) {
			case STIVALE2_MMAP_USABLE:
				entry.type = MemoryType::Usable;
				break;
			case STIVALE2_MMAP_FRAMEBUFFER:
				entry.type = MemoryType::FrameBuffer;
				break;
			case STIVALE2_MMAP_KERNEL_AND_MODULES:
				entry.type = MemoryType::Kernel;
				break;
			default:
				entry.type = MemoryType::Unknown;
				break;
		}
		memoryMap.entries[i] = entry;
	}

	uintptr_t fontStart = 0;
	uint64_t fontSize = 0;
	for (int i = 0; i < modules->module_count; ++i) {
		if (strcmp(modules->modules[i].string, "Uni2-VGA16.psf") == 0) {
			fontStart = modules->modules[i].begin;
			fontSize = modules->modules[i].end - modules->modules[i].begin;
		}
	}

	BootInfo bootInfo {
			{frameBuffer->framebuffer_width, frameBuffer->framebuffer_height,
		 frameBuffer->framebuffer_bpp, frameBuffer->framebuffer_pitch, frameBuffer->framebuffer_addr},
			fontStart, fontSize,memoryMap, kernelFileAddress->kernel_file,
			kernelBaseAddress->physical_base_address,
			kernelBaseAddress->virtual_base_address, reinterpret_cast<void*>(rsdp->rsdp)};

	kernelStart(bootInfo);
}