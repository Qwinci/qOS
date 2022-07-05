#pragma once

typedef struct InterruptFrame InterruptFrame;

__attribute__((interrupt)) void pagefault_handler(InterruptFrame* interrupt_frame);