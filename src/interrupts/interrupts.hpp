#pragma once

struct interrupt_frame;

__attribute__((interrupt)) void pageFaultHandler(interrupt_frame* frame);