#include "malloc.h"
#include "paging/memory.h"
#include "stdio.h"

typedef struct Node {
	struct Node* next;
} Node;

typedef struct {
	Node* root;
} FreeList;

// 8 16 32 64 128 256 512 1024 2048

static inline const size_t get_size_index(size_t size) {
	if (size <= 8) return 0;
	else if (size <= 16) return 1;
	else if (size <= 32) return 2;
	else if (size <= 64) return 3;
	else if (size <= 128) return 4;
	else if (size <= 256) return 5;
	else if (size <= 512) return 6;
	else if (size <= 1024) return 7;
	else if (size <= 2048) return 8;
	else return 9;
}

static inline size_t pow(size_t value, uint8_t pow) {
	for (size_t orig = value; pow > 1; --pow) value *= orig;
	return value;
}

static inline const size_t index_to_size(size_t index) {
	return index < 9 ? pow(2, index + 3) : 0;
}

static FreeList free_lists[9] = {};

static void free_list_insert(size_t index, void* ptr);

static void try_merge_node(size_t index, Node* parent, Node* prev) { // NOLINT(misc-no-recursion)
	Node* inserted_node = parent->next;
	if (index < 8) {
		if ((uintptr_t) parent + index_to_size(index) == (uintptr_t) inserted_node) {
			if (prev) prev->next = inserted_node->next;
			free_list_insert(index + 1, (void*) parent);
		}
	}
	else if (index == 8) {
		if ((uintptr_t) parent + index_to_size(index) == (uintptr_t) inserted_node) {
			if (prev) prev->next = inserted_node->next;
			pfree(parent, 1);
		}
	}
}

static void free_list_insert(size_t index, void* ptr) { // NOLINT(misc-no-recursion)
	FreeList* list = &free_lists[index];
	Node* new_node = (Node*) ptr;
	new_node->next = NULL;

	if (!list->root) {
		list->root = new_node;
		return;
	}

	Node* node = list->root;
	Node* prev = NULL;
	while (node->next) {
		if ((uintptr_t) node >= (uintptr_t) ptr) {
			Node* old_next = node->next;
			node->next = new_node;
			new_node->next = old_next;
			try_merge_node(index, node, prev);
			return;
		}
		prev = node;
		node = node->next;
	}

	node->next = new_node;
}

static void* free_list_get(size_t index) {
	FreeList* list = &free_lists[index];

	if (!list->root) {
		if (index == 8) {
			void* memory = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
			void* returned_block = memory;
			list->root = (Node*) ((uintptr_t) returned_block + 0x800);
			list->root->next = NULL;
			return returned_block;
		}
		else {
			void* blocks = pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
			void* list_blocks = (void*) ((uintptr_t) blocks + index_to_size(index));
			list->root = (Node*) list_blocks;
			list_blocks = (void*) ((uintptr_t) list_blocks + index_to_size(index));
			size_t remaining_blocks = 0x1000 / index_to_size(index) - 2;
			Node* node = list->root;
			for (size_t i = 0; i < remaining_blocks; ++i) {
				node->next = (Node*) list_blocks;
				node->next->next = NULL;
				list_blocks = (void*) ((uintptr_t) list_blocks + index_to_size(index));
				node = node->next;
			}
			return blocks;
		}
	}
	else {
		Node* node = list->root;
		list->root = list->root->next;
		return node;
	}
}

void* malloc(size_t size) {
	size_t index = get_size_index(size);
	if (index == 9) {
		void* mem = pmalloc((size - 1 + 0x1000) / 0x1000, MEMORY_ALLOC_TYPE_NORMAL);
		return mem;
	}
	else {
		void* ptr = free_list_get(index);
		return ptr;
	}
}

void free(void* ptr, size_t size) {
	size_t index = get_size_index(size);
	if (index == 9) {
		pfree(ptr, (size - 1 + 0x1000) / 0x1000);
	}
	else free_list_insert(index, ptr);
}