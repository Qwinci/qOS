#include "stdio.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "boot_info.h"
#include <stdbool.h>
#include "string.h"
#include "timers/timers.h"
#include <stdatomic.h>
#include "smp/spinlock.h"

typedef struct {
	Framebuffer* fb;
	uint32_t fg;
	uint32_t bg;
	uint32_t col;
	uint32_t line;
} State;

static State state;

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t header_size;
	uint32_t flags;
	uint32_t num_glyph;
	uint32_t bytes_per_glyph;
	uint32_t height;
	uint32_t width;
} Psf2Font;

static Psf2Font* font;
static Framebuffer framebuffer;

void initialize_printf(Framebuffer* fb, uintptr_t font_start) {
	font = (Psf2Font*) font_start;
	if (font->magic != 0x864ab572) font = NULL;
	state.fb = fb;
	state.fg = 0x00FF00;
	state.bg = 0;
	framebuffer = *fb;
}

__attribute__((no_caller_saved_registers)) void set_fg_color(uint32_t color) {
	state.fg = color;
}

__attribute__((no_caller_saved_registers)) void set_bg_color(uint32_t color) {
	state.bg = color;
}

__attribute__((no_caller_saved_registers)) void clear_screen(uint32_t color) {
	for (uint64_t y = 0; y < state.fb->height; ++y) {
		for (uint64_t x = 0; x < state.fb->width; ++x) {
			*((uint32_t*) (state.fb->address + state.fb->pitch * y + state.fb->bpp / 8 * x)) = color;
		}
	}
	state.line = 0;
	state.col = 0;
}

static inline void put_char(char character, uint32_t col, uint32_t line) {
	if (col * font->width + font->width > state.fb->width ||
		line * font->height > state.fb->height) return;
	uint8_t* current_line = (uint8_t*) font + font->header_size + character * font->bytes_per_glyph;
	for (uint32_t y = 0; y < font->height; ++y) {
		for (uint32_t x = 0; x < font->width; ++x) {
			*(uint32_t*) (state.fb->address + (line * font->height + y) * state.fb->pitch + (col * font->width + x) * state.fb->bpp / 8) =
					*current_line & 1 << (7 - x) ? state.fg : state.bg;
		}
		++current_line;
	}
}

static inline void put_string(const char* str) {
	for (; *str; ++str) {
		if (state.col * font->width + font->width > state.fb->width) {
			state.col = 0;
			++state.line;
		}
		if (state.line * font->height + font->height > state.fb->height) {
			// todo scrolling
			clear_screen(0);
			state.line = 0;
			state.col = 0;
			//break;
		}
		put_char(*str, state.col++, state.line);
	}
}

static spinlock lock = false;

void printf(const char* fmt, ...) {
	spinlock_lock(&lock);
	va_list valist;
	va_start(valist, fmt);
	for (; *fmt; ++fmt) {
		if (*fmt == '%' && *(fmt + 1) != 0) {
			++fmt;
			if (*fmt == 'd' || strncmp(fmt, "i32", 3) == 0) {
				if (*fmt != 'd') fmt += 2;
				int32_t value = va_arg(valist, int32_t);

				char string[21];
				string[20] = 0;
				int16_t i = 19;
				bool is_negative = false;
				if (value < 0) {
					is_negative = true;
					value *= -1;
				}
				else if (value == 0) string[i--] = '0';

				while (value > 0) {
					string[i--] = (char) ('0' + value % 10);
					value /= 10;
				}

				if (is_negative) string[i--] = '-';

				put_string(string + i + 1);
			}
			else if (strncmp(fmt, "i64", 3) == 0) {
				fmt += 2;
				int64_t value = va_arg(valist, int64_t);

				char string[21];
				string[20] = 0;
				int16_t i = 19;
				bool is_negative = false;
				if (value < 0) {
					is_negative = true;
					value *= -1;
				}
				else if (value == 0) string[i--] = '0';

				while (value > 0) {
					string[i--] = (char) ('0' + value % 10);
					value /= 10;
				}

				if (is_negative) string[i--] = '-';

				put_string(string + i + 1);
			}
			else if (strncmp(fmt, "u64", 3) == 0 || strncmp(fmt, "u32", 3) == 0
					|| strncmp(fmt, "u16", 3) == 0 || strncmp(fmt, "u8", 2) == 0) {
				if (strncmp(fmt, "u8", 2) == 0) ++fmt;
				else fmt += 2;
				uint64_t value = va_arg(valist, uint64_t);

				char string[21];
				string[20] = 0;
				int16_t i = 19;
				if (value == 0) string[i--] = '0';
				while (value > 0) {
					string[i--] = (char) ('0' + value % 10);
					value /= 10;
				}

				put_string(string + i + 1);
			}
			else if (*fmt == 'h') {
				uintptr_t value = va_arg(valist, uintptr_t);

				char string[21];
				string[20] = 0;
				int16_t i = 19;
				if (value == 0) string[i--] = '0';

				while (value > 0) {
					string[i--] = (char) (value % 16 < 10 ? '0' + value % 16 : 'A' + value % 16 - 10);
					value /= 16;
				}

				put_string(string + i + 1);
			}
			else if (*fmt == 's') {
				const char* string = va_arg(valist, const char*);
				put_string(string);
			}
			else if (*fmt == 'c') {
				char string[2] = {va_arg(valist, int32_t), 0};
				put_string(string);
			}
		}
		else if (*fmt == '\n') {
			state.col = 0;
			++state.line;
		}
		else if (*fmt == '\t') {
			if (state.col % 4) state.col += 4 - state.col % 4;
			else state.col += 4;
		}
		else {
			if (state.col * font->width + font->width > state.fb->width) {
				state.col = 0;
				++state.line;
			}
			if (state.line * font->height + font->height > state.fb->height) {
				// todo scrolling
				clear_screen(0);
				state.line = 0;
				state.col = 0;
				//break;
			}
			put_char(*fmt, state.col++, state.line);
		}
	}

	va_end(valist);
	spinlock_unlock(&lock);
}