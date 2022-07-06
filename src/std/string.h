#pragma once

static inline int strncmp(const char* str1, const char* str2, size_t count) {
	for (size_t i = 0; i < count; ++str1, ++str2, ++i) {
		if (!*str1 || !*str2) return *str1 ? 1 : -1;
		if (*str1 > *str2) return 1;
		else if (*str1 < *str2) return -1;
	}
	return 0;
}