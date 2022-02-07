#include "keyboard.hpp"
#include "../../interrupts/pic.hpp"
#include "../../utils/io.hpp"
#include "../../console/renderer.hpp"

__attribute__((interrupt)) void keyboardInterruptHandler(struct interrupt_frame* frame) {
	uint8_t scanCode = in1(0x60);
	globalRenderer << "Scancode: " << scanCode << std::endl;
	endInterrupt(1);
}