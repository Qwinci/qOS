#include "idt.hpp"
#include "../paging/pageFrameAllocator.hpp"
#include "interrupts.hpp"
#include "../utils/memory.hpp"
#include "drivers/ps2/keyboard.hpp"

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
	memset(reinterpret_cast<void*>(idtr.offset), 0, 0x1000);

	registerInterrupt(INTERRUPT_GATE, 0x0, reinterpret_cast<uint64_t>(&divideByZeroHandler));
	registerInterrupt(INTERRUPT_GATE, 0x1, reinterpret_cast<uint64_t>(&debugHandler));
	registerInterrupt(INTERRUPT_GATE, 0x2, reinterpret_cast<uint64_t>(&nmiHandler));
	registerInterrupt(INTERRUPT_GATE, 0x3, reinterpret_cast<uint64_t>(&breakpointHandler));
	registerInterrupt(INTERRUPT_GATE, 0x4, reinterpret_cast<uint64_t>(&overflowExceptionHandler));
	registerInterrupt(INTERRUPT_GATE, 0x5, reinterpret_cast<uint64_t>(&boundRangeExceededHandler));
	registerInterrupt(INTERRUPT_GATE, 0x6, reinterpret_cast<uint64_t>(&invalidOpcodeHandler));
	registerInterrupt(INTERRUPT_GATE, 0x7, reinterpret_cast<uint64_t>(&deviceNotAvailableHandler));
	registerInterrupt(INTERRUPT_GATE, 0x8, reinterpret_cast<uint64_t>(&doubleFaultHandler));
	registerInterrupt(INTERRUPT_GATE, 0xA, reinterpret_cast<uint64_t>(&invalidTSSHandler));
	registerInterrupt(INTERRUPT_GATE, 0xB, reinterpret_cast<uint64_t>(&segmentNotPresentHandler));
	registerInterrupt(INTERRUPT_GATE, 0xC, reinterpret_cast<uint64_t>(&stackSegmentFaultHandler));
	registerInterrupt(INTERRUPT_GATE, 0xD, reinterpret_cast<uint64_t>(&gpFaultHandler));
	registerInterrupt(INTERRUPT_GATE, 0xE, reinterpret_cast<uint64_t>(&pageFaultHandler));
	registerInterrupt(INTERRUPT_GATE, 0x10, reinterpret_cast<uint64_t>(&x87FloatingPointExceptionHandler));
	registerInterrupt(INTERRUPT_GATE, 0x11, reinterpret_cast<uint64_t>(&alignmentCheckHandler));
	registerInterrupt(INTERRUPT_GATE, 0x12, reinterpret_cast<uint64_t>(&machineCheckHandler));
	registerInterrupt(INTERRUPT_GATE, 0x13, reinterpret_cast<uint64_t>(&simdFloatingPointExceptionHandler));
	registerInterrupt(INTERRUPT_GATE, 0x14, reinterpret_cast<uint64_t>(&virtualizationExceptionHandler));
	registerInterrupt(INTERRUPT_GATE, 0x15, reinterpret_cast<uint64_t>(&controlProtectionExceptionHandler));
	registerInterrupt(INTERRUPT_GATE, 0x1C, reinterpret_cast<uint64_t>(&hypervisorInjectionExceptionHandler));
	registerInterrupt(INTERRUPT_GATE, 0x1D, reinterpret_cast<uint64_t>(&vmmCommunicationExceptionHandler));
	registerInterrupt(INTERRUPT_GATE, 0x1E, reinterpret_cast<uint64_t>(&securityExceptionHandler));

	registerInterrupt(INTERRUPT_GATE, 0x21, reinterpret_cast<uint64_t>(&keyboardInterruptHandler));

	asm("lidt %0" : : "m"(idtr));
	asm("sti");
}