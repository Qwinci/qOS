#include "boot_info.h"
#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "paging/memory.h"

extern char kernel_end[];

__attribute__((noreturn)) void kmain(BootInfo boot_info) {
	initialize_printf(&boot_info.framebuffer, boot_info.font_start);
	initialize_memory(&boot_info.memory_map);
	printf("hello world\n");

	void* a = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
	printf("0x%h\n", a);
	printf("%u64\n", a);
	pfree(a, 1);

	for (size_t i = 0; i < boot_info.memory_map.entry_count; ++i) {
		MemoryEntry* entry = boot_info.memory_map.entries[i];
		uintptr_t address = entry->base;
		for (size_t i2 = 0; i2 < entry->size;) {
			if (address & 0xFFF) {
				address &= 0xFFFFFFFFFFFFF000;
				pmap(
						address + i2,
						address + i2 + 0xffff800000000000,
						PAGEFLAG_PRESENT | PAGEFLAG_RW);
			}
			else {
				pmap(
						address + i2,
						address + i2 + 0xffff800000000000,
						PAGEFLAG_PRESENT | PAGEFLAG_RW);
				i2 += 0x1000;
			}
		}
	}
	printf("done0\n");
	for (size_t i = 0; i < 0x100000000; i += 0x1000) {
		pmap(i, 0xffff800000000000 + i, PAGEFLAG_PRESENT | PAGEFLAG_RW);
	}

	printf("done1\n");

	for (uintptr_t i = 0; i < (uintptr_t) kernel_end - boot_info.kernel_virtual_address; i += 0x1000) {
		pmap(
				boot_info.kernel_physical_address + i,
				boot_info.kernel_virtual_address + i,
				PAGEFLAG_PRESENT
				);
	}

	while (true) __asm__("hlt");
}