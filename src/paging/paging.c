#include "memory.h"
#include <stdint.h>
#include "std/memory.h"

uint64_t* pml4 = NULL;
size_t p_offset = 0;
void pmap(uintptr_t physical_address, uintptr_t virtual_address, PageFlag flags) {
	virtual_address >>= 12;
	uint64_t pt_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pd_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pdp_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pml4_offset = virtual_address & 0x1FF;

	physical_address &= 0xFFFFFFFFFFFF000;

	if (!pml4) {
		pml4 = (uint64_t*) ((uintptr_t) pmalloc(1, MEMORY_ALLOC_TYPE_LOW) - p_offset);
		memset(pml4, 0, 0x1000);
	}

	uint64_t* pdp_entry;
	if (((uint64_t*) ((uintptr_t) pml4 + p_offset))[pml4_offset] & PAGEFLAG_PRESENT) {
		pdp_entry = (uint64_t*) (((uint64_t*) ((uintptr_t) pml4 + p_offset))[pml4_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pdp_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL) - p_offset;
		memset((void*) pdp_entry + p_offset, 0, 0x1000);
		((uint64_t*) ((uintptr_t) pml4 + p_offset))[pml4_offset] |= (uint64_t) pdp_entry | flags;
	}

	uint64_t* pd_entry;
	if (((uint64_t*) ((uintptr_t) pdp_entry + p_offset))[pdp_offset] & PAGEFLAG_PRESENT) {
		pd_entry = (uint64_t*) (((uint64_t*) ((uintptr_t) pdp_entry + p_offset))[pdp_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pd_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL) - p_offset;
		memset((void*) pd_entry + p_offset, 0, 0x1000);
		((uint64_t*) ((uintptr_t) pdp_entry + p_offset))[pdp_offset] |= (uint64_t) pd_entry | flags;
	}

	uint64_t* pt_entry;
	if (((uint64_t*) ((uintptr_t) pd_entry + p_offset))[pd_offset] & PAGEFLAG_PRESENT) {
		pt_entry = (uint64_t*) (((uint64_t*) ((uintptr_t) pd_entry + p_offset))[pd_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pt_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL) - p_offset;
		memset((void*) pt_entry + p_offset, 0, 0x1000);
		((uint64_t*) ((uintptr_t) pd_entry + p_offset))[pd_offset] |= (uint64_t) pt_entry | flags;
	}

	((uint64_t*) ((uintptr_t) pt_entry + p_offset))[pt_offset] = physical_address | flags;
}
void punmap(uintptr_t virtual_address) {
	virtual_address >>= 12;
	uint64_t pt_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pd_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pdp_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pml4_offset = virtual_address & 0x1FF;

	if (!pml4) {
		pml4 = pmalloc(1, MEMORY_ALLOC_TYPE_LOW);
		memset(pml4, 0, 0x1000);
	}

	uint64_t* pdp_entry;
	if (pml4[pml4_offset] & PAGEFLAG_PRESENT) {
		pdp_entry = (uint64_t*) (((uint64_t*) ((uintptr_t) pml4 + p_offset))[pml4_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		return;
	}

	uint64_t* pd_entry;
	if (pdp_entry[pdp_offset] & PAGEFLAG_PRESENT) {
		pd_entry = (uint64_t*) (((uint64_t*) ((uintptr_t) pdp_entry + p_offset))[pdp_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		return;
	}

	uint64_t* pt_entry;
	if (pd_entry[pd_offset] & PAGEFLAG_PRESENT) {
		pt_entry = (uint64_t*) (((uint64_t*) ((uintptr_t) pd_entry + p_offset))[pd_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		return;
	}

	pt_entry[pt_offset] = 0;
}

void preload() {
	__asm__("mov cr3, %0" : : "r"(pml4));
}

void prefresh(uintptr_t address) {
	__asm__("invlpg %0" : : "m"(address));
}