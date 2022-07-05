#include "memory.h"
#include "boot_info.h"
#include <stdint.h>
#include <stddef.h>
#include "std/memory.h"

static size_t* bitmap = NULL;
static size_t bitmap_size = 0;
static size_t bitmap_real_size = 0;

void initialize_memory(MemoryMap* memory_map) {
	size_t best_size = 0;
	uintptr_t best_entry = 0;
	size_t largest_address = 0;
	for (size_t i = 0; i < memory_map->entry_count; ++i) {
		MemoryEntry* entry = memory_map->entries[i];
		if (entry->type == MEMORYTYPE_USABLE && entry->size > best_size) {
			best_entry = entry->base;
			best_size = entry->size;
		}
		if (entry->base >= 0xffff800000000000) {
			entry->base -= 0xffff800000000000;
		}
		if (entry->base > largest_address) largest_address = entry->base;
	}

	size_t page_count = largest_address / 0x1000 & 0xFFF ? largest_address / 0x1000 + 1 : largest_address / 0x1000;
	bitmap_size = page_count;
	size_t real_size = page_count & 0x7 ? page_count / 8 + 1 : page_count / 8;
	bitmap_real_size = real_size;

	if (best_size < bitmap_real_size) *(uintptr_t*) 0xFFFFFFFFFFFF = 0;

	bitmap = (size_t*) best_entry;

	memset(bitmap, 0xFF, bitmap_real_size);

	for (size_t i = 0; i < memory_map->entry_count; ++i) {
		MemoryEntry* entry = memory_map->entries[i];
		if (entry->type == MEMORYTYPE_USABLE) {
			for (size_t i2 = 0; i2 < entry->size / 0x1000; ++i2) {
				bitmap[(entry->base + 0x1000 * i2) / 0x1000 / 64] &= ~(1 << ((entry->base + 0x1000 * i2) / 0x1000 % 64));
			}
		}
	}
}

void* pmalloc(size_t count, MemoryAllocType type) {
	size_t found = 0;
	size_t starting_index = 0;
	if (type == MEMORY_ALLOC_TYPE_LOW) {
		for (size_t i = 0; i < bitmap_size; ++i) {
			size_t* entry = &bitmap[i / 64];
			if (!(*entry & (1 << (i % 64)))) {
				if (found == 0) starting_index = i;
				++found;
				if (found == count) {
					for (size_t i2 = 0; i2 < count; ++i2) {
						bitmap[(starting_index + i2) / 64] |= 1 << ((starting_index + i2) % 64);
					}
					return (void*) (starting_index * 0x1000);
				}
			}
			else found = 0;
		}
	}
	else {
		for (size_t i = bitmap_size - 1; i > 0; --i) {
			size_t* entry = &bitmap[i / 64];
			if (!(*entry & (1 << (i % 64)))) {
				if (found == 0) starting_index = i;
				++found;
				if (found == count) {
					for (size_t i2 = 0; i2 < count; ++i2) {
						bitmap[(starting_index - i2) / 64] |= 1 << ((starting_index - i2) % 64);
					}
					return (void*) (i * 0x1000);
				}
			}
			else found = 0;
		}
	}
	return NULL;
}
void pfree(void* ptr, size_t count) {
	uintptr_t memory = (uintptr_t) ptr;
	for (size_t i = 0; i < count; ++i) {
		bitmap[(memory + i * 0x1000) / 0x1000 / 64] &= ~(1 << ((memory + i * 0x1000) / 0x1000 % 64));
	}
}