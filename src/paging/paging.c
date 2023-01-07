#include "memory.h"
#include <stdint.h>
#include "std/memory.h"
#include "stdio.h"

uint64_t* pml4 = NULL;

void pmap(uintptr_t physical_address, uintptr_t virtual_address, PageFlag flags) {
	virtual_address >>= 12;
	uint64_t pt_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pd_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pdp_offset = virtual_address & 0x1FF;
	virtual_address >>= 9;
	uint64_t pml4_offset = virtual_address & 0x1FF;

	bool huge = false;

	if (flags & PAGEFLAG_HUGE_PAGE) {
		huge = true;
		flags &= ~PAGEFLAG_HUGE_PAGE;
	}

	if (!pml4) {
		pml4 = (uint64_t*) ((uintptr_t) pmalloc(1, MEMORY_ALLOC_TYPE_LOW));
		memset(pml4, 0, 0x1000);
	}

	uint64_t* pdp_entry;
	if (pml4[pml4_offset] & PAGEFLAG_PRESENT) {
		pdp_entry = (uint64_t*) to_virt(pml4[pml4_offset] & 0x000FFFFFFFFFF000);
	}
	else {
		pdp_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset(pdp_entry, 0, 0x1000);
		pml4[pml4_offset] |= to_phys((uint64_t) pdp_entry) | flags;
	}

	uint64_t* pd_entry;
	if (pdp_entry[pdp_offset] & PAGEFLAG_PRESENT) {
		pd_entry = (uint64_t*) to_virt(pdp_entry[pdp_offset] & 0x000FFFFFFFFFF000);
	}
	else {
		pd_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset(pd_entry, 0, 0x1000);
		pdp_entry[pdp_offset] |= to_phys((uint64_t) pd_entry) | flags;
	}

	if (huge) {
		flags |= PAGEFLAG_HUGE_PAGE;
		uint64_t* entry = pd_entry + pd_offset;
		*entry = physical_address | flags;
		return;
	}

	uint64_t* pt_entry;
	if (pd_entry[pd_offset] & PAGEFLAG_PRESENT) {
		pt_entry = (uint64_t*) to_virt(pd_entry[pd_offset] & 0x000FFFFFFFFFF000);
	}
	else {
		pt_entry = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
		memset(pt_entry, 0, 0x1000);
		pd_entry[pd_offset] |= to_phys((uint64_t) pt_entry) | flags;
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
		return;
	}

	uint64_t* pdp_entry;
	if (pml4[pml4_offset] & PAGEFLAG_PRESENT) {
		pdp_entry = (uint64_t*) to_virt(pml4[pml4_offset] & 0x000FFFFFFFFFF000);
	}
	else {
		return;
	}

	uint64_t* pd_entry;
	if (pdp_entry[pdp_offset] & PAGEFLAG_PRESENT) {
		pd_entry = (uint64_t*) to_virt(pdp_entry[pdp_offset] & 0x000FFFFFFFFFF000);
	}
	else {
		return;
	}

	uint64_t* pt_entry;
	if (pd_entry[pd_offset] & PAGEFLAG_PRESENT) {
		pt_entry = (uint64_t*) to_virt(pd_entry[pd_offset] & 0x000FFFFFFFFFF000);
	}
	else {
		return;
	}

	pt_entry[pt_offset] = 0;
}

void preload() {
	__asm__ volatile("mov cr3, %0" : : "r"(to_phys((uintptr_t) pml4)));
}

void prefresh(uintptr_t address) {
	__asm__ volatile("invlpg %0" : : "m"(address));
}