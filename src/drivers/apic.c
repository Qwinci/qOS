#include "apic.h"
#include "acpi/acpi.h"
#include "stdio.h"
#include <stddef.h>
#include "paging/memory.h"
#include "utils/io.h"
#include "memory.h"
#include <stdbool.h>

static void write_io_apic_register(uintptr_t base, uint8_t reg, uint32_t value) {
	*(volatile uint32_t*) (base) = reg; // IOREGSEL
	*(volatile uint32_t*) (base + 0x10) = value;
}

static uint32_t read_io_apic_register(uintptr_t base, uint8_t reg) {
	*(volatile uint32_t*) (base) = reg; // IOREGSEL
	return *(volatile uint32_t*) (base + 0x10);
}

static uintptr_t lapic_address = 0;

void lapic_write(LapicReg reg, uint32_t value) {
	*(volatile uint32_t*) (lapic_address + (uint32_t) reg) = value;
}

uint32_t lapic_read(LapicReg reg) {
	return *(volatile uint32_t*) (lapic_address + (uint32_t) reg);
}

typedef struct {
	uintptr_t base;
	uint32_t interrupt_base;
	uint8_t irq_count;
	uint32_t used_irqs;
} IoApic;

typedef struct {
	uint8_t enabled;
	uint8_t global_base;
	uint8_t active_low;
	uint8_t level_triggered;
} IrqOverride;

static IoApic io_apic_list[16];
static uint8_t io_apic_count = 0;
static IrqOverride irq_overrides[16] = {};

typedef struct {
	uint8_t io_apic_index;
	uint8_t io_apic_entry;
	uint8_t active_low;
	uint8_t level_triggered;
} IsaIrqRedirectionEntryInfo;

static inline IsaIrqRedirectionEntryInfo get_io_apic_info_for_isa_irq(uint8_t irq) {
	IrqOverride override = irq_overrides[irq];
	if (override.enabled) {
		for (uint8_t i = 0; i < io_apic_count; ++i) {
			if (io_apic_list[i].interrupt_base <= override.global_base) {
				if (override.global_base - io_apic_list[i].interrupt_base < io_apic_list[i].irq_count) {
					return (IsaIrqRedirectionEntryInfo) {
						.io_apic_index = i,
						.io_apic_entry = override.global_base - io_apic_list[i].interrupt_base};
				}
			}
		}
		__builtin_unreachable();
	}
	for (uint8_t i = 0; i < io_apic_count; ++i) {
		if (io_apic_list[i].interrupt_base == 0) {
			return (IsaIrqRedirectionEntryInfo) {io_apic_list[i].base, irq};
		}
	}
	__builtin_unreachable();
}

void write_io_apic_isa_redirection_entry(uint8_t irq, IoApicRedirectionEntry entry) {
	IsaIrqRedirectionEntryInfo info = get_io_apic_info_for_isa_irq(irq);

	uint64_t e = entry.vector;
	e |= (uint64_t) entry.delivery_mode << 8;
	e |= (uint64_t) entry.destination_mode << 11;
	e |= (uint64_t) entry.pin_polarity << 13;
	e |= (uint64_t) info.active_low << 13;
	e |= (uint64_t) entry.trigger_mode << 15;
	e |= (uint64_t) info.level_triggered << 15;
	e |= (uint64_t) entry.mask << 16;
	e |= (uint64_t) entry.destination << 56;

	write_io_apic_register(io_apic_list[info.io_apic_index].base, 0x10 + info.io_apic_entry * 2, e & 0xFFFFFFFF);
	write_io_apic_register(io_apic_list[info.io_apic_index].base, 0x10 + info.io_apic_entry * 2 + 1, e >> 32);
	io_apic_list[info.io_apic_index].used_irqs |= 1 << info.io_apic_entry;
}

void register_io_apic_redirection_entry(uint8_t io_apic_irq, IoApicRedirectionEntry entry) {
	uint64_t e = entry.vector;
	e |= (uint64_t) entry.delivery_mode << 8;
	e |= (uint64_t) entry.destination_mode << 11;
	e |= (uint64_t) entry.pin_polarity << 13;
	e |= (uint64_t) entry.trigger_mode << 15;
	e |= (uint64_t) entry.mask << 16;
	e |= (uint64_t) entry.destination << 56;

	for (uint8_t i = 0; i < io_apic_count; ++i) {
		IoApic* io_apic = &io_apic_list[i];
		if (io_apic->interrupt_base <= io_apic_irq) {
			if (io_apic_irq - io_apic->interrupt_base < io_apic->irq_count) {
				write_io_apic_register(io_apic->base, 0x10 + (io_apic_irq - io_apic->interrupt_base) * 2, e & 0xFFFFFFFF);
				write_io_apic_register(io_apic->base, 0x10 + (io_apic_irq - io_apic->interrupt_base) * 2 + 1, e >> 32);
				io_apic->used_irqs |= 1 << io_apic_irq;
				return;
			}
		}
	}

	__builtin_unreachable();
}

bool io_apic_is_entry_free(uint8_t io_apic_irq) {
	for (uint8_t i = 0; i < io_apic_count; ++i) {
		IoApic* io_apic = &io_apic_list[i];
		if (io_apic->interrupt_base <= io_apic_irq) {
			if (io_apic_irq - io_apic->interrupt_base < io_apic->irq_count) {
				if (io_apic->used_irqs & 1 << (io_apic_irq - io_apic->interrupt_base)) return false;
				else return true;
			}
		}
	}
	return false;
}

uint8_t* io_apic_get_free_irq() {
	static uint8_t irq;
	for (uint8_t i = 0; i < io_apic_count; ++i) {
		IoApic* io_apic = &io_apic_list[i];
		if (io_apic->interrupt_base != 0) continue;
		for (uint8_t i2 = 0; i2 < io_apic->irq_count; ++i2) {
			if ((io_apic->used_irqs & 1 << i2) == 0) {
				irq = i2;
				return &irq;
			}
		}
	}
	return NULL;
}

extern char smp_trampoline_start[];
extern char smp_trampoline_end[];

static uint8_t ap_lapic_ids[16];
static uint8_t ap_lapic_count = 0;

extern _Noreturn void ap_entry(uint8_t apic_id);

void usleep(uint32_t us) {
	for (uint32_t i = 0; i < us; ++i) io_wait();
}

uint8_t bsp_apic_id = 0;

void initialize_apic(void* rsdp) {
	__asm__("mov al, 0xFF; out 0xA1, al; out 0x21, al;" : : : "al");
	void* madt = locate_acpi_table(rsdp, "APIC");

	if (!madt) {
		printf("failed to find madt table. falling back to pic\n");
		return;
	}

	SDTHeader* header = (SDTHeader*) madt;
	madt += sizeof(SDTHeader);

	uint64_t local_apic_address = to_virt(*(uint32_t*) madt);
	madt += 4;
	uint32_t flags = *(uint32_t*) madt;
	madt += 4;

	for (size_t i = (uintptr_t) header; i < (uintptr_t) header + header->length; i += 0x1000) {
		pmap(to_phys(i) & ~0xFFF, i & ~0xFFF, PAGEFLAG_PRESENT | PAGEFLAG_RW);
		prefresh(i & ~0xFFF);
	}

	uint8_t bsp_id;
	__asm__ __volatile__("mov eax, 1; cpuid; shr ebx, 24" : "=b"(bsp_id));

	bsp_apic_id = bsp_id;

	for (size_t i = 0; i < header->length - sizeof(SDTHeader) - 8;) {
		uint8_t type = *(uint8_t*) (madt + i);
		uint8_t length = *(uint8_t*) (madt + i + 1);
		i += 2;
		switch (type) { // NOLINT(hicpp-multiway-paths-covered)
			case 0: // processor local apic
			{
				uint8_t acpi_processor_id = *(uint8_t*) (madt + i);
				++i;
				uint8_t apic_id = *(uint8_t*) (madt + i);
				++i;
				uint32_t flags2 = *(uint32_t*) (madt + i);
				i += 4;

				if (flags2 & 1 || (flags & 1 << 1)) {
					if (apic_id != bsp_id) ap_lapic_ids[ap_lapic_count++] = apic_id;
				}
				break;
			}
			case 1: // io apic
			{
				uint8_t io_apic_id = *(uint8_t*) (madt + i);
				i += 2; // reserved
				uint32_t io_apic_address = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t global_system_interrupt_base = *(uint32_t*) (madt + i);
				i += 4;

				io_apic_list[io_apic_count++] = (IoApic)
						{.base = to_virt(io_apic_address),
						 .interrupt_base = global_system_interrupt_base,
						 .used_irqs = 0};
				break;
			}
			case 2: // io apic interrupt source override
			{
				uint8_t bus_source = *(uint8_t*) (madt + i);
				++i;
				uint8_t irq_source = *(uint8_t*) (madt + i);
				++i;
				uint32_t global_system_interrupt = *(uint32_t*) (madt + i);
				i += 4;
				uint16_t flags2 = *(uint16_t*) (madt + i);
				i += 2;

				uint8_t active_low = 0;
				uint8_t level_triggered = 0;
				if (flags2 & 2) active_low = 1;
				if (flags2 & 8) level_triggered = 1;

				irq_overrides[irq_source] = (IrqOverride) {
					.enabled = 1,
					.global_base = global_system_interrupt,
					.active_low = active_low,
					.level_triggered = level_triggered};
				break;
			}
			case 3: // io apic non maskable interrupt source
			{
				uint8_t nmi_source = *(uint8_t*) (madt + i);
				i += 2; // reserved
				uint16_t flags2 = *(uint16_t*) (madt + i);
				i += 2;
				uint32_t global_system_interrupt = *(uint32_t*) (madt + i);
				i += 4;
				break;
			}
			case 4: // local apic non maskable interrupts
			{
				uint8_t acpi_processor_id = *(uint8_t*) (madt + i);
				++i;
				uint16_t flags2 = *(uint16_t*) (madt + i);
				i += 2;
				uint8_t lint = *(uint8_t*) (madt + i);
				++i;
				break;
			}
			case 5: // local apic address override
			{
				// reserved
				i += 2;
				uint64_t address = *(uint64_t*) (madt + i);
				i += 8;
				local_apic_address = to_virt(address);
				break;
			}
			case 9: // processor local x2apic
			{
				// reserved
				i += 2;
				uint32_t processor_local_x2apic_id = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t flags2 = *(uint32_t*) (madt + i);
				i += 4;
				uint32_t acpi_id = *(uint32_t*) (madt + i);
				i += 4;
				break;
			}
		}
	}

	for (uint8_t i = 0; i < io_apic_count; ++i) {
		io_apic_list[i].irq_count = (read_io_apic_register(io_apic_list[i].base, 1) >> 16 & 0xFF) + 1;
	}

	lapic_address = local_apic_address;

	IoApicRedirectionEntry ps2_keyboard_entry = {
			.vector = 0x20,
			.destination = bsp_id
	};

	IoApicRedirectionEntry sb16_entry = {
			.vector = 0x21,
			.destination = bsp_id
	};

	write_io_apic_isa_redirection_entry(1, ps2_keyboard_entry);
	write_io_apic_isa_redirection_entry(5, sb16_entry);

	// enable lapic and set spurious interrupt vector to 0xFF
	lapic_write(LAPIC_REG_SPURIOUS_INTERRUPT_VEC, 0xFF | 0x100);
	lapic_write(LAPIC_REG_TASK_PRIORITY, 0);

	const uint32_t DEST_MODE_INIT = 5 << 8;
	const uint32_t DEST_MODE_SIPI = 6 << 8;
	const uint32_t DELIVERY_STATUS = 1 << 12;
	const uint32_t INIT_DEASSERT_CLEAR = 1 << 14;
	const uint32_t INIT_DEASSERT = 1 << 15;

	size_t trampoline_size = (size_t) (smp_trampoline_end - smp_trampoline_start);
	void* trampoline = (void*) to_virt(0x1000);
	uintptr_t trampoline_phys = to_phys((uintptr_t) trampoline);

	pmap(
			trampoline_phys,
			(uintptr_t) trampoline,
			PAGEFLAG_PRESENT | PAGEFLAG_RW);
	pmap(
			trampoline_phys,
			trampoline_phys,
			PAGEFLAG_PRESENT | PAGEFLAG_RW);

	memcpy(
			trampoline,
			smp_trampoline_start,
			trampoline_size);

	extern uint64_t* pml4;

	typedef struct {
		uint8_t flag;
		uint32_t page_table;
		uint8_t apic_id;
		uint64_t entry;
		uint64_t stack;
	} __attribute__((packed)) InitInfo;

	InitInfo* info = (InitInfo*) ((uintptr_t) trampoline + trampoline_size);
	info->flag = 0;
	info->page_table = (uint32_t) (uintptr_t) pml4;
	info->entry = (uint64_t) ap_entry;

	for (uint8_t a = 0; a < ap_lapic_count; ++a) {
		uint32_t value = DEST_MODE_INIT | INIT_DEASSERT_CLEAR;
		uint32_t ap = ap_lapic_ids[a];
		info->apic_id = ap;
		info->stack = (uint64_t) pmalloc(8, MEMORY_ALLOC_TYPE_NORMAL) + 0x8000;

		// init
		lapic_write(LAPIC_REG_INTERRUPT_CMD_BASE + 0x10, ap << 24);
		lapic_write(LAPIC_REG_INTERRUPT_CMD_BASE, value);
		while (lapic_read(LAPIC_REG_INTERRUPT_CMD_BASE) & DELIVERY_STATUS) __asm__ volatile("pause" : : : "memory");

		// init deassert
		value = DEST_MODE_INIT | INIT_DEASSERT;
		lapic_write(LAPIC_REG_INTERRUPT_CMD_BASE + 0x10, ap << 24);
		lapic_write(LAPIC_REG_INTERRUPT_CMD_BASE, value);
		while (lapic_read(LAPIC_REG_INTERRUPT_CMD_BASE) & DELIVERY_STATUS) __asm__ volatile("pause" : : : "memory");

		// wait 10ms
		usleep(1000 * 10);

		// startup ipi
		for (uint8_t i = 0; i < 2; ++i) {
			// starting at 0x1000
			value = 1 | DEST_MODE_SIPI | INIT_DEASSERT_CLEAR;
			lapic_write(LAPIC_REG_INTERRUPT_CMD_BASE + 0x10, ap << 24);
			lapic_write(LAPIC_REG_INTERRUPT_CMD_BASE, value);

			// wait 200us
			usleep(i == 0 ? 1000 : 1000 * 1000);

			while (lapic_read(LAPIC_REG_INTERRUPT_CMD_BASE) & DELIVERY_STATUS) __asm__ volatile("pause" : : : "memory");

			if (info->flag) {
				break;
			}
		}

		if (info->flag) {
			info->flag = 0;
		}
		else pfree((void*) (info->stack - 0x8000), 8);
	}
}