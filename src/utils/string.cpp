#include "string.hpp"

bool strcmp(const char* str1, const char* str2) {
	for (int i = 0; str1[i] != 0 && str2[i] != 0; ++i) {
		if (str1[i] != str2[i]) return false;
	}
	return true;
}

bool strncmp(const char* str1, const char* str2, size_t length) {
	for (int i = 0; str1[i] != 0 && str2[i] != 0 && i < length; ++i) {
		if (str1[i] != str2[i]) return false;
	}
	return true;
}