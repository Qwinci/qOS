#pragma once
#include <stddef.h>

void* memset(void* dest, int ch, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* lhs, const void* rhs, size_t count);