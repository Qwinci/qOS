#include "memory.h"
#include <stdint.h>
#include <stddef.h>
#include "boot_info.h"
#include "stdio.h"

typedef struct Node {
	size_t size;
	struct Node* next;
	struct Node* prev;
} Node;

static Node* root = NULL;
static Node* end = NULL;
static size_t offset = 0;

Node* node_new(void* address, size_t size, Node* next, Node* prev) {
	Node* node = (Node*) address;
	node->size = size;
	node->next = next;
	node->prev = prev;
	return node;
}

void try_merge_node(Node* node, Node** e) {
	if (node->next && (uintptr_t) node + node->size * 0x1000 == (uintptr_t) node->next) {
		node->size += node->next->size;
		node->next = node->next->next;
		if (node->next->next) node->next->next->prev = node;
		if (node->next == *e) *e = node;
	}
	if (node->prev && (uintptr_t) node->prev + node->prev->size * 0x1000 == (uintptr_t) node) {
		node->prev->size += node->size;
		node->prev->next = node->next;
		if (node->next) node->next->prev = node->prev;
		if (node == *e) *e = node->prev;
	}
}

void insert_node(void* ptr, size_t size) {
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
		try_merge_node(end, &end);
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
		if (end == root) end = node;
		root = node;
		return;
	}

	Node* node = root;
	while (node->next) {
		if ((void*) node > ptr) {
			if (node == root) {
				Node* old_root = root;
				root = node_new(ptr, size, old_root, NULL);
				old_root->prev = root;
				try_merge_node(old_root, &end);
			}
			else {
				Node* new_node = node_new(ptr, size, node, node->prev);
				node->prev->next = new_node;
				node->prev = node->prev->next;
				try_merge_node(new_node, &end);
			}
			return;
		}
		node = node->next;
	}

	node->next = node_new(ptr, size, NULL, node);
	end = node->next;
	try_merge_node(node, &end);
}

void add_memory(void* memory, size_t size) {
	if (size < 0x1000) return;
	size_t pages = size / 0x1000;
	insert_node(memory, pages);
}

void remove_node(Node* node, Node** r, Node** e) {
	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;
	if (node == *r) *r = node->next;
	if (node == *e) *e = node->prev;
}

void* pmalloc(size_t count, MemoryAllocType type) {
	if (type == MEMORY_ALLOC_TYPE_NORMAL) {
		Node* node = root;
		if (!node) return NULL;
		do {
			if (node->size >= count) {
				size_t remaining = node->size - count;
				remove_node(node, &root, &end);
				if (remaining > 0) {
					insert_node((void*) ((uintptr_t) node + count * 0x1000), remaining);
				}

				return (void*) node;
			}
			node = node->next;
		} while (node->next);
	}
	else {
		Node* node = end;
		if (!node) return NULL;
		do {
			if ((uintptr_t) node > 0x100000000) return NULL;
			if (node->size >= count) {
				size_t remaining = node->size - count;
				remove_node(node, &root, &end);
				if (remaining > 0) {
					insert_node((void*) ((uintptr_t) node + count * 0x1000), remaining);
				}

				return (void*) node;
			}
		} while (node->next);
	}

	return NULL;
}

void pfree(void* ptr, size_t count) {
	insert_node(ptr, count);
}

extern char kernel_end[];

void initialize_memory(const BootInfo* boot_info) {
	for (size_t i = 0; i < boot_info->memory_map.entry_count; ++i) {
		const MemoryEntry* entry =boot_info->memory_map.entries[i];
		if (entry->type == MEMORYTYPE_USABLE) {
			add_memory((void*) entry->base, entry->size);
		}
	}

	for (size_t i = 0; i < boot_info->memory_map.entry_count; ++i) {
		const MemoryEntry* entry = boot_info->memory_map.entries[i];
		uintptr_t address = entry->base;
		for (size_t i2 = 0; i2 < entry->size;) {
			if (address & 0xFFF) {
				address &= 0xFFFFFFFFFFFFF000;
				pmap(
						address + i2,
						address + i2 + 0xFFFF800000000000,
						PAGEFLAG_PRESENT | PAGEFLAG_RW);
			}
			else {
				pmap(
						address + i2,
						address + i2 + 0xFFFF800000000000,
						PAGEFLAG_PRESENT | PAGEFLAG_RW);
				i2 += 0x1000;
			}
		}
	}
	for (size_t i = 0; i < 0x100000000; i += 0x1000) {
		pmap(i, 0xFFFF800000000000 + i, PAGEFLAG_PRESENT | PAGEFLAG_RW);
		//pmap(i, i, PAGEFLAG_PRESENT | PAGEFLAG_RW);
	}

	for (uintptr_t i = 0; i < (uintptr_t) kernel_end - boot_info->kernel_virtual_address; i += 0x1000) {
		pmap(
				boot_info->kernel_physical_address + i,
				boot_info->kernel_virtual_address + i,
				PAGEFLAG_PRESENT | PAGEFLAG_RW
		);
	}

	preload();

	if (!root) return;
	Node* node = (Node*) ((uintptr_t) root + 0xFFFF800000000000);
	root = (Node*) ((uintptr_t) root + 0xFFFF800000000000);
	end = (Node*) ((uintptr_t) end + 0xFFFF800000000000);
	while (node) {
		if (node->next) {
			node->next = (Node*) ((uintptr_t) node->next + 0xFFFF800000000000);
		}
		if (node->prev) {
			node->prev = (Node*) ((uintptr_t) node->prev + 0xFFFF800000000000);
		}
		node = node->next;
	}
}