#include "hpet.h"
#include <stdint.h>
#include "acpi/acpi.h"
#include <stdbool.h>
#include "stdio.h"
#include "interrupts/idt.h"
#include "apic.h"

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

__attribute__((interrupt)) void timer0_interrupt(InterruptFrame* frame);

static uint32_t nanoseconds_in_tick = 0;

bool hpet_initialized = false;
static uint8_t hpet_int = 0;

bool initialize_hpet(void* rsdp) {
	HPETHeader* header = (HPETHeader*) locate_acpi_table(rsdp, "HPET");
	if (!header) return false;

	base_address = header->base_address.address + 0xFFFF800000000000;

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

	const uint8_t* hpet_interrupt = get_free_interrupt_index();
	if (!hpet_interrupt) {
		printf("hpet: error: failed to allocate interrupt vector\n");
		return false;
	}
	hpet_int = *hpet_interrupt;
	register_interrupt(hpet_int, timer0_interrupt, INTERRUPT_TYPE_INTERRUPT);

	if (!hpet_reselect_irq()) return false;

	hpet_initialized = true;

	return true;
}

bool hpet_reselect_irq() {
	uint64_t timer0_conf = read_register_64(REG_TIMER_CONF(0));
	uint32_t timer0_supported_irqs = timer0_conf >> 32;

	bool supported_irq_found = false;
	for (int16_t i = 23; i >= 0; --i) {
		if (timer0_supported_irqs & 1 << i && io_apic_is_entry_free(i)) {
			// irq and non periodic mode
			timer0_conf |= i << 9 | 1 << 2;
			supported_irq_found = true;

			IoApicRedirectionEntry entry = {
					.vector = hpet_int,
					.destination = bsp_apic_id
			};

			register_io_apic_redirection_entry(i, entry);
			break;
		}
	}

	if (supported_irq_found) write_register_64(REG_TIMER_CONF(0), timer0_conf);

	return supported_irq_found;
}

static volatile bool sleeped = false;

__attribute__((interrupt)) void timer0_interrupt(InterruptFrame* frame) {
	sleeped = true;
	lapic_write(LAPIC_REG_EOI, 0);
}

void hpet_sleep(uint32_t us) {
	sleeped = false;
	uint64_t ticks = (uint64_t) us * 1000 / nanoseconds_in_tick;
	uint64_t value = read_register_64(REG_MAIN_COUNTER) + ticks;
	write_register_64(REG_TIMER_COMP(0), value);
	while (!sleeped) __builtin_ia32_pause();
}