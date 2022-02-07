#pragma once
#include <cstdint>

struct interrupt_frame
{
	uint16_t ip;
	uint16_t cs;
	uint16_t flags;
	uint16_t sp;
	uint16_t ss;
};

__attribute__((interrupt)) void divideByZeroHandler(interrupt_frame* frame);
__attribute__((interrupt)) void debugHandler(interrupt_frame* frame);
__attribute__((interrupt)) void nmiHandler(interrupt_frame* frame);
__attribute__((interrupt)) void breakpointHandler(interrupt_frame* frame);
__attribute__((interrupt)) void overflowExceptionHandler(interrupt_frame* frame);
__attribute__((interrupt)) void boundRangeExceededHandler(interrupt_frame* frame);
__attribute__((interrupt)) void invalidOpcodeHandler(interrupt_frame* frame);
__attribute__((interrupt)) void deviceNotAvailableHandler(interrupt_frame* frame);
__attribute__((interrupt)) void doubleFaultHandler(interrupt_frame* frame);
__attribute__((interrupt)) void invalidTSSHandler(interrupt_frame* frame);
__attribute__((interrupt)) void segmentNotPresentHandler(interrupt_frame* frame);
__attribute__((interrupt)) void stackSegmentFaultHandler(interrupt_frame* frame);
__attribute__((interrupt)) void gpFaultHandler(interrupt_frame* frame);
__attribute__((interrupt)) void pageFaultHandler(interrupt_frame* frame, uint64_t error);
__attribute__((interrupt)) void x87FloatingPointExceptionHandler(interrupt_frame* frame);
__attribute__((interrupt)) void alignmentCheckHandler(interrupt_frame* frame);
__attribute__((interrupt)) void machineCheckHandler(interrupt_frame* frame);
__attribute__((interrupt)) void simdFloatingPointExceptionHandler(interrupt_frame* frame);
__attribute__((interrupt)) void virtualizationExceptionHandler(interrupt_frame* frame);
__attribute__((interrupt)) void controlProtectionExceptionHandler(interrupt_frame* frame);
__attribute__((interrupt)) void hypervisorInjectionExceptionHandler(interrupt_frame* frame);
__attribute__((interrupt)) void vmmCommunicationExceptionHandler(interrupt_frame* frame);
__attribute__((interrupt)) void securityExceptionHandler(interrupt_frame* frame);
__attribute__((interrupt)) void unhandledInterruptHandler(interrupt_frame* frame);