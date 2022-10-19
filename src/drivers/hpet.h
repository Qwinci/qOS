#pragma once
#include <stdbool.h>
#include <stdint.h>

bool initialize_hpet(void* rsdp);
void hpet_sleep(uint32_t us);