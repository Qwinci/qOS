#include "handlers.h"
#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "drivers/apic.h"
#include "drivers/sb16.h"
#include "utils/unwind.h"
#include "utils/io.h"

typedef struct InterruptFrame {
	uint16_t ip;
	uint16_t cs;
	uint16_t flags;
	uint16_t sp;
	uint16_t ss;
} InterruptFrame;

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

static bool force_reboot = false;

__attribute__((interrupt)) void double_fault_handler(InterruptFrame* interrupt_frame) {
	if (force_reboot) *(volatile uint8_t*) 0 = 0;

	printf("double fault\n");
	printf("IP: 0x%h\n", 0xffffffff80000000 + interrupt_frame->ip);
	unwind();

	while (true) __asm__("hlt");
}

__attribute__((interrupt)) void general_protection_fault_handler(InterruptFrame* interrupt_frame, uint64_t error) {
	printf("general protection fault\n");
	printf("IP: 0x%h\n", 0xffffffff80000000 + interrupt_frame->ip);
	unwind();

	while (true) __asm__("hlt");
}

__attribute__((interrupt)) void page_fault_handler(InterruptFrame* interrupt_frame, uint64_t error) {
	if (force_reboot) *(volatile uint8_t*) 0 = 0;

	uint64_t address;
	__asm__("mov %0, cr2" : "=r"(address));
	printf("pagefault at address 0x%h\n", address);
	printf("IP: 0x%h\n", 0xffffffff80000000 + interrupt_frame->ip);
	unwind();

	while (true) __asm__("hlt");
}

__attribute__((interrupt)) void keyboard_interrupt(InterruptFrame* interrupt_frame) {
	printf("keyboard\n");

	uint8_t byte1 = in1(PS2_DATA_PORT);
	io_wait();
	uint8_t status = in1(PS2_STATUS_PORT);
	if (status & 1) in1(PS2_DATA_PORT);

	lapic_write(LAPIC_REG_EOI, 0);

	force_reboot = true;

	*(volatile uint8_t*) 0 = 0;
}

__attribute__((interrupt)) void sb16_interrupt(InterruptFrame* interrupt_frame) {
	//sb16_update();
	lapic_write(LAPIC_REG_EOI, 0);
}