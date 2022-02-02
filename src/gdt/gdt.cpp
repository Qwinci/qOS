#include "gdt.hpp"

#define F_PRESENT (1 << 7)
#define F_PRIVILEGE1 (1 << 5)
#define F_PRIVILEGE2 (1 << 6)
#define F_CODEDATA (1 << 4)
#define F_EXEC (1 << 3)
#define F_DIRECTION (1 << 2)
#define F_RW (1 << 1)

__attribute__((aligned(0x1000))) GDT gdt {
		{0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0x9A, 0xA0, 0},
		{0, 0, 0, 0x92, 0x20, 0},

		{0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0x9A | F_PRIVILEGE1 | F_PRIVILEGE2, 0xA0, 0},
		{0, 0, 0, 0x92 | F_PRIVILEGE1 | F_PRIVILEGE2, 0x20, 0},
};
