#include "keyboard.hpp"
#include "../../interrupts/pic.hpp"
#include "../../utils/io.hpp"
#include "../../console/renderer.hpp"
#include "layouts/qwerty_europe.hpp"

__attribute__((interrupt)) void keyboardInterruptHandler(struct interrupt_frame* frame) {
	uint8_t scancode1 = in1(0x60);
	uint8_t scancode2 = 0;
	uint8_t status = in1(0x64);
	if (status & 0b1) {
		scancode2 = in1(0x64);
	}

	auto character = translateScancode(scancode1, scancode2);
	if (character != '\0') {
		globalRenderer << character;
	}
	endInterrupt(1);
}

void initializeKeyboard() {
	asm("cli");
	clearMask(1);
	asm("sti");
}