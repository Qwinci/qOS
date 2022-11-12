#pragma once
#include "pci.h"
#include <stdbool.h>
#include <stddef.h>

extern uint8_t usb_int;
extern bool usb_int_initialized;

void initialize_usb_uhci(PCIDeviceHeader0* header, PciDeviceInfo info);
void initialize_usb_ohci(PCIDeviceHeader0* header);
void initialize_usb_ehci(PCIDeviceHeader0* header);
void initialize_usb_xhci(PCIDeviceHeader0* header);

typedef enum {
	USB_STATUS_SUCCESS,
	USB_STATUS_ERROR,
	USB_STATUS_WAITING
} UsbStatus;

typedef enum {
	USB_PACKET_TYPE_
} UsbPacketType;

typedef struct {
	size_t size;
	UsbPacketType type;
	UsbStatus status;
} UsbPacketHeader;

typedef struct {
	UsbPacketHeader header;
} UsbPacket;

typedef struct {
	void (*send_packet)(uint8_t device, UsbPacket packet);
} Controller;

typedef struct {
	Controller* controller;
	uint8_t num;
} UsbDevice;