#pragma once
#include <stdint.h>

typedef struct {
	uint8_t length;
	uint8_t type;
	uint16_t release_num;
	uint8_t device_class;
	uint8_t device_subclass;
	uint8_t protocol;
	uint8_t max_packet_size;
	uint16_t vendor_id;
	uint16_t product_id;
	uint16_t device_rel;
	uint8_t manufacturer;
	uint8_t product;
	uint8_t serial_num;
	uint8_t configs;
} DeviceDescriptor;

typedef struct {
	uint8_t size;
	uint8_t type;
	uint16_t lang_ids[];
} StringLanguageDescriptor;

typedef struct {
	uint8_t size;
	uint8_t type;
	uint16_t string[];
} StringDescriptor;

#define ENDPOINT_ADDR_NUM_MASK (0b1111)
#define ENDPOINT_ADDR_IN (1 << 7)
#define ENDPOINT_ATTR_TRANSFER_TYPE_MASK (0b11)
#define ENDPOINT_TRANSFER_TYPE_CONTROL (0)
#define ENDPOINT_TRANSFER_TYPE_ISO (1)
#define ENDPOINT_TRANSFER_TYPE_BULK (0b10)
#define ENDPOINT_TRANSFER_TYPE_INT (0b11)
#define ENDPOINT_ATTR_ISO_SYNC_TYPE_NO_SYNC (0 << 2)
#define ENDPOINT_ATTR_ISO_SYNC_TYPE_ASYNC (1 << 2)
#define ENDPOINT_ATTR_ISO_SYNC_TYPE_ADAPTIVE (0b10 << 2)
#define ENDPOINT_ATTR_ISO_SYNC_TYPE_SYNC (0b11 << 2)
#define ENDPOINT_ATTR_ISO_USAGE_DATA_ENDPOINT (0 << 4)
#define ENDPOINT_ATTR_ISO_USAGE_FEEDBACK_ENDPOINT (1 << 4)
#define ENDPOINT_ATTR_ISO_USAGE_EXPLICIT_FEEDBACK (0b10 << 4)

typedef struct {
	uint8_t length;
	uint8_t type;
	uint8_t address;
	uint8_t attributes;
} __attribute__((packed)) EndpointDescriptor;

typedef struct {
	uint8_t length;
	uint8_t type;
	uint8_t interface_num;
	uint8_t alternate_set;
	uint8_t endpoint_count;
	uint8_t class_code;
	uint8_t subclass;
	uint8_t protocol;
	uint8_t interface_str_index;
	EndpointDescriptor endpoints[];
} __attribute__((packed)) InterfaceDescriptor;

#define CONF_ATTR_REMOTE_WAKEUP (1 << 5)
#define CONF_ATTR_SELF_POWERED (1 << 6)

typedef struct {
	uint8_t size;
	uint8_t type;
	uint16_t total_length;
	uint8_t num_interfaces;
	uint8_t config_val;
	uint8_t config_string;
	uint8_t attributes;
	uint8_t max_power;
	InterfaceDescriptor interfaces[];
} __attribute__((packed)) ConfigurationDescriptor;