#include "idt.hpp"
#include "../paging/pageFrameAllocator.hpp"
#include "interrupts.hpp"

#define INTERRUPT_GATE 0b10001110
#define TRAP_GATE 0b10001111

IDTR idtr;

void registerInterrupt(uint8_t typeAttributes, uint8_t index, uint64_t handler) {
	auto* descriptor = reinterpret_cast<IDTDescriptor*>(idtr.offset + sizeof(IDTDescriptor) * index);
	descriptor->offset0 = handler & 0xFFFF;
	descriptor->offset1 = handler >> 16 & 0xFFFF;
	descriptor->offset2 = handler >> 32 & 0xFFFFFFFF;
	descriptor->selector = 0x08;
	descriptor->typeAttributes = typeAttributes;
}

void initializeInterrupts() {
	idtr.size = 0xFFF;
	idtr.offset = reinterpret_cast<uint64_t>(globalAllocator.allocatePage());

	registerInterrupt(INTERRUPT_GATE, 0xE, reinterpret_cast<uint64_t>(&pageFaultHandler));
	registerInterrupt(INTERRUPT_GATE, 0xD,reinterpret_cast<uint64_t>(&gpFaultHandler));

	asm("lidt %0" : : "m"(idtr));
	asm("sti");
}