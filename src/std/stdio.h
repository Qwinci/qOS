#pragma once
#include <stdint.h>

typedef struct Framebuffer Framebuffer;

static Framebuffer* framebuffer;

void initialize_printf(Framebuffer* framebuffer, uintptr_t font_start);
__attribute__((no_caller_saved_registers)) void printf(const char* fmt, ...);
__attribute__((no_caller_saved_registers)) void set_fg_color(uint32_t color);
__attribute__((no_caller_saved_registers)) void set_bg_color(uint32_t color);
__attribute__((no_caller_saved_registers)) void clear_screen(uint32_t color);