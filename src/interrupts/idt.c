#include "idt.h"
#include <stdint.h>
#include "paging/memory.h"
#include "std/memory.h"
#include "handlers.h"
#include "gdt.h"
#include "stdio.h"

typedef struct {
	uint16_t size;
	uint64_t offset;
} __attribute__((packed)) IDTR;

typedef struct {
	uint16_t offset1;
	uint16_t selector;
	uint8_t ist;
	uint8_t type_attributes;
	uint16_t offset2;
	uint32_t offset3;
	uint32_t zero;
} InterruptDescriptor;

static IDTR idtr;

void register_interrupt(uint8_t index, void* handler, InterruptType type) {
	InterruptDescriptor* descriptor = (InterruptDescriptor*) (idtr.offset + index * sizeof(InterruptDescriptor));
	uintptr_t address = (uintptr_t) handler;
	descriptor->offset1 = address & 0xFFFF;
	address >>= 16;
	descriptor->offset2 = address & 0xFFFF;
	address >>= 16;
	descriptor->offset3 = address & 0xFFFFFFFF;
	descriptor->ist = 0;
	descriptor->selector = 0x8;
	descriptor->type_attributes = type;
	descriptor->zero = 0;
}

static uint8_t interrupts[256] = {};

void initialize_interrupts() {
	__asm__("cli");
	load_gdt();

	idtr.size = 0x1000 - 1;
	idtr.offset = (uint64_t) pmalloc(1, MEMORY_ALLOC_TYPE_NORMAL);
	memset((void*) idtr.offset, 0, 0x1000);

	register_interrupt(0x8, double_fault_handler, INTERRUPT_TYPE_TRAP);
	register_interrupt(0xD, general_protection_fault_handler, INTERRUPT_TYPE_TRAP);
	register_interrupt(0xE, page_fault_handler, INTERRUPT_TYPE_TRAP);
	register_interrupt(0x20, keyboard_interrupt, INTERRUPT_TYPE_INTERRUPT);
	register_interrupt(0x21, sb16_interrupt, INTERRUPT_TYPE_INTERRUPT);

	for (uint8_t i = 0; i < 0x30; ++i) {
		interrupts[i] = 1;
	}

	__asm__("lidt %0" : : "m"(idtr));
	__asm__("sti");
}

const uint8_t* get_free_interrupt_index() {
	static uint8_t interrupt;
	for (uint16_t i = 0; i < 256; ++i) {
		if (!interrupts[i]) {
			interrupt = i;
			return &interrupt;
		}
	}
	return NULL;
}