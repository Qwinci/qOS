#pragma once
#include <cstdint>

static inline void out1(uint16_t port, uint8_t value) {
	asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}
__attribute__((no_caller_saved_registers)) static inline uint8_t in1(uint16_t port) {
	uint8_t value;
	asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}
static inline void out2(uint16_t port, uint16_t value) {
	asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}
static inline uint16_t in2(uint16_t port) {
	uint16_t value;
	asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}
static inline void out4(uint16_t port, uint32_t value) {
	asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}
static inline uint32_t in4(uint16_t port) {
	uint32_t value;
	asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline void io_wait() {
	out1(0x80, 0);
}