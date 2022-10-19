#include "memory.h"

void* memset(void* dest, int ch, size_t count) {
	for (size_t i = 0; i < count; ++i, ++dest) {
		*(unsigned char*) dest = (unsigned char) ch;
	}
	return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
	for (size_t i = 0; i < count; ++i, ++src, ++dest) {
		*(unsigned char*) dest = *(unsigned char*) src;
	}
	return dest;
}