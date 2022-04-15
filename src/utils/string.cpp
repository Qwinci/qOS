#include "string.hpp"

int strcmp(const char* lhs, const char* rhs) {
	for (size_t i = 0; lhs[i] != 0 && rhs[i] != 0; ++i) {
		if (lhs[i] < rhs[i]) return -1;
		else if (lhs[i] > rhs[i]) return 1;
	}
	return 0;
}

int strncmp(const char* lhs, const char* rhs, size_t length) {
	for (size_t i = 0; lhs[i] != 0 && rhs[i] != 0 && i < length; ++i) {
		if (lhs[i] < rhs[i]) return -1;
		else if (lhs[i] > rhs[i]) return 1;
	}
	return 0;
}