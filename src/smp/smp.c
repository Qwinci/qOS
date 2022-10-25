#include "smp.h"
#include "stdio.h"
#include <stddef.h>

typedef struct {
	_Atomic(void (*)(uint8_t apic_id)) ptr;
} Task;

static Task cpus[16] = {};
static uint8_t cpu_count = 0;

void sched_task(void (*task)(uint8_t apic_id)) {
	while (1) {
		for (uint8_t i = 0; i < cpu_count; ++i) {
			if (!cpus[i].ptr) {
				cpus[i].ptr = task;
				return;
			}
		}
		__asm__ volatile("pause" : : : "memory");
	}
}

void sched_task_all(void (*task)(uint8_t apic_id)) {
	for (uint8_t i = 0; i < cpu_count; ++i) {
		while (cpus[i].ptr) __asm__ volatile("pause" : : : "memory");
		cpus[i].ptr = task;
	}
}

_Noreturn void ap_entry(uint8_t apic_id) {
	++cpu_count;
	while (1) {
		if (cpus[apic_id - 1].ptr) {
			cpus[apic_id - 1].ptr(apic_id);
			cpus[apic_id - 1].ptr = NULL;
		}
		__asm__ volatile("pause" : : : "memory");
	}
}