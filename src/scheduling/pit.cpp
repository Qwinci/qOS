#include "pit.hpp"
#include "../utils/io.hpp"
#include "../interrupts/pic.hpp"
#include <cstdint>

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43

enum Channel : uint8_t {
	Channel0 = 0,
	Channel1 = 1 << 7,
	Channel2 = 1 << 6,
	ReadBackCommand = 1 << 6 | 1 << 7
};
enum class AccessMode : uint8_t {
	LatchCountValueCommand = 0,
	LoByteOnly = 1 << 5,
	HiByteOnly = 1 << 4,
	LoHiByte = 1 << 4 | 1 << 5
};
enum class OperatingMode : uint8_t {
	InterruptOnTerminalCount = 0,
	HardwareReTriggerableOneShot = 1 << 3,
	RateGenerator = 1 << 2,
	SquareWaveGenerator = 1 << 2 | 1 << 3,
	SoftwareTriggeredStrobe = 1 << 1,
	HardwareTriggeredStrobe = 1 << 1 | 1 << 3,
	RateGenerator2 = 1 << 1 | 1 << 2,
	SquareWaveGenerator2 = 1 << 1 | 1 << 2 | 1 << 3
};
enum class BinaryMode : uint8_t {
	Binary16 = 0,
	FourDigitBCD = 1
};

void initializePIT() {
	asm("cli");
	clearMask(0);

	uint32_t divisor = 1193180 / 1000;

	out1(PIT_COMMAND, static_cast<uint8_t>(Channel::Channel0) | static_cast<uint8_t>(AccessMode::LoHiByte) |
	static_cast<uint8_t>(OperatingMode::RateGenerator));

	uint8_t low = divisor & 0xFF;
	uint8_t high = divisor >> 8 & 0xFF;

	out1(PIT_CHANNEL0, low);
	io_wait();
	out1(PIT_CHANNEL0, high);
	asm("sti");
}