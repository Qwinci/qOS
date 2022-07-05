#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct MemoryMap MemoryMap;

typedef enum {
	MEMORY_ALLOC_TYPE_NORMAL,
	MEMORY_ALLOC_TYPE_LOW
} MemoryAllocType;

typedef enum {
	PAGEFLAG_PRESENT = 1 << 0,
	PAGEFLAG_RW = 1 << 1,
	PAGEFLAG_USER = 1 << 2
} PageFlag;

void initialize_memory(MemoryMap* memory_map);
void* pmalloc(size_t count, MemoryAllocType type);
void pfree(void* ptr, size_t count);
void pmap(uintptr_t physical_address, uintptr_t virtual_address, PageFlag flags);
void punmap(uintptr_t virtual_address);
void preload();