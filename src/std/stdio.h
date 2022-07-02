#pragma once
#include <stdint.h>

typedef struct Framebuffer Framebuffer;

void initialize_printf(Framebuffer* framebuffer, uintptr_t font_start);
void printf(const char* fmt, ...);