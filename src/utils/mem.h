#pragma once
#include <stdint.h>

extern size_t HIGH_HALF_OFFSET;

static inline uintptr_t to_virt(uintptr_t phys) {
	return phys + HIGH_HALF_OFFSET;
}

static inline uintptr_t to_phys(uintptr_t virt) {
	return virt - HIGH_HALF_OFFSET;
}

static const uintptr_t SIZE_2MB = 0x200000;