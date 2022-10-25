#pragma once
#include <stdint.h>

void sched_task(void (*task)(uint8_t apic_id));
void sched_task_all(void (*task)(uint8_t apic_id));