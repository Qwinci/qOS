#pragma once
#include <cstddef>

int strncmp(const char* lhs, const char* rhs, size_t length);
int strcmp(const char* lhs, const char* rhs);
static inline size_t strlen(const char* str) {
	size_t count = 0;
	while (*str++ != 0) {
		++count;
	}
	return count;
}