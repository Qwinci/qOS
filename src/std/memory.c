#include "memory.h"

void* memset(void* dest, int ch, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		*((unsigned char*) dest + i) = (unsigned char) ch;
	}
	return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		*((unsigned char*) dest + i) = *((unsigned char*) src + i);
	}
	return dest;
}

int memcmp(const void* lhs, const void* rhs, size_t count) {
	const unsigned char* lhs1 = (const unsigned char*) lhs;
	const unsigned char* rhs1 = (const unsigned char*) rhs;
	for (size_t i = 0; i < count; ++i, ++lhs1, ++rhs1) {
		if (*lhs1 < *rhs1) return -1;
		else if (*lhs1 > *rhs1) return 1;
	}
	return 0;
}