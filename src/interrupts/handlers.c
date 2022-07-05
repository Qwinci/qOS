#include "handlers.h"
#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"

typedef struct InterruptFrame {
	uint16_t ip;
	uint16_t cs;
	uint16_t flags;
	uint16_t sp;
	uint16_t ss;
} InterruptFrame;

__attribute__((interrupt)) void pagefault_handler(InterruptFrame* interrupt_frame) {
	uint64_t address;
	__asm__("mov %0, cr2" : "=r"(address));
	printf("pagefault at address 0x%h\n", address);
	printf("IP: 0x%h\n", 0xffffffff80000000 + interrupt_frame->ip);

	while (true) __asm__("hlt");
}