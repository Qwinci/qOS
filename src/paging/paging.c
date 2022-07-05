#include "memory.h"
#include <stdint.h>
#include "std/memory.h"

static uint64_t* pml4 = NULL;
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
		pml4 = pmalloc(1, MEMORY_ALLOC_TYPE_LOW);
		memset(pml4, 0, 0x1000);
	}

	uint64_t* pdp_entry;
	if (pml4[pml4_offset] & PAGEFLAG_PRESENT) {
		pdp_entry = (uint64_t*) (pml4[pml4_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pdp_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset((void*) pdp_entry, 0, 0x1000);
		pml4[pml4_offset] |= (uint64_t) pdp_entry | flags;
	}

	uint64_t* pd_entry;
	if (pdp_entry[pdp_offset] & PAGEFLAG_PRESENT) {
		pd_entry = (uint64_t*) (pdp_entry[pdp_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pd_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset((void*) pd_entry, 0, 0x1000);
		pdp_entry[pdp_offset] |= (uint64_t) pd_entry | flags;
	}

	uint64_t* pt_entry;
	if (pd_entry[pd_offset] & PAGEFLAG_PRESENT) {
		pt_entry = (uint64_t*) (pd_entry[pd_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pt_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset((void*) pt_entry, 0, 0x1000);
		pd_entry[pd_offset] |= (uint64_t) pt_entry | flags;
	}

	pt_entry[pt_offset] = physical_address | flags;
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
		pdp_entry = (uint64_t*) (pml4[pml4_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pdp_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset((void*) pdp_entry, 0, 0x1000);
		pml4[pml4_offset] |= (uint64_t) pdp_entry;
	}

	uint64_t* pd_entry;
	if (pdp_entry[pdp_offset] & PAGEFLAG_PRESENT) {
		pd_entry = (uint64_t*) (pdp_entry[pdp_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pd_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset((void*) pd_entry, 0, 0x1000);
		pdp_entry[pdp_offset] |= (uint64_t) pd_entry;
	}

	uint64_t* pt_entry;
	if (pd_entry[pd_offset] & PAGEFLAG_PRESENT) {
		pt_entry = (uint64_t*) (pd_entry[pd_offset] & 0xFFFFFFFFFFFF000);
	}
	else {
		pt_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset((void*) pt_entry, 0, 0x1000);
		pd_entry[pd_offset] |= (uint64_t) pt_entry;
	}

	pt_entry[pt_offset] = 0;
}

void preload() {
	__asm__("mov cr3, %0" : : "r"(pml4));
}