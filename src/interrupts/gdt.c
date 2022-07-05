#include "gdt.h"
#include <stdint.h>

typedef struct {
	uint16_t size;
	uint64_t offset;
} __attribute__((packed)) GDTDescriptor;

typedef struct {
	uint16_t limit0;
	uint16_t base0;
	uint8_t base1;
	uint8_t access_byte;
	uint8_t limit1_flags;
	uint8_t base2;
} __attribute__((packed)) GDTEntry;

typedef struct {
	GDTEntry null;
	GDTEntry kernel_code;
	GDTEntry kernel_data;
	GDTEntry user_code;
	GDTEntry user_data;
} __attribute__((packed)) GDT;

#define AC_PRESENT 1
#define AC_PRIVILEGE_USER 3 << 5
#define AC_CODE_DATA 1 << 4
#define AC_EXEC 1 << 3
#define AC_DIRECTION 1 << 2
#define AC_RW 1 << 1

#define FLAG_LONG (1 << 1)
#define FLAG_DB (1 << 2)

__attribute__((aligned(0x1000))) GDT gdt = {
		.null = {0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0x9A, 0xA << 4, 0},
		{0, 0, 0, 0x92, 0xC << 4, 0},
		{0, 0, 0, 0xFA, 0xA << 4, 0},
		{0, 0, 0, 0xF2, 0xC << 4, 0}
};

extern void load_gdt_asm(GDTDescriptor* gdt);

void load_gdt() {
	GDTDescriptor gdtr = {.size = sizeof(GDT) - 1, (uint64_t) &gdt};
	load_gdt_asm(&gdtr);
}