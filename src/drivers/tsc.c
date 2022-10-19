#include "tsc.h"
#include "hpet.h"
#include "utils/cpuid.h"
#include "stdio.h"

extern bool hpet_initialized;

static uint64_t read_clock() {
	uint32_t low, high;
	__asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
	return (uint64_t) low | (uint64_t) high << 32;
}