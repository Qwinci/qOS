#include "bootInfo.hpp"
#include "interrupts/idt.hpp"
#include "interrupts/pic.hpp"
#include "paging/memory.hpp"
#include "console/console.hpp"
#include "drivers/ps2/keyboard.hpp"
#include "console/renderer.hpp"
#include "scheduling/pit.hpp"
#include "scheduling/timer.hpp"
#include "interrupts/apic.hpp"
#include "acpi/acpi.hpp"

[[noreturn]] void kernelStart(BootInfo bootInfo) {
	initializeRendering(bootInfo);
	initializeMemory(bootInfo);
	initializeInterrupts();
	//auto madt = findTable(static_cast<RSDP*>(bootInfo.rsdp), "APIC");
	//initializeAPIC(static_cast<MADT*>(madt));

	initializePIC();
	initializeKeyboard();
	initializePIT();
	auto madt = findTable(static_cast<RSDP*>(bootInfo.rsdp), "APIC");
	initializeAPIC(static_cast<MADT*>(madt));

	//auto mcfg = findTable(static_cast<RSDP*>(bootInfo.rsdp), "MCFG");
	//enumeratePCI(static_cast<MCFG*>(mcfg));
	//globalRenderer << "PCI enumeration done" << std::endl;

	while (true) {
		asm("hlt");
	}
}