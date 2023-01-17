#include "memory.h"
#include <stdint.h>
#include <stddef.h>
#include "boot_info.h"

typedef struct Node {
	size_t size;
	struct Node* next;
	struct Node* prev;
} Node;

static Node* root = NULL;
static Node* end = NULL;

static inline Node* node_new(void* address, size_t size, Node* next, Node* prev) {
	Node* node = (Node*) address;
	node->size = size;
	node->next = next;
	node->prev = prev;
	return node;
}

static void try_merge_node(Node* node) {
	Node* next = node->next;
	if ((uintptr_t) node + node->size * 0x1000 == (uintptr_t) next) {
		node->size += next->size;

		if (next->next) {
			next->next->prev = node;
		}
		else {
			end = node;
		}
		node->next = next->next;
	}
	Node* prev = node->prev;
	if (prev && (uintptr_t) prev + prev->size * 0x1000 == (uintptr_t) node) {
		prev->size += node->size;

		if (node->next) {
			node->next->prev = prev;
		}
		else {
			end = prev;
		}
		node->prev->next = node->next;
	}
}

static void insert_node(void* ptr, size_t size) {
	if (!root) {
		root = node_new(ptr, size, NULL, NULL);
		end = root;
		return;
	}
	else if (ptr > (void*) end) {
		if ((uintptr_t) end + end->size * 0x1000 == (uintptr_t) ptr) {
			end->size += size;
			return;
		}
		end->next = node_new(ptr, size, NULL, end);
		end = end->next;
		return;
	}
	else if (ptr < (void*) root) {
		if ((uintptr_t) ptr + size * 0x1000 == (uintptr_t) root) {
			Node* new_root = node_new(ptr, root->size + size, root->next, NULL);
			if (root->next) root->next->prev = new_root;
			if (end == root) end = new_root;
			root = new_root;
			return;
		}
		Node* node = node_new(ptr, size, root, NULL);
		root->prev = node;
		root = node;
		return;
	}

	Node* node = root;
	while (node->next) {
		if ((void*) node->next > ptr) {
			Node* new_node = node_new(ptr, size, node->next, node);
			node->next->prev = new_node;
			node->next = new_node;
			try_merge_node(new_node);
			return;
		}
		node = node->next;
	}
}

void add_memory(void* memory, size_t size) {
	if (size < 0x1000) return;
	size_t pages = size / 0x1000;
	insert_node((void*) to_virt((uintptr_t) memory), pages);
}

static void remove_node(Node* node) {
	if (node->prev) {
		node->prev->next = node->next;
	}
	else {
		root = node->next;
	}

	if (node->next) {
		node->next->prev = node->prev;
	}
	else {
		end = node->prev;
	}
}

void* pmalloc(size_t count, MemoryAllocType type) {
	bool start_from_root = type == MEMORY_ALLOC_TYPE_LOW;
	Node* node = start_from_root ? root : end;

	if (!node) return NULL;
	do {
		if (node->size >= count) {
			size_t remaining = node->size - count;
			if (start_from_root) {
				if ((uintptr_t) node - HIGH_HALF_OFFSET > 0xFFFFFFFF) return NULL;
				if (remaining == 0) {
					remove_node(node);
					return (void*) node;
				}
				Node* new_node = (Node*) ((uintptr_t) node + count * 0x1000);
				new_node->size = remaining;
				new_node->prev = node->prev;
				new_node->next = node->next;

				if (node->prev) {
					node->prev->next = new_node;
				}
				else {
					root = new_node;
				}

				if (node->next) {
					node->next->prev = new_node;
				}
				else {
					end = new_node;
				}

				return (void*) node;
			}
			else {
				node->size = remaining;
				if (node->size == 0) {
					remove_node(node);
					return (void*) node;
				}
				return (void*) ((uintptr_t) node + remaining * 0x1000);
			}
		}
		node = start_from_root ? node->next : node->prev;
	} while (start_from_root ? node->next : node->prev);

	return NULL;
}

void pfree(void* ptr, size_t count) {
	insert_node(ptr, count);
}

extern char KERNEL_END[];

void initialize_memory(const BootInfo* boot_info) {
	HIGH_HALF_OFFSET = boot_info->hhdm_start;

	for (size_t i = 0; i < boot_info->memory_map.entry_count; ++i) {
		MemoryEntry* entry = boot_info->memory_map.entries[i];
		if (entry->type == MEMORYTYPE_USABLE) {
			if (entry->base == 0x1000) {
				entry->base += 0x1000;
				entry->size -= 0x1000;
			}
			add_memory((void*) entry->base, entry->size);
		}
	}

	for (size_t i = 0; i < boot_info->memory_map.entry_count; ++i) {
		const MemoryEntry* entry = boot_info->memory_map.entries[i];
		uintptr_t address = entry->base;

		if ((
				entry->type != MEMORYTYPE_FRAMEBUFFER &&
				entry->type != MEMORYTYPE_USABLE &&
				entry->type != MEMORYTYPE_KERNEL_AND_MODULES &&
				entry->type != MEMORYTYPE_BOOTLOADER_REACLAIMABLE) ||
				entry->base == boot_info->kernel_physical_address) {
			continue;
		}

		size_t align = 0;
		if (address & (0x1000 - 1)) {
			align = (address & (0x1000 - 1));
			address &= (0x1000 - 1);
		}

		size_t i2 = 0;
		while (((address + i2) & (SIZE_2MB - 1)) != 0 && i2 < entry->size + align) {
			pmap(address + i2, HIGH_HALF_OFFSET + address + i2, PAGEFLAG_PRESENT | PAGEFLAG_RW);
			i2 += 0x1000;
		}

		while (i2 < entry->size + align) {
			pmap(address + i2, HIGH_HALF_OFFSET + address + i2, PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_HUGE_PAGE);
			i2 += SIZE_2MB;
		}
	}

	for (size_t i = 0; i < 0x100000000; i += SIZE_2MB) {
		pmap(i, to_virt(i), PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_HUGE_PAGE);
	}

	size_t i = 0;
	const size_t KERNEL_SIZE = (uintptr_t) KERNEL_END - boot_info->kernel_virtual_address;
	while (((boot_info->kernel_physical_address + i) & (SIZE_2MB - 1)) != 0 && i < KERNEL_SIZE) {
		pmap(
				boot_info->kernel_physical_address + i,
				boot_info->kernel_virtual_address + i,
				PAGEFLAG_PRESENT | PAGEFLAG_RW
				);
		i += 0x1000;
	}

	while (i < KERNEL_SIZE) {
		pmap(
				boot_info->kernel_physical_address + i,
				boot_info->kernel_virtual_address + i,
				PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_HUGE_PAGE
				);
		i += SIZE_2MB;
	}

	preload();
}