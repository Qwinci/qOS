#include "memory.hpp"

extern "C" void* memset(void* ptr, int value, size_t sum) {
	for (size_t i = 0; i < sum; ++i) {
		*(static_cast<unsigned char*>(ptr) + i) = value;
	}
	return ptr;
}

extern "C" void* memcpy(void* destination, const void* source, size_t num) {
	for (size_t i = 0; i < num; ++i) {
		static_cast<unsigned char*>(destination)[i] = static_cast<const unsigned char*>(source)[i];
	}
	return destination;
}