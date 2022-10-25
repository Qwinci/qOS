#pragma once
#include <stddef.h>

void* malloc(size_t size);
void* lmalloc(size_t size);
void free(void* ptr, size_t size);
void lfree(void* ptr, size_t size);