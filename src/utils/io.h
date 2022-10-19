#pragma once
#include <stdint.h>

static inline volatile uint8_t in1(uint16_t port) {
	uint8_t value;
	__asm__ volatile("in %0, %1" : "=a"(value) : "Nd"(port));
	return value;
}

static inline volatile uint16_t in2(uint16_t port) {
	uint16_t value;
	__asm__ volatile("in %0, %1" : "=a"(value) : "Nd"(port));
	return value;
}

static inline volatile uint32_t in4(uint16_t port) {
	uint32_t value;
	__asm__ volatile("in %0, %1" : "=a"(value) : "Nd"(port));
	return value;
}

static inline void out1(uint16_t port, uint8_t value) {
	__asm__ volatile("out %0, %1" : : "Nd"(port), "a"(value));
}

static inline void out2(uint16_t port, uint16_t value) {
	__asm__ volatile("out %0, %1" : : "Nd"(port), "a"(value));
}

static inline void out4(uint16_t port, uint32_t value) {
	__asm__ volatile("out %0, %1" : : "Nd"(port), "a"(value));
}

static inline void io_wait() {
	out1(0x80, 0);
}