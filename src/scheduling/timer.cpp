#include "timer.hpp"
#include "../interrupts/pic.hpp"

uint64_t ms = 0;

__attribute__((interrupt)) void tick(struct reg* regs) {
	ms += 1;
	endInterrupt(0);
}