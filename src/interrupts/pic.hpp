#pragma once
#include <cstdint>

void initializePIC();
__attribute__((no_caller_saved_registers)) void endInterrupt(uint8_t irq);
void setMask(uint8_t irq);
void clearMask(uint8_t irq);