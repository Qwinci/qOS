#include "intel_82574.h"
#include "paging/memory.h"
#include "stdio.h"
#include <stdbool.h>
#include "interrupts/idt.h"

static uint64_t bar0;

typedef enum : uint32_t {
	REG_CTRL = 0x4,
	REG_STATUS = 0x8,
	REG_EEC = 0x10,
	REG_EERD = 0x14,
	REG_CTRL_EXT = 0x18,
	REG_FLA = 0x1C,
	REG_MDIC = 0x20,
	REG_FCAL = 0x28,
	REG_FCAH = 0x2C,
	REG_FCT = 0x30,
	REG_VET = 0x38,
	REG_FCTTV = 0x170,
	REG_FCRTV = 0x5F40,
	REG_LED_CTL = 0xE00,
	REG_EXTCNF_CTRL = 0xF00,
	REG_EXTCNF_SIZE = 0xF08,
	REG_PBA = 0x1000,
	REG_EEMNGCTL = 0x1010,
	REG_EEMNGDATA = 0x1014,
	REG_FLMNGCTL = 0x1018,
	REG_FLMNGDATA = 0x101C,
	REG_FLMNGCNT = 0x1020,
	REG_FLASHT = 0x1028,
	REG_EEWR = 0x102C,
	REG_FLSWCTL = 0x1030,
	REG_FLSWDATA = 0x1034,
	REG_FLSWCNT = 0x1038,
	REG_FLOP = 0x103C,
	REG_FLOL = 0x1050,
	REG_GCR = 0x5B00,
	REG_FUNCTAG = 0x5B08,
	REG_GSCL_1 = 0x5B10,
	REG_GSCL_2 = 0x5B14,
	REG_GSCL_3 = 0x5B18,
	REG_GSCL_4 = 0x5B1C,
	REG_GSCN_0 = 0x5B20,
	REG_GSCN_1 = 0x5B24,
	REG_GSCN_2 = 0x5B28,
	REG_GSCN_3 = 0x5B2C,
	REG_SWSM = 0x5B50,
	REG_GCR2 = 0x5B64,
	REG_PBACLR = 0x5B68,
	REG_ICR = 0xC0,
	REG_ITR = 0xC4,
	REG_EITR_0 = 0xE8,
	REG_EITR_1 = 0xE8 + 4,
	REG_EITR_2 = 0xE8 + 8,
	REG_EITR_3 = 0xE8 + 12,
	REG_ICS = 0xC8,
	REG_IMS = 0xD0,
	REG_IMC = 0xD8,
	REG_EIAC = 0xDC,
	REG_IAM = 0xE0,
	REG_IVAR = 0xE4,
	REG_RCTL = 0x100,
	REG_PSRCTL = 0x2170,
	REG_FCRTL = 0x2160,
	REG_FCRTH = 0x2168,
	REG_RDBAL0 = 0x2800,
	REG_RDBAH0 = 0x2804,
	REG_RDLEN0 = 0x2808,
	REG_RDH0 = 0x2810,
	REG_RDT0 = 0x2818,
	REG_RDTR = 0x2820,
	REG_RXDCTL = 0x2828,
	REG_RADV = 0x282C,
	REG_RSRPD = 0x2C00,
	REG_RAID = 0x2C08,
	REG_RXCSUM = 0x5000,
	REG_RFCTL = 0x5008,
	REG_MADVTV0 = 0x5010,
	REG_MADVTV1 = 0x5014,
	REG_MADVTV2 = 0x5018,
	REG_MADVTV3 = 0x501C,
	REG_MTA_START = 0x5200,
	REG_MTA_LAST = 0x53FC
} Register;

#pragma region control
/// 0 half duplex, 1 full duplex
#define CTRL_FD (1 << 0)
#define CTRL_GIO_MASTER_DISABLE (1 << 2)
/// mac auto speed detection enable
#define CTRL_ASDE (1 << 5)
/// set link up
#define CTRL_SLU (1 << 6)
#define CTRL_SPEED_MASK (0b11 << 8)
#define CTRL_SPEED_10MB (0 << 8)
#define CTRL_SPEED_100MB (1 << 8)
#define CTRL_SPEED_1000MB (1 << 9)
#define CTRL_SPEED_NOT_USED (0b11 << 8)
/// force duplex
#define CTRL_FRCDPLX (1 << 12)
/// d3cold wakeup capability advertisement enable (must be set)
#define CTRL_ADVD3WUC (1 << 20)
/// device reset (mac)
#define CTRL_RST (1 << 26)
/// receive flow control enable
#define CTRL_RFCE (1 << 27)
/// transfer flow control enable
#define CTRL_TFCE (1 << 28)
/// vlan mode enable
#define CTRL_VME (1 << 30)
/// phy reset
#define CTRL_PHY_RST (1 << 31)
#pragma endregion

static inline void write_reg(Register reg, uint32_t value) {
	*(volatile uint32_t*) (bar0 + reg) = value;
}

static inline uint32_t read_reg(Register reg) {
	return *(volatile uint32_t*) (bar0 + reg);
}

static __attribute__((interrupt)) void net_int(InterruptFrame* frame);

#define EERD_START (1 << 0)
#define EERD_DONE (1 << 1)

static uint16_t read_eeprom(uint8_t addr) {
	// start
	uint32_t tmp = EERD_START | (uint32_t) addr << 2;

	write_reg(REG_EERD, tmp);

	while ((read_reg(REG_EERD) & EERD_DONE) == 0) __asm__ volatile("pause" : : : "memory");

	uint16_t data = read_reg(REG_EERD) >> 16;

	tmp = read_reg(REG_EERD) & ~EERD_START;
	write_reg(REG_EERD, tmp);

	return data;
}

#define EEC_SK (1 << 0)
#define EEC_CS (1 << 1)
#define EEC_DI (1 << 2)
#define EEC_REQ (1 << 6)
#define EEC_GNT (1 << 7)

static inline void lock_eeprom() {
	uint32_t eec = read_reg(REG_EEC);
	eec |= EEC_REQ;
	write_reg(REG_EEC, eec);

	while ((read_reg(REG_EEC) & EEC_GNT) == 0) {
		if (eec != read_reg(REG_EEC)) {
			printf("new value: 0x%h\n", read_reg(REG_EEC));
		}
		__asm__ volatile("pause" : : : "memory");
	}
}

static inline void unlock_eeprom() {
	uint32_t eec = read_reg(REG_EEC);
	eec &= ~EEC_REQ;
	write_reg(REG_EEC, eec);
}

void initialize_intel_82574(PCIDeviceHeader0* header) {
	uint8_t type = (header->BAR0 & 0b110) >> 1;
	if (type == 0) bar0 = header->BAR0 & 0xFFFFFFF0;
	else if (type == 2) bar0 = (uint64_t) (header->BAR0 & 0xFFFFFFF0) + ((uint64_t) header->BAR1 << 32);

	uint32_t original_bar0 = header->BAR0;
	header->BAR0 = 0xFFFFFFFF;
	uint32_t new_bar = *(volatile uint32_t*) (&header->BAR0);
	uint32_t size = ~(new_bar & 0xFFFFFFF0) + 1;
	header->BAR0 = original_bar0;

	for (size_t i = 0; i < size; i += 0x1000) {
		pmap(
				bar0 + i,
				to_virt(bar0 + i),
				PAGEFLAG_PRESENT | PAGEFLAG_RW | PAGEFLAG_CACHE_DISABLE);
		prefresh(to_virt(bar0 + i));
	}

	bar0 = to_virt(bar0);

	printf("disabling interrupts\n");

	write_reg(REG_IMC, 1);

	printf("starting phy reset\n");

	write_reg(REG_CTRL, read_reg(REG_CTRL) | CTRL_PHY_RST);

	printf("waiting for phy reset...\n");

	while (read_reg(REG_CTRL) & CTRL_PHY_RST) __asm__("pause" : : : "memory");

	printf("phy reset done\n");

	printf("disabling interrupts again after phy reset\n");

	write_reg(REG_IMC, 1);

	write_reg(REG_GCR, read_reg(REG_GCR) | 1 << 22);

	if ((read_reg(REG_EEC) & 1 << 8) == 0) {
		printf("intel 82574: no nvm present\n");
		return;
	}

	PciMsiCapability* msi = pci_get_msi_cap0(header);
	msi->msg_data = NETWORK_CONTROLLER_0_INT; // interrupt vector
	// address + cpu
	msi->msg_addr = 0xFEE00000 | 0 << 12;
	msi->msg_control |= MSI_CTRL_ENABLE;
	if ((msi->msg_control & MSI_CTRL_MM_CAPABLE) != MSI_CTRL_MM_1_CAPABLE) {
		printf("intel 82574: requesting more than 1 interrupt, allocating only one.\n");
	}
	msi->msg_control |= MSI_CTRL_MM_1_ENABLE;

	register_interrupt(NETWORK_CONTROLLER_0_INT, net_int, INTERRUPT_TYPE_INTERRUPT);

	// enable eeprom
	uint32_t eec = read_reg(REG_EEC);
	eec |= EEC_SK | EEC_CS | EEC_DI;
	write_reg(REG_EEC, eec);

	printf("locking eeprom for mac read\n");

	lock_eeprom();
	printf("eeprom locked\n");
	uint8_t mac_addr[6];
	*(uint16_t*) &mac_addr[0] = read_eeprom(0x0);
	*(uint16_t*) &mac_addr[2] = read_eeprom(0x1);
	*(uint16_t*) &mac_addr[4] = read_eeprom(0x2);
	unlock_eeprom();

	printf("mac address: %h %h %h %h %h %h",
		   mac_addr[0], mac_addr[1], mac_addr[2],
		   mac_addr[3], mac_addr[4], mac_addr[5]);
}

static __attribute__((interrupt)) void net_int(InterruptFrame* frame) {}