#include "hpet.h"
#include <stdint.h>
#include "acpi/acpi.h"
#include <stdbool.h>
#include "utils/mem.h"

typedef struct {
	uint8_t address_space_id;
	uint8_t register_bit_width;
	uint8_t register_bit_offset;
	uint8_t reserved;
	uint64_t address;
} __attribute__((packed)) AddressStructure;

#define LEGACY_REPLACEMENT_IRQ_ROUTING_CAP (1 << 15)
#define COUNTER_SIZE_CAP (1 << 13)
#define NUM_OF_COMPARATORS (0b11111 << 8)

typedef struct {
	SDTHeader header;
	uint32_t event_timer_block_id;
	AddressStructure base_address;
	uint8_t hpet_number;
	uint16_t minimum_tick;
	uint8_t page_protection;
} HPETHeader;

static uintptr_t base_address = 0;

static inline void write_register_64(uint32_t reg, uint64_t value) {
	*(volatile uint64_t*) (base_address + reg) = value;
}

static inline uint64_t read_register_64(uint32_t reg) {
	return *(volatile uint64_t*) (base_address + reg);
}

#define REG_CAP 0
#define REG_CONF 0x10
#define REG_INT_STATUS 0x20
#define REG_MAIN_COUNTER 0xF0
#define REG_TIMER_CONF(n) (0x100 + n * 0x20)
#define REG_TIMER_COMP(n) (0x108 + n * 0x20)
#define REG_TIMER_FSB_CONF(n) (0x110 + n * 0x20)

#define CONF_LEGACY_MAPPING_ENABLED (1 << 1)
#define CONF_ENABLE (1 << 0)

static uint32_t nanoseconds_in_tick = 0;

bool hpet_initialized = false;

bool initialize_hpet(void* rsdp) {
	HPETHeader* header = (HPETHeader*) locate_acpi_table(rsdp, "HPET");
	if (!header) return false;

	base_address = to_virt(header->base_address.address);

	uint64_t cap = read_register_64(REG_CAP);
	if ((cap & 1 << 13) == 0) {
		return false;
	}

	uint32_t femtoseconds_in_tick = cap >> 32;
	nanoseconds_in_tick = femtoseconds_in_tick / 1000000;

	uint64_t conf = read_register_64(REG_CONF);
	conf &= ~CONF_LEGACY_MAPPING_ENABLED;
	conf |= CONF_ENABLE;
	write_register_64(REG_CONF, conf);

	hpet_initialized = true;

	return true;
}

void hpet_sleep(uint32_t us) {
	uint64_t ticks = (uint64_t) us * 1000 / nanoseconds_in_tick;
	uint64_t value = read_register_64(REG_MAIN_COUNTER) + ticks;
	while (read_register_64(REG_MAIN_COUNTER) < value) __builtin_ia32_pause();
}