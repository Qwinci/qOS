#include "stdio.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "boot_info.h"
#include <stdbool.h>

static inline bool strncmp(const char* str1, const char* str2, size_t count) {
	for (size_t i = 0; i < count; ++str1, ++str2, ++i) {
		if (!*str1 || !*str2 || *str1 != *str2) return false;
	}
	return true;
}

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

void initialize_printf(Framebuffer* framebuffer, uintptr_t font_start) {
	font = (Psf2Font*) font_start;
	if (font->magic != 0x864ab572) font = NULL;
	state.fb = framebuffer;
	state.fg = 0x00FF00;
	state.bg = 0;
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

void printf(const char* fmt, ...) { // NOLINT(misc-no-recursion)
	va_list valist;
	va_start(valist, fmt);

	for (; *fmt; ++fmt) {
		if (*fmt == '%' && *(fmt + 1) != 0) {
			++fmt;
			if (*fmt == 'd' || strncmp(fmt, "i32", 3)) { // NOLINT(bugprone-suspicious-string-compare)
				if (*fmt != 'd') fmt += 2;
				int32_t value = va_arg(valist, int32_t);

				char string[21];
				string[20] = 0;
				uint8_t i = 19;
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

				printf(string + i + 1);
			}
			else if (strncmp(fmt, "i64", 3)) { // NOLINT(bugprone-suspicious-string-compare)
				fmt += 2;
				int64_t value = va_arg(valist, int64_t);

				char string[21];
				string[20] = 0;
				uint8_t i = 19;
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

				printf(string + i + 1);
			}
			else if (strncmp(fmt, "u64", 3) || strncmp(fmt, "u32", 3) // NOLINT(bugprone-suspicious-string-compare)
					|| strncmp(fmt, "u16", 3) || strncmp(fmt, "u8", 2)) { // NOLINT(bugprone-suspicious-string-compare)
				fmt += 2;
				uint64_t value = va_arg(valist, uint64_t);

				char string[21];
				string[20] = 0;
				uint8_t i = 19;
				if (value == 0) string[i--] = '0';

				while (value > 0) {
					string[i--] = (char) ('0' + value % 10);
					value /= 10;
				}

				printf(string + i + 1);
			}
			else if (*fmt == 'h') { // NOLINT(bugprone-suspicious-string-compare)
				uintptr_t value = va_arg(valist, uintptr_t);

				char string[21];
				string[20] = 0;
				uint8_t i = 19;
				if (value == 0) string[i--] = '0';

				while (value > 0) {
					string[i--] = (char) (value % 16 < 10 ? '0' + value % 16 : 'A' + value % 16 - 10);
					value /= 16;
				}

				printf(string + i + 1);
			}
			else if (*fmt == 's') {
				++fmt;
				const char* string = va_arg(valist, const char*);
				printf(string);
			}
			else if (*fmt == 'c') {
				++fmt;
				char string[2] = {va_arg(valist, int32_t), 0};
				printf(string);
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
			if (state.line * font->height > state.fb->height) break;
			put_char(*fmt, state.col++, state.line);
		}
	}

	va_end(valist);
}