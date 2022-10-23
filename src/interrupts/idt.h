#pragma once
#include <stdint.h>

typedef enum {
	INTERRUPT_TYPE_INTERRUPT = 0x8E,
	INTERRUPT_TYPE_TRAP = 0x8F
} InterruptType;

typedef struct InterruptFrame InterruptFrame;

#define IRQ0 0x20

void initialize_interrupts();
void register_interrupt(uint8_t index, void* handler, InterruptType type);

#define NETWORK_CONTROLLER_0_INT 0xFE