#include "memory.h"

void* memset(void* dst, int ch, size_t count) {
	for (size_t i = 0; i < count; ++i, ++dst) {
		*(unsigned char*) dst = (unsigned char) ch;
	}
	return dst;
}