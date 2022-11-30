#include "drivers/usb.h"
#include "stdio.h"
#include "interrupts/idt.h"

[[gnu::interrupt]] void usb_interrupt(InterruptFrame* frame);

void initialize_usb_xhci(PCIDeviceHeader0* header) {
	uintptr_t bar = pci_map_bar(&header->BAR0, &header->BAR1);
	header->reserved1 = 0;
	header->reserved2 = 0;
	header->reserved3 = 0;

	if (!usb_int_initialized) {
		const uint8_t* index = get_free_interrupt_index();
		if (!index) {
			printf("usb xhci: error: all interrupt vectors are in use\n");
			return;
		}
		usb_int = *index;
		usb_int_initialized = true;

		register_interrupt(usb_int, usb_interrupt, INTERRUPT_TYPE_INTERRUPT);
	}

	PciMsiCapability* msi = pci_get_msi_cap0(header);
	PciMsiXCapability* msi_x = pci_get_msi_x_cap0(header);
	if (!msi && !msi_x) {
		printf("usb xhci: error: msi/msi-x is not supported on this controller\n");
		return;
	}

	if (msi_x) {
		printf("bir: %u64, offset: %h\n", msi_x->bir, msi_x->table_offset);
	}

	printf("XHCI INITIALIZING\n");
}

[[gnu::interrupt]] void usb_interrupt(InterruptFrame* frame) {

}