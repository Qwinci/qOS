#pragma once

typedef struct {
	void* entry;
} MemoryPool;

static inline void* pool_get(MemoryPool* pool) {
	void* entry = pool->entry;
	if (entry) pool->entry = *(void**) entry;
	return entry;
}

static inline void pool_free(MemoryPool* pool, void* entry) {
	void* old_entry = pool->entry;
	pool->entry = entry;
	*(void**) entry = old_entry;
}