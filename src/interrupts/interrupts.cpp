#include "interrupts.hpp"
#include "../console/renderer.hpp"
#include <cstdint>

__attribute__((interrupt)) void unhandledInterruptHandler(interrupt_frame* frame) {
	globalRenderer << "Unhandled interrupt" << std::endl;
	while (true) {
		asm("hlt");
	}
}

__attribute__((interrupt)) void pageFaultHandler(interrupt_frame* frame, uint64_t error) {
	globalRenderer.setColor(0xFF0000);
	uint64_t address;
	asm("mov %%cr2, %0" : "=r"(address));
	globalRenderer << "Pagefault caused by address 0x" << Mode::Hex << address << std::endl;
	globalRenderer << "IP: 0x" << 0xffffffff80000000 + frame->ip << std::endl;

	uint8_t present = error & 0b1;
	uint8_t write = (error & 0b10) >> 1;
	uint8_t user = (error & 0b100) >> 2;
	uint8_t resWrite = (error & 0b1000) >> 3;
	uint8_t insFetch = (error & 0b10000) >> 4;
	uint8_t protKey = (error & 0b100000) >> 5;
	uint8_t shadowStack = (error & 0b1000000) >> 6;
	uint8_t SGE = (error & 0b10000000) >> 7;

	globalRenderer << "Present\tWrite\tUser\tResWrite\tInsFetch\tProtKey\tShadowStack\tSGE" << std::endl;
	globalRenderer << present << "\t\t  " << write << "\t\t" << user << "\t   "
	<< resWrite << "\t\t   " << insFetch << "\t\t   " << protKey << "\t\t  " << shadowStack
	<<"\t\t\t  " << SGE << std::endl;

	while (true) {
		asm("hlt");
	}
}

__attribute__((interrupt)) void gpFaultHandler(interrupt_frame* frame) {
	globalRenderer << "General protection fault" << std::endl;

	while (true) {
		asm("hlt");
	}
}