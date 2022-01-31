#pragma once
#include <cstddef>

extern "C" void* memset(void* ptr, int value, size_t sum);
extern "C" void* memcpy(void* destination, const void* source, size_t num);