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

__attribute__((interrupt)) void pageFaultHandler(interrupt_frame* frame, uint64_t error);
__attribute__((interrupt)) void gpFaultHandler(interrupt_frame* frame);