#pragma once
#include <stddef.h>
#include <stdint.h>
#include "utils/mem.h"

typedef struct BootInfo BootInfo;

typedef enum {
	MEMORY_ALLOC_TYPE_NORMAL,
	MEMORY_ALLOC_TYPE_LOW
} MemoryAllocType;

typedef enum {
	PAGEFLAG_PRESENT = 1 << 0,
	PAGEFLAG_RW = 1 << 1,
	PAGEFLAG_USER = 1 << 2,
	PAGEFLAG_WRITE_THROUGH = 1 << 3,
	PAGEFLAG_CACHE_DISABLE = 1 << 4,
	PAGEFLAG_HUGE_PAGE = 1 << 7,
	PAGEFLAG_GLOBAL = 1 << 8
} PageFlag;

void initialize_memory(const BootInfo* boot_info);
void* pmalloc(size_t count, MemoryAllocType type);
void pfree(void* ptr, size_t count);
void pmap(uintptr_t physical_address, uintptr_t virtual_address, PageFlag flags);
void punmap(uintptr_t virtual_address);
void preload();
void prefresh(uintptr_t address);