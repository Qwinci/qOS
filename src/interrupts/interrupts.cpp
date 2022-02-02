#include "interrupts.hpp"
#include "../console/renderer.hpp"

__attribute__((interrupt)) void pageFaultHandler(interrupt_frame* frame) {
	globalRenderer.setColor(0xFF0000);
	globalRenderer << "PageFault" << std::endl;
	while (true) {
		asm("hlt");
	}
}