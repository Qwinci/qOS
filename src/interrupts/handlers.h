#pragma once
#include <stdint.h>

typedef struct InterruptFrame InterruptFrame;

__attribute__((interrupt)) void double_fault_handler(InterruptFrame* interrupt_frame);
__attribute__((interrupt)) void page_fault_handler(InterruptFrame* interrupt_frame, uint64_t error);
__attribute__((interrupt)) void general_protection_fault_handler(InterruptFrame* interrupt_frame, uint64_t error);
__attribute__((interrupt)) void keyboard_interrupt(InterruptFrame* interrupt_frame);
__attribute__((interrupt)) void sb16_interrupt(InterruptFrame* interrupt_frame);