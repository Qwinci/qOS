#pragma once
#include <stdint.h>

const static inline uintptr_t to_virt(uintptr_t phys) {
	return phys + 0xFFFF800000000000;
}

const static inline uintptr_t to_phys(uintptr_t virt) {
	return virt - 0xFFFF800000000000;
}