#include "unwind.h"
#include <stdint.h>
#include "stdio.h"

typedef struct StackFrame {
	struct StackFrame* rbp;
	uint64_t rip;
} StackFrame;

void unwind() {
	StackFrame* stack_frame;
	__asm__("mov %0, rbp" : "=r"(stack_frame));
	printf("stack trace:\n");
	while (stack_frame) {
		printf("0x%h\n", stack_frame->rip);
		stack_frame = stack_frame->rbp;
	}
}