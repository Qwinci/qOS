#pragma once
#include <stdint.h>

typedef struct {
	uint32_t eax, ebx, ecx, edx;
} CpuidInfo;

static inline CpuidInfo cpuid(uint32_t eax, uint32_t ecx) {
	CpuidInfo cpuid;
	__asm__ volatile("cpuid" :
		"=a"(cpuid.eax),
		"=b"(cpuid.ebx),
		"=c"(cpuid.ecx),
		"=d"(cpuid.edx)
		: "0"(eax), "2"(ecx));

	return cpuid;
}