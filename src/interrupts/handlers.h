#pragma once
#include <stdint.h>

typedef struct InterruptFrame InterruptFrame;

__attribute__((interrupt)) void pagefault_handler(InterruptFrame* interrupt_frame, uint64_t error);
__attribute__((interrupt)) void keyboard_interrupt(InterruptFrame* interrupt_frame);