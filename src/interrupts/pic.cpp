#include "pic.hpp"
#include <cstdint>
#include "../utils/io.hpp"

#define PIC_INIT 0x10
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20
#define PIC_8086 0x01

void setMask(uint8_t irq) {
	uint16_t port;
	uint8_t value;

	if (irq < 8) {
		port = PIC1_DATA;
	}
	else {
		port = PIC2_DATA;
		irq -= 8;
	}
	value = in1(port) | (1 << irq);
	out1(port, value);
}

void clearMask(uint8_t irq) {
	uint16_t port;
	uint8_t value;

	if (irq < 8) {
		port = PIC1_DATA;
	}
	else {
		port = PIC2_DATA;
		irq -= 8;
	}

	value = in1(port) & ~(1 << irq);
	out1(port, value);
}

__attribute__((no_caller_saved_registers)) void endInterrupt(uint8_t irq) {
	if (irq >= 8) {
		out1(PIC2_COMMAND, PIC_EOI);
	}
	out1(PIC1_COMMAND, PIC_EOI);
}

void initializePIC() {
	asm("cli");
	uint8_t pic1Mask = in1(PIC1_DATA);
	uint8_t pic2Mask = in1(PIC2_DATA);

	out1(PIC1_COMMAND, PIC_INIT);
	io_wait();
	out1(PIC2_COMMAND, PIC_INIT);
	io_wait();
	out1(PIC1_DATA, 0x20);
	io_wait();
	out1(PIC2_DATA, 0x28);
	io_wait();
	out1(PIC1_DATA, 4);
	io_wait();
	out1(PIC2_DATA, 2);
	io_wait();

	out1(PIC1_DATA, PIC_8086);
	io_wait();
	out1(PIC2_DATA, PIC_8086);
	io_wait();

	out1(PIC1_DATA, pic1Mask);
	out1(PIC2_DATA, pic2Mask);
	asm("sti");
}