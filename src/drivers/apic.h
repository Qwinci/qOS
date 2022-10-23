#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum : uint32_t {
	LAPIC_REG_ID = 0x20,
	LAPIC_REG_VERSION = 0x30,
	LAPIC_REG_TASK_PRIORITY = 0x80,
	LAPIC_REG_ARBITRATION_PRIORITY = 0x90,
	LAPIC_REG_PROCESSOR_PRIORITY = 0xA0,
	LAPIC_REG_EOI = 0xB0,
	LAPIC_REG_REMOTE_READ = 0xC0,
	LAPIC_REG_LOGICAL_DEST = 0xD0,
	LAPIC_REG_DEST_FORMAT = 0xE0,
	LAPIC_REG_SPURIOUS_INTERRUPT_VEC = 0xF0,
	LAPIC_REG_IN_SERVICE_BASE = 0x100,
	LAPIC_REG_TRIGGER_MODE_BASE = 0x180,
	LAPIC_REG_INTERRUPT_REQUEST_BASE = 0x200,
	LAPIC_REG_ERROR_STATUS = 0x280,
	LAPIC_REG_LVT_CORRECTED_MCE_INTERRUPT = 0x2F0,
	LAPIC_REG_INTERRUPT_CMD_BASE = 0x300,
	LAPIC_REG_LVT_TIMER = 0x320,
	LAPIC_REG_LVT_THERMAL_SENSOR = 0x330,
	LAPIC_REG_LVT_PERF_MON_COUNTERS = 0x340,
	LAPIC_REG_LVT_LINT0 = 0x350,
	LAPIC_REG_LVT_LINT1 = 0x360,
	LAPIC_REG_LVT_ERROR = 0x370,
	LAPIC_REG_INIT_COUNT = 0x380,
	LAPIC_REG_CURR_COUNT = 0x390,
	LAPIC_REG_DIV_CONF = 0x3E0
} LapicReg;

void initialize_apic(void* rsdp);
void lapic_write(LapicReg reg, uint32_t value);
uint32_t lapic_read(LapicReg reg);

typedef enum : uint8_t {
	IO_APIC_DELIVERY_MODE_FIXED = 0,
	IO_APIC_DELIVERY_MODE_LOW_PRIORITY = 0b1,
	IO_APIC_DELIVERY_MODE_SMI = 0b10,
	IO_APIC_DELIVERY_MODE_NMI = 0b100,
	IO_APIC_DELIVERY_MODE_INIT = 0b101,
	IO_APIC_DELIVERY_MODE_EXT_INT = 0b111
} IoApicDeliveryMode;

typedef enum : uint8_t {
	IO_APIC_DEST_MODE_PHYSICAL = 0,
	IO_APIC_DEST_MODE_LOGICAL = 1
} IoApicDestinationMode;

typedef enum : uint8_t {
	IO_APIC_PIN_POLARITY_ACTIVE_HIGH = 0,
	IO_APIC_PIN_POLARITY_ACTIVE_LOW = 1,
} IoApicPinPolarity;

typedef enum : uint8_t {
	IO_APIC_TRIGGER_MODE_EDGE = 0,
	IO_APIC_TRIGGER_MODE_LEVEL = 1
} IoApicTriggerMode;

typedef struct {
	uint8_t vector;
	IoApicDeliveryMode delivery_mode;
	IoApicDestinationMode destination_mode;
	IoApicPinPolarity pin_polarity;
	IoApicTriggerMode trigger_mode;
	uint8_t mask;
	uint8_t destination;
} IoApicRedirectionEntry;

void register_io_apic_redirection_entry(uint8_t io_apic_irq, IoApicRedirectionEntry entry);
bool io_apic_is_entry_free(uint8_t io_apic_irq);

extern uint8_t bsp_apic_id;