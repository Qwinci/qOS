#include "sb16.h"
#include "utils/io.h"
#include "stdio.h"
#include "paging/memory.h"
#include "std/memory.h"

#define DSP_MIXER_PORT 0x224
#define DSP_MIXER_DATA_PORT 0x225
#define DSP_RESET_PORT 0x226
#define DSP_READ_PORT 0x22A
#define DSP_WRITE_PORT 0x22C
#define DSP_READ_STATUS_PORT 0x22E
#define DSP_16BIT_INTERRUPT_ACK_PORT 0x22F

#define DSP_CMD_SET_TIME_CONSTANT 0x40
#define DSP_CMD_SET_SAMPLE_RATE 0x41
#define DSP_CMD_TURN_SPEAKER_ON 0xD1
#define DSP_CMD_TURN_SPEAKER_OFF 0xD3
#define DSP_CMD_STOP_PLAYING_8 0xD0
#define DSP_CMD_RESUME_8 0xD4
#define DSP_CMD_STOP_PLAYING_16 0xD5
#define DSP_CMD_RESUME_16 0xD6
#define DSP_CMD_GET_VERSION 0xE1

#define DSP_MIXER_CMD_SET_VOLUME 0x22
#define DSP_MIXER_CMD_SET_IRQ 0x80

/*static inline void udelay(uint8_t count) {
	for (uint8_t i = 0; i < count; ++i) io_wait();
}

extern unsigned char output_raw[];
extern unsigned int output_raw_len;

uintptr_t sound_data = 0;
size_t sound_data_offset = 0;
size_t sound_data_remaining = 0;

bool initialize_sb16() {
	out1(DSP_RESET_PORT, 1);
	udelay(3);
	out1(DSP_RESET_PORT, 0);

	bool is_good = false;
	for (uint8_t i = 0; i < 150; ++i) {
		if (in1(DSP_READ_STATUS_PORT) & 1 << 7 && in1(DSP_READ_PORT) == 0xAA) {
			is_good = true;
			break;
		}
		udelay(1);
	}

	if (!is_good) return false;

	out1(DSP_MIXER_PORT, DSP_MIXER_CMD_SET_IRQ);
	out1(DSP_MIXER_DATA_PORT, 0x2); // irq 5

	out1(DSP_WRITE_PORT, DSP_CMD_TURN_SPEAKER_ON);
	sound_data = (uintptr_t) pmalloc(output_raw_len / 0x1000 + 1, MEMORY_ALLOC_TYPE_LOW);
	memcpy((void*) sound_data, output_raw, output_raw_len);

	if ((uintptr_t) sound_data - 0xFFFF800000000000 > 0xFFFFFF) {
		printf("not playable sound data position %h\n", sound_data - 0xFFFF800000000000);
		return false;
	}

	uint8_t sound_data_page = (uint8_t) ((uintptr_t) sound_data >> 16 & 0xFF);
	uint8_t sound_data_low = (uint8_t) ((uintptr_t) sound_data & 0xFF);
	uint8_t sound_data_high = (uint8_t) ((uintptr_t) sound_data >> 8 & 0xFF);

	// disable channel 1
	out1(0xA, 0x04 + 1);
	// flip flop
	out1(0xC, 1);
	// transfer mode (0x48 single mode, 0x58 auto mode) + channel number
	out1(0xB, 0x59);
	// page (0x[01]0000
	out1(0x83, sound_data_page);
	// position low (0x0100[00]
	out1(0x2, sound_data_low);
	// position high (0x01[00]00)
	out1(0x2, sound_data_high);
	// count low (0x0F[FF])
	out1(0x3, output_raw_len & 0xFF);
	// count high (0x[0F]FF
	out1(0x3, output_raw_len >> 8 & 0xFF);
	// enable channel 1
	out1(0xA, 1);

	uint16_t sample_rate = 44100;
	out1(DSP_WRITE_PORT, DSP_CMD_SET_SAMPLE_RATE);
	// high
	out1(DSP_WRITE_PORT, sample_rate >> 8);
	// low
	out1(DSP_WRITE_PORT, sample_rate);
	// 8 bit sound (0xC0 8bit, 0xB0 16bit)
	out1(DSP_WRITE_PORT, 0xC0);
	// unsigned mono data (bit4 signed/unsigned, bit5 stereo/mono)
	out1(DSP_WRITE_PORT, 0);
	// low bit of (count - 1)
	out1(DSP_WRITE_PORT, (output_raw_len - 1) & 0xFF);
	// high bit of (count - 1)
	out1(DSP_WRITE_PORT, (output_raw_len - 1) >> 8 & 0xFF);

	sound_data_remaining = output_raw_len - 0xFFFF;
	sound_data_offset += 0xFFFF;

	return true;
}

void sb16_update() {
	memcpy((void*) sound_data, (const void*) (sound_data + sound_data_offset), sound_data_remaining > 0xFFFF ? 0xFFFF : sound_data_remaining);

	sound_data_remaining = sound_data_remaining > 0xFFFF ? sound_data_remaining - 0xFFFF : 0;
	sound_data_offset += sound_data_remaining > 0xFFFF ? 0xFFFF : sound_data_remaining;

	in1(DSP_READ_STATUS_PORT);
	out1(DSP_WRITE_PORT, DSP_CMD_RESUME_8);
}*/