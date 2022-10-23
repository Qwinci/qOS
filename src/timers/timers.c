#include "timers.h"
#include "drivers/hpet.h"
#include "stdio.h"

void initialize_timers(void* rsdp) {
	if (!initialize_hpet(rsdp)) printf("hpet initialization failed\n");
}