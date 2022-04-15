#pragma once
#include "../console/renderer.hpp"

extern uint64_t ms;

__attribute__((interrupt)) void tick(struct reg* regs);