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

void* memmove(void* dest, const void* src, size_t count) {
	const unsigned char* src_ptr;
	unsigned char* dest_ptr;
	if (dest < src) {
		src_ptr = (const unsigned char*) src;
		dest_ptr = (unsigned char*) dest;
		for (size_t i = 0; i < count; ++i) {
			*dest_ptr++ = *src_ptr++;
		}
	}
	else if (dest > src) {
		src_ptr = (const unsigned char*) src + count;
		dest_ptr = (unsigned char*) dest + count;
		for (size_t i = 0; i < count; ++i) {
			*dest_ptr-- = *src_ptr--;
		}
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