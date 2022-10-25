#include "timers.h"
#include "drivers/hpet.h"
#include "stdio.h"

extern bool hpet_initialized;

void initialize_timers(void* rsdp) {
	if (!initialize_hpet(rsdp)) printf("hpet initialization failed\n");
}

void udelay(uint64_t us) {
	if (!hpet_initialized) {
		printf("timer: error: no available timer\n");
		return;
	}

	hpet_sleep(us);
}