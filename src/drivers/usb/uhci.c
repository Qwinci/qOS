#include "drivers/usb.h"
#include "paging/memory.h"
#include "stdio.h"
#include <stdbool.h>
#include "interrupts/idt.h"
#include "drivers/apic.h"
#include "utils/io.h"
#include "timers/timers.h"
#include "memory.h"
#include "paging/malloc.h"
#include "lai/helpers/pci.h"
#include "drivers/hpet.h"
#include "common.h"

static bool io_space = false;
static uint64_t bar;

#define CMD_RUN (1 << 0)
/// Host Controller Reset
#define CMD_HCRESET (1 << 1)
/// Global Reset
#define CMD_GRESET (1 << 2)
/// Enter Global Suspend Mode
#define CMD_EGSM (1 << 3)
/// Force Global Resume
#define CMD_FGR (1 << 4)
/// Software Debug
#define CMD_SWDBG (1 << 5)
/// Configure Flag
#define CMD_CF (1 << 6)
/// Max Packet (1 = 64 bytes, 0 = 32 bytes)
#define CMD_MAXP (1 << 7)

/// USB Interrupt
#define STATUS_USBINT (1 << 0)
/// USB Error Interrupt
#define STATUS_USB_ERR_INT (1 << 1)
/// Resume Detect
#define STATUS_RESUME_DETECT (1 << 2)
/// Host System Error
#define STATUS_HOST_SYS_ERR (1 << 3)
/// Host Controller Process Error
#define STATUS_HOST_CON_PROCESS_ERR (1 << 4)
/// Controller Halted
#define STATUS_CON_HALTED (1 << 5)

#define INT_TIMEOUT_CRC_ENABLE (1 << 0)
#define INT_RESUME_ENABLE (1 << 1)
/// Interrupt on Complete Enable
#define INT_IOC_ENABLE (1 << 2)
#define INT_SHORT_PACKET_ENABLE (1 << 3)

#define FRAME_LIST_CUR_NUM_MASK (0x3FF)

#define START_OF_FRAME_SOF_TIMING_MASK (0b1111111)

#define PORT_SC_CUR_CONNECT_STATUS (1 << 0)
#define PORT_SC_CONNECT_STATUS_CHANGE (1 << 1)
#define PORT_SC_ENABLE (1 << 2)
#define PORT_SC_ENABLE_CHANGE (1 << 3)
#define PORT_SC_LINE_STATUS_MASK (0b11 << 4)
#define PORT_SC_LINE_STATUS_SHIFT 4
#define PORT_SC_RESUME_DETECT (1 << 6)
#define PORT_SC_RESERVED (1 << 7)
#define PORT_SC_LOW_SPEED (1 << 8)
#define PORT_SC_RESET (1 << 9)
#define PORT_SC_SUSPEND (1 << 12)

#define USB_RESET_DELAY 10

typedef enum : uint32_t {
	REG_USBCMD = 0,
	REG_USBSTS = 0x2,
	REG_USBINTR = 0x4,
	REG_FRNUM = 0x6,
	REG_FRBASEADD = 0x8,
	/// Start of Frame Modify
	REG_SOFMOD = 0xC,
	REG_PORTSC0 = 0x10,
	REG_PORTSC1 = 0x12
} Register;

static void write16(Register reg, uint16_t value) {
	if (io_space) out2(bar + reg, value);
	else *(volatile uint16_t*) (bar + reg) = value;
}

static void write32(Register reg, uint32_t value) {
	if (io_space) out4(bar + reg, value);
	else *(volatile uint32_t*) (bar + reg) = value;
}

static uint16_t read16(Register reg) {
	if (io_space) return in2(bar + reg);
	else return *(volatile uint16_t*) (bar + reg);
}

static uint32_t read32(Register reg) {
	if (io_space) return in4(bar + reg);
	else return *(volatile uint32_t*) (bar + reg);
}

__attribute__((interrupt)) static void usb_interrupt(InterruptFrame* interrupt_frame);
static inline bool reset_port(uint8_t port_offset);
static inline bool check_port_present(uint8_t port_offset);
static inline DeviceDescriptor* setup_device(uint8_t port_offset);

typedef uint32_t FrameEntry;

#define FRAME_ENTRY_TERMINATE (1 << 0)
#define FRAME_ENTRY_QUEUE (1 << 1)

typedef struct {
	uint32_t link_ptr;

	union {
		struct {
			uint16_t act_len;
			uint8_t status;
			uint8_t flags;
		};
		uint32_t status_flags;
	} d1;
	uint32_t d2;
	uint32_t buffer_ptr;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t reserved4;
} UsbUhciTransferDescriptor;

typedef struct {
	uint32_t head_link_ptr;
	uint32_t element_link_ptr;
	uint32_t parent_ptr;
	uint32_t reserved;
} UsbUhciQueueDescriptor;

#define TD_PTR_FLAG_TERMINATE (1 << 0)
#define TD_PTR_FLAG_QUEUE (1 << 1)
#define TD_PTR_FLAG_DEPTH_FIRST (1 << 2)

#define TD_ACT_LEN_MASK (0x7FF)

#define TD_D2_PID_MASK (0xFF)
#define TD_D2_DEV_ADDR_MASK (0x7F)
#define TD_D2_DEV_ADDR_SHIFT (8)
#define TD_D2_ENDPOINT_MASK (0b1111)
#define TD_D2_ENDPOINT_SHIFT (15)
#define TD_D2_DATA_TOGGLE (1 << 19)
#define TD_D2_MAXLEN_MASK (0b11111111111)
#define TD_D2_MAXLEN_SHIFT (21)

#define TD_STATUS_BITSTUFF_ERR (1 << 1)
#define TD_STATUS_CRC_TIMEOUT_ERR (1 << 2)
#define TD_STATUS_NAK_RECEIVED (1 << 3)
#define TD_STATUS_BABBLE_DETECTED (1 << 4)
#define TD_STATUS_DATA_BUFFER_ERR (1 << 5)
#define TD_STATUS_STALLED (1 << 6)
#define TD_STATUS_ACTIVE (1 << 7)

/// Interrupt on Complete
#define TD_FLAG_IOC (1 << 0)
#define TD_FLAG_ISO (1 << 1)
/// Low Speed Device
#define TD_FLAG_LS (1 << 2)
#define TD_FLAG_CERR_MASK (0b11 << 3)
#define TD_FLAG_CERR_SHIFT (3)
/// Short Packet Detect
#define TD_FLAG_SPD (1 << 5)

static uint32_t port_count = 0;
static UsbUhciQueueDescriptor* queues;
#define FIRST_PERIODIC_QUEUE 8

#define PID_IN 0x69
#define PID_OUT 0xE1
#define PID_SETUP 0x2D

typedef enum {
	QUEUE_1MS = FIRST_PERIODIC_QUEUE,
	QUEUE_2MS,
	QUEUE_4MS,
	QUEUE_8MS,
	QUEUE_16MS,
	QUEUE_32MS,
	QUEUE_64MS,
	QUEUE_128MS,
	QUEUE_CONTROL = 7,
	QUEUE_BULK = 6
} Queue;

static UsbUhciTransferDescriptor* td_chain_starts[3 * 16] = {};
static UsbUhciTransferDescriptor* td_chain_ends[3 * 16] = {};
static uint8_t current_chain_indices[16] = {};
static FrameEntry* frame_list;

static inline void insert_td_chain(Queue queue, UsbUhciTransferDescriptor* td, UsbUhciTransferDescriptor* end) {
	uint8_t current_chain = current_chain_indices[queue];
	uint8_t next_chain = queue * 3 + (current_chain > 0 ? current_chain - 1 : 2);
	if (!td_chain_starts[next_chain]) {
		td_chain_starts[next_chain] = td;
		td_chain_ends[next_chain] = end;
		return;
	}
	td_chain_ends[next_chain]->link_ptr = to_phys((uintptr_t) end) | TD_PTR_FLAG_DEPTH_FIRST;
	end->d1.flags |= TD_FLAG_IOC;
}

static inline void force_process_current_chain(Queue queue) {
	uint8_t current_chain = current_chain_indices[queue];
	if (queues[queue].element_link_ptr & TD_PTR_FLAG_TERMINATE) {
		for (uint8_t i = 0; i < 3; ++i) {
			uint8_t tmp = current_chain + i;
			uint8_t index = tmp < 3 ? tmp : tmp - 3;
			UsbUhciTransferDescriptor* td = td_chain_starts[queue * 3 + index];

			if (td) {
				queues[queue].element_link_ptr = to_phys((uintptr_t) td);
				td_chain_starts[queue * 3 + index] = NULL;
			}
		}
	}
}

static inline void poll_td_active(volatile UsbUhciTransferDescriptor* td) {
	while (td->d1.status & TD_STATUS_ACTIVE) __asm__ volatile("hlt" : : : "memory");
}

static inline void link_packet(UsbUhciTransferDescriptor* packet1, UsbUhciTransferDescriptor* packet2) {
	packet1->link_ptr = (uint32_t) to_phys((uintptr_t) packet2) | TD_PTR_FLAG_DEPTH_FIRST;
}

static inline UsbUhciTransferDescriptor* create_packet(
		void* buffer,
		size_t size,
		bool end,
		uint8_t pid,
		bool toggle,
		uint8_t device,
		uint8_t endpoint) {
	UsbUhciTransferDescriptor* packet = lmalloc(sizeof(UsbUhciTransferDescriptor));
	memset(packet, 0, sizeof(*packet));

	packet->link_ptr = TD_PTR_FLAG_TERMINATE;
	if (buffer) packet->buffer_ptr = (uint32_t) to_phys((uintptr_t) buffer);
	packet->d1.flags = 3 << TD_FLAG_CERR_SHIFT | (end ? TD_FLAG_IOC : 0);
	packet->d1.status = TD_STATUS_ACTIVE;
	packet->d2 = ((size - 1) & TD_D2_MAXLEN_MASK) << TD_D2_MAXLEN_SHIFT | pid | (toggle ? TD_D2_DATA_TOGGLE : 0);
	packet->d2 |= (uint32_t) (device & TD_D2_DEV_ADDR_MASK) << TD_D2_DEV_ADDR_SHIFT;
	packet->d2 |= (uint32_t) (endpoint & TD_D2_ENDPOINT_MASK) << TD_D2_ENDPOINT_SHIFT;
	return packet;
}

typedef struct {
	uint8_t request_type;
	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t length;
} DeviceRequest;

typedef enum : uint8_t {
	DIR_HOST_TO_DEVICE = 0,
	DIR_DEVICE_TO_HOST = 1 << 7
} RequestDirection;

typedef enum : uint8_t {
	TYPE_STANDARD = 0,
	TYPE_CLASS = 1 << 5,
	TYPE_VENDOR = 2 << 5
} RequestType;

typedef enum : uint8_t {
	REC_DEVICE = 0,
	REC_INTERFACE = 1,
	REC_ENDPOINT = 2,
	REC_OTHER = 3
} RequestRecipient;

typedef enum : uint8_t {
	REQ_GET_STATUS = 0,
	REQ_CLEAR_FEATURE = 1,
	REG_SET_FEATURE = 3,
	REQ_SET_ADDRESS = 5,
	REQ_GET_DESCRIPTOR = 6,
	REQ_SET_DESCRIPTOR = 7,
	REQ_GET_CONF = 8,
	REQ_SET_CONF = 9
} Request;

#define DATA_FEATURE_SELECTOR(selector) selector
#define DATA_DEVICE_ADDR(addr) a
#define DATA_DESC(type, index) type

typedef enum : uint8_t {
	DESC_TYPE_DEVICE = 1,
} DescType;

const inline uint16_t data_feature_selector(uint16_t selector) {
	return selector;
}

const inline uint16_t data_device_addr(uint16_t addr) {
	return addr;
}

const inline uint16_t data_desc(DescType type, uint8_t index) {
	return (uint16_t) type << 8 | index;
}

const inline uint16_t data_conf_value(uint16_t value) {
	return value;
}

static inline DeviceRequest* create_request(
		Request request,
		RequestDirection direction,
		RequestType type,
		RequestRecipient recipient,
		uint16_t specific_data,
		uint16_t index,
		uint16_t length) {

	DeviceRequest* req = lmalloc(sizeof(DeviceRequest));
	*req = (DeviceRequest) {
			.request_type = direction | type | recipient,
			.request = request,
			.value = specific_data,
			.index = index,
			.length = length
	};

	return req;
}

void initialize_usb_uhci(PCIDeviceHeader0* header, PciDeviceInfo info) {
	uint32_t size;
	if (header->BAR4 & 1) {
		bar = header->BAR4 & 0xFFFFFFFC;
		io_space = true;
		uint32_t original_bar = header->BAR4;
		header->BAR4 = 0xFFFFFFFF;
		size = ~(header->BAR4 & 0xFFFFFFFC) + 1;
		header->BAR4 = original_bar;
	}
	else if ((header->BAR4 >> 1 & 0b11) == 2) {
		bar = (header->BAR4 & 0xFFFFFFF0) + ((uint64_t) (header->BAR5 & 0xFFFFFFFF) << 32);
		uint32_t original_bar = header->BAR4;
		header->BAR4 = 0xFFFFFFFF;
		size = ~(header->BAR4 & 0xFFFFFFF0) + 1;
		header->BAR4 = original_bar;
	}
	else {
		bar = header->BAR4 & 0xFFFFFFF0;
		uint32_t original_bar = header->BAR4;
		header->BAR4 = 0xFFFFFFFF;
		size = ~(header->BAR4 & 0xFFFFFFF0) + 1;
		header->BAR4 = original_bar;
	}

	if (!io_space && header->BAR4 & 1 << 3) {
		for (uint32_t i = 0; i < size; i += 0x1000) {
			pmap(
					bar + i,
					to_virt(bar + i),
					PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_WRITE_THROUGH);
		}
		bar += 0xFFFF800000000000;
	}
	else if (!io_space) {
		for (uint32_t i = 0; i < size; i += 0x1000) {
			pmap(
					bar + i,
					to_virt(bar + i),
					PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_CACHE_DISABLE);
		}
		bar = to_virt(bar);
	}

	printf("initializing usb uhci controller\n");

	header->reserved1 = 0;
	header->reserved2 = 0;
	header->reserved3 = 0;

	if (!usb_int_initialized) {
		const uint8_t* index = get_free_interrupt_index();
		if (!index) {
			printf("usb uhci: error: all interrupt vectors are in use\n");
			return;
		}
		usb_int = *index;
		usb_int_initialized = true;

		register_interrupt(usb_int, usb_interrupt, INTERRUPT_TYPE_INTERRUPT);
	}

	PciMsiCapability* msi = pci_get_msi_cap0(header);
	if (msi) {
		msi->msg_data = usb_int; // interrupt vector
		// address + cpu
		msi->msg_addr = 0xFEE00000 | 0 << 12;
		msi->msg_control |= MSI_CTRL_ENABLE;
		if ((msi->msg_control & MSI_CTRL_MM_CAPABLE) != MSI_CTRL_MM_1_CAPABLE) {
			printf("usb uhci: requesting more than 1 interrupt, allocating only one.\n");
		}
		msi->msg_control |= MSI_CTRL_MM_1_ENABLE;
	}
	else {
		acpi_resource_t result;
		lai_pci_route_pin(&result, info.segment, info.bus, info.device, info.function, header->interrupt_pin);

		IoApicTriggerMode trigger_mode = (result.irq_flags &
				(ACPI_SMALL_IRQ_EDGE_TRIGGERED | ACPI_EXTENDED_IRQ_EDGE_TRIGGERED))
						? IO_APIC_TRIGGER_MODE_EDGE : IO_APIC_TRIGGER_MODE_LEVEL;
		IoApicPinPolarity polarity = (result.irq_flags &
				(ACPI_SMALL_IRQ_ACTIVE_LOW | ACPI_EXTENDED_IRQ_ACTIVE_LOW))
						? IO_APIC_PIN_POLARITY_ACTIVE_LOW : IO_APIC_PIN_POLARITY_ACTIVE_HIGH;

		IoApicRedirectionEntry entry = {
				.vector = usb_int,
				.destination = bsp_apic_id,
				.trigger_mode = trigger_mode,
				.pin_polarity = polarity
		};

		if (!io_apic_is_entry_free(result.base)) {
			printf("io apic entry %u8 is used\n", result.base);
		}

		register_io_apic_redirection_entry(result.base, entry);
	}

	if (io_space) header->header.command |= PCI_CMD_BUS_MASTER | PCI_CMD_IO_SPACE;
	else header->header.command |= PCI_CMD_BUS_MASTER | PCI_CMD_MEM_SPACE;

	const uint32_t USB_LEGSUP = 0xC0;
	const uint32_t USB_SBRN = 0x60;

	// disable legacy support
	*(volatile uint32_t*) ((uintptr_t) header + USB_LEGSUP) = 0x8F00;

	uint32_t revision = *(volatile uint8_t*) ((uintptr_t) header + USB_SBRN);
	if (revision != 0 && revision != 0x10) {
		printf("usb uhci: error: invalid usb revision %h\n", revision);
		return;
	}

	PciPmCapability* pm = pci_get_pm_cap0(header);
	if (pm) {
		if ((pm->state & PCI_PM_MASK) == PCI_PM_D3) pm->state &= PCI_PM_MASK;
	}

	// do a global reset
	write16(REG_USBCMD, CMD_GRESET);
	udelay(USB_RESET_DELAY * 1000);
	write16(REG_USBCMD, 0);

	if (read16(REG_PORTSC0) == 0x8000) {
		printf("usb uhci: error: big endian controllers are not supported\n");
		return;
	}

	if (read16(REG_USBCMD) != 0) {
		printf("usb uhci: error: usbcmd is not zero after global reset\n");
		return;
	}

	if ((read16(REG_USBSTS) & STATUS_CON_HALTED) == 0) {
		printf("usb uhci: error: usbsts is not on halted state after global reset\n");
		return;
	}

	// clear status
	write16(REG_USBSTS, 0xFFFF);

	if ((read16(REG_SOFMOD) & START_OF_FRAME_SOF_TIMING_MASK) != 0x40) {
		printf("usb uhci: error: invalid sof timing\n");
		return;
	}

	write16(REG_USBCMD, CMD_HCRESET);
	udelay(42 * 1000);
	if (read16(REG_USBCMD) & CMD_HCRESET) {
		printf("usb uhci: error: controller doesn't reset properly\n");
		return;
	}

	write16(REG_USBINTR, INT_SHORT_PACKET_ENABLE | INT_IOC_ENABLE | INT_RESUME_ENABLE | INT_TIMEOUT_CRC_ENABLE);

	write16(REG_FRNUM, 0);

	frame_list = (FrameEntry*) pmalloc(1, MEMORY_ALLOC_TYPE_LOW);
	if (!frame_list) {
		printf("usb uhci: error: failed to allocate uhci frame list\n");
		return;
	}
	memset(frame_list, 0, 0x1000);

	// resv0 resv1 resv2 resv3 resv4 iso bulk control 1ms 2ms 4ms 8ms 16ms 32ms 64ms 128ms
	queues = lmalloc(16 * sizeof(UsbUhciQueueDescriptor));
	printf("16 * sizeof(UsbUhciQueueDescriptor) = %u64\n", 16 * sizeof(UsbUhciQueueDescriptor));
	if (!queues) {
		printf("usb uhci: error: failed to allocate queue descriptors\n");
		return;
	}
	memset(queues, 0, 16 * sizeof(UsbUhciQueueDescriptor));

	queues[6].head_link_ptr = TD_PTR_FLAG_TERMINATE;
	queues[6].element_link_ptr = TD_PTR_FLAG_TERMINATE;
	for (uint8_t queue = 7; queue < 16; ++queue) {
		queues[queue].head_link_ptr = (uint32_t) to_phys((uintptr_t) &queues[queue - 1]);
		queues[queue].head_link_ptr |= TD_PTR_FLAG_QUEUE;
		queues[queue].element_link_ptr = TD_PTR_FLAG_TERMINATE;
	}

	// 1ms 2ms 1ms 4ms 1ms 2ms 1ms 8ms 1ms 2ms 1ms 4ms 1ms 2ms 1ms 16ms
	for (uint16_t i = 0; i < 0x1000 / 4; ++i) {
		UsbUhciQueueDescriptor* queue = NULL;
		// 128ms
		if ((i & 0b1111111) == 0b1111111) queue = &queues[FIRST_PERIODIC_QUEUE + 7];
		// 64ms
		else if ((i & 0b111111) == 0b111111) queue = &queues[FIRST_PERIODIC_QUEUE + 6];
		// 32ms
		else if ((i & 0b11111) == 0b11111) queue = &queues[FIRST_PERIODIC_QUEUE + 5];
		// 16ms
		else if ((i & 0b1111) == 0b1111) queue = &queues[FIRST_PERIODIC_QUEUE + 4];
		// 8ms
		else if ((i & 0b111) == 0b111) queue = &queues[FIRST_PERIODIC_QUEUE + 3];
		// 4ms
		else if ((i & 0b11) == 0b11) queue = &queues[FIRST_PERIODIC_QUEUE + 2];
		// 2ms
		else if ((i & 1)) queue = &queues[FIRST_PERIODIC_QUEUE + 1];
		// 1ms
		else if ((i & 1) == 0) queue = &queues[FIRST_PERIODIC_QUEUE];
		else printf("usb uhci: error: missing queue size\n");
		frame_list[i] = (uint32_t) to_phys((uintptr_t) queue);
		frame_list[i] |= FRAME_ENTRY_QUEUE;
	}

	write32(REG_FRBASEADD, (uint32_t) to_phys((uintptr_t) frame_list));

	write16(REG_SOFMOD, 0x40);

	// clear status
	write16(REG_USBSTS, 0xFFFF);

	write16(REG_USBCMD, CMD_RUN);

	for (uint8_t i = 0; i < UINT8_MAX; i += 2) {
		if (check_port_present(i)) {
			printf("port %u8 present\n", i / 2);
			if (read16(REG_PORTSC0 + i) & PORT_SC_CUR_CONNECT_STATUS) {
				if (reset_port(i)) {
					printf("successfully enabled port %u8\n", i / 2);
					DeviceDescriptor* desc = setup_device(i);
				}
			}
			++port_count;
		}
		else break;
	}
}

static bool device_numbers[255] = {true};

static inline uint8_t get_free_device_number() {
	for (uint16_t i = 0; i < 255; ++i) {
		if (!device_numbers[i]) {
			device_numbers[i] = true;
			return i;
		}
	}
	return 0;
}

static inline DeviceDescriptor* setup_device(uint8_t port_offset) {
	DeviceRequest* desc_request = create_request(
			REQ_GET_DESCRIPTOR,
			DIR_DEVICE_TO_HOST,
			TYPE_STANDARD,
			REC_DEVICE,
			data_desc(DESC_TYPE_DEVICE, 0),
			0,
			8);

	DeviceDescriptor* out_desc = lmalloc(sizeof(DeviceDescriptor));
	memset(out_desc, 0, sizeof(DeviceDescriptor));

	UsbUhciTransferDescriptor* setup_packet = create_packet(desc_request, 8, false, PID_SETUP, false, 0, 0);
	UsbUhciTransferDescriptor* in_packet = create_packet(out_desc, 8, false, PID_IN, true, 0, 0);
	UsbUhciTransferDescriptor* status_packet = create_packet(NULL, 0, true, PID_OUT, true, 0, 0);
	link_packet(setup_packet, in_packet);
	link_packet(in_packet, status_packet);
	insert_td_chain(QUEUE_CONTROL, setup_packet, status_packet);

	force_process_current_chain(QUEUE_CONTROL);
	poll_td_active(status_packet);
	printf("setup packet status: 0x%h\n", setup_packet->d1.status);
	printf("in packet status: 0x%h\n", in_packet->d1.status);
	printf("out packet status: 0x%h\n", status_packet->d1.status);

	reset_port(port_offset);

	uint8_t dev_num = get_free_device_number();
	DeviceRequest* address_request = create_request(
			REQ_SET_ADDRESS,
			DIR_HOST_TO_DEVICE,
			TYPE_STANDARD,
			REC_DEVICE,
			data_device_addr(dev_num),
			0,
			0);

	UsbUhciTransferDescriptor* address_setup = create_packet(
			address_request,
			sizeof(*address_request),
			false,
			PID_SETUP,
			false,
			0,
			0);

	UsbUhciTransferDescriptor* address_status = create_packet(NULL, 0, true, PID_IN, true, 0, 0);
	link_packet(address_setup, address_status);
	insert_td_chain(QUEUE_CONTROL, address_setup, address_status);

	force_process_current_chain(QUEUE_CONTROL);
	poll_td_active(address_status);
	printf("address setup packet status: 0x%h\n", address_setup->d1.status);
	printf("address status: 0x%h\n", address_status->d1.status);

	free(address_status, sizeof(*address_status));
	free(address_setup, sizeof(*address_setup));
	free(setup_packet, sizeof(*setup_packet));
	free(in_packet, sizeof(*in_packet));
	free(status_packet, sizeof(*status_packet));
	free(address_request, sizeof(*address_request));

	desc_request->length = out_desc->length;
	setup_packet = create_packet(desc_request, sizeof(*desc_request), false, PID_SETUP, false, dev_num, 0);
	in_packet = create_packet(out_desc, out_desc->max_packet_size, false, PID_IN, true, dev_num, 0);
	link_packet(setup_packet, in_packet);

	UsbUhciTransferDescriptor* end = in_packet;
	bool toggle = false;
	for (uint8_t i = 0; i < out_desc->length - out_desc->max_packet_size; i += out_desc->max_packet_size) {
		UsbUhciTransferDescriptor* in_packet_2 = create_packet(
				(void*) ((uintptr_t) out_desc + 8 + i),
				out_desc->max_packet_size, false, PID_IN, toggle, dev_num, 0);
		link_packet(end, in_packet_2);
		toggle = !toggle;
		end = in_packet_2;
	}

	status_packet = create_packet(NULL, 0, true, PID_OUT, true, dev_num, 0);
	link_packet(end, status_packet);

	insert_td_chain(QUEUE_CONTROL, setup_packet, status_packet);

	force_process_current_chain(QUEUE_CONTROL);

	poll_td_active(status_packet);

	printf("class: %u8, subclass: %u8\n", out_desc->device_class, out_desc->device_subclass);
	printf("max packet size: %u8\n", out_desc->max_packet_size);

	return out_desc;
}

__attribute__((interrupt)) static void usb_interrupt(InterruptFrame*) {
	uint16_t status = read16(REG_USBSTS);
	uint16_t frame = read16(REG_FRNUM) & FRAME_LIST_CUR_NUM_MASK;
	if (status & STATUS_USBINT) {
		UsbUhciQueueDescriptor* queue = (UsbUhciQueueDescriptor*) to_virt(frame_list[frame] & ~0b1111);
		uint8_t queue_index = (uintptr_t) (queue - queues) / sizeof(UsbUhciQueueDescriptor);

		uint8_t current_chain = current_chain_indices[queue_index];
		uint8_t next_chain = current_chain < 2 ? current_chain + 1 : 0;
		current_chain_indices[queue_index] = next_chain;

		if (queue->element_link_ptr & TD_PTR_FLAG_TERMINATE) {
			UsbUhciTransferDescriptor* td = td_chain_starts[queue_index * 3 + next_chain];
			if (td) {
				queue->element_link_ptr = to_phys((uintptr_t) td);
				td_chain_starts[queue_index * 3 + next_chain] = NULL;
			}
		}
	}
	else {
		printf("usb error: 0x%h\n", status & ~STATUS_USBINT);
		write16(REG_USBSTS, 0xFFFF);
	}

	write16(REG_USBSTS, STATUS_USBINT);
	lapic_write(LAPIC_REG_EOI, 0);
}

static inline bool check_port_present(uint8_t port_offset) {
	if ((read16(REG_PORTSC0 + port_offset) & 1 << 7) == 0) return false;

	write16(REG_PORTSC0 + port_offset, read16(REG_PORTSC0 + port_offset) & ~(1 << 7));
	if ((read16(REG_PORTSC0 + port_offset) & 1 << 7) == 0) return false;

	write16(REG_PORTSC0 + port_offset, read16(REG_PORTSC0 + port_offset) | 1 << 7);
	if ((read16(REG_PORTSC0 + port_offset) & 1 << 7) == 0) return false;

	write16(REG_PORTSC0 + port_offset, read16(REG_PORTSC0 + port_offset) | 0xA);
	if ((read16(REG_PORTSC0 + port_offset) & 0xA) != 0) return false;

	return true;
}

static inline bool reset_port(uint8_t port_offset) {
	uint32_t value = read16(REG_PORTSC0 + port_offset);
	write16(REG_PORTSC0 + port_offset, value | PORT_SC_RESET);
	udelay(USB_RESET_DELAY);

	value = read16(REG_PORTSC0 + port_offset);
	write16(REG_PORTSC0 + port_offset, value & ~PORT_SC_RESET);
	udelay(300);

	// clear connect status change and set enable bit
	value = read16(REG_PORTSC0 + port_offset);
	write16(REG_PORTSC0 + port_offset, value | PORT_SC_CONNECT_STATUS_CHANGE);
	write16(REG_PORTSC0 + port_offset, value | PORT_SC_ENABLE);
	udelay(50);

	value = read16(REG_PORTSC0 + port_offset);
	// clear bits and set enable bit again
	value |= PORT_SC_CUR_CONNECT_STATUS | PORT_SC_CONNECT_STATUS_CHANGE | PORT_SC_ENABLE | PORT_SC_ENABLE_CHANGE;
	write16(REG_PORTSC0 + port_offset, value);
	udelay(50 * 1000);

	return read16(REG_PORTSC0 + port_offset) & PORT_SC_ENABLE;
}