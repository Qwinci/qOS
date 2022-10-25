#pragma once
#include "pci.h"
#include <stdbool.h>

extern uint8_t usb_int;
extern bool usb_int_initialized;

void initialize_usb_uhci(PCIDeviceHeader0* header);
void initialize_usb_ohci(PCIDeviceHeader0* header);
void initialize_usb_ehci(PCIDeviceHeader0* header);
void initialize_usb_xhci(PCIDeviceHeader0* header);