#include <cstdint>
#include <cstddef>
#include "../thirdparty/stivale/stivale.hpp"
#include "bootInfo.hpp"

static uint8_t stack[8192];

static stivale2_header_tag_terminal terminal_hdr_tag {
		.tag = {
				.identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
				.next = 0
		},
		.flags = 0
};

static stivale2_header_tag_framebuffer framebuffer_hdr_tag {
		.tag = {
				.identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
				.next = (uint64_t) &terminal_hdr_tag
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

extern "C" void start(stivale2_struct* stivale2_struct) {
	stivale2_struct_tag_terminal* terminal_tag;
	terminal_tag = static_cast<stivale2_struct_tag_terminal*>(stivale2_get_tag(stivale2_struct,
	                                                                           STIVALE2_STRUCT_TAG_TERMINAL_ID));

	auto terminal_write = reinterpret_cast<void (*)(const char*, size_t)>(terminal_tag->term_write);

	terminal_write("Hello world\n", 12);

	auto memoryMap_ = reinterpret_cast<stivale2_struct_tag_memmap*>(stivale2_get_tag(stivale2_struct,
	                                                                                STIVALE2_STRUCT_TAG_MEMMAP_ID));
	auto frameBuffer = reinterpret_cast<stivale2_struct_tag_framebuffer*>(stivale2_get_tag(stivale2_struct,
	                                                                                       STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID));
	auto rsdp = reinterpret_cast<stivale2_struct_tag_rsdp*>(stivale2_get_tag(stivale2_struct,
	                                                                         STIVALE2_STRUCT_TAG_RSDP_ID));

	MemoryMap memoryMap {};
	memoryMap.count = memoryMap_->entries;
	memoryMap.entries = static_cast<MemoryMapEntry*>(__builtin_alloca(memoryMap_->entries * sizeof(MemoryMapEntry)));
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

	BootInfo bootInfo {
		{frameBuffer->framebuffer_width, frameBuffer->framebuffer_height,
		 frameBuffer->framebuffer_bpp, frameBuffer->framebuffer_addr},
		memoryMap, rsdp};
	kernelStart(bootInfo);
}