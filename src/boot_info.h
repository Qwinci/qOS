#include <stdint.h>
#include <stddef.h>

typedef struct Framebuffer {
	void* address;
	uint16_t bpp;
	uint64_t pitch;
	uint64_t width;
	uint64_t height;
} Framebuffer;

typedef enum : uint64_t {
	MEMORYTYPE_USABLE,
	MEMORYTYPE_ACPI_RECLAIMABLE,
	MEMORYTYPE_ACPI_NVS,
	MEMORYTYPE_BOOTLOADER_REACLAIMABLE,
	MEMORYTYPE_FRAMEBUFFER,
	MEMORYTYPE_UNKNOWN
} MemoryType;

typedef struct {
	uint64_t base;
	uint64_t size;
	MemoryType type;
} MemoryEntry;

typedef struct MemoryMap {
	MemoryEntry** entries;
	size_t entry_count;
} MemoryMap;

typedef struct BootInfo {
	Framebuffer framebuffer;
	MemoryMap memory_map;
	uintptr_t kernel_virtual_address;
	uintptr_t kernel_physical_address;
	void* rsdp;
	uintptr_t font_start;
} BootInfo;