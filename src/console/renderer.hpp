#pragma once
#include "bootInfo.hpp"

enum class Mode {
	Normal,
	Hex,
	Binary
};

class Renderer {
public:
	explicit Renderer(FrameBuffer& frameBuffer) : frameBuffer{frameBuffer} {}
	Renderer(FrameBuffer& frameBuffer, uintptr_t font) : frameBuffer{frameBuffer}, fontStart{font} {}
	Renderer() = default;

	void setColor(uint32_t color);
	void setBGColor(uint32_t color);
	void clear(uint32_t clearColor);

	Renderer& operator<<(char c);
	Renderer& operator<<(const char* string);
	Renderer& operator<<(uint8_t number);
	Renderer& operator<<(uint16_t number);
	Renderer& operator<<(uint32_t number);
	Renderer& operator<<(uint64_t number);
	Renderer& operator<<(int16_t number);
	Renderer& operator<<(int32_t number);
	Renderer& operator<<(int64_t number);
	Renderer& operator<<(Mode newMode);

private:
	void putChar(unsigned char character, int charX, int charY, uint32_t fg, uint32_t bg) const;
	Renderer& printNumber(uint64_t number);
	Renderer& printNumber(int64_t number);

	uint32_t fgColor{0xFFFFFF};
	uint32_t bgColor{0};
	int xOff{0}, yOff{0};
	FrameBuffer frameBuffer{};
	uintptr_t fontStart{};
	Mode mode{Mode::Normal};
};

extern Renderer globalRenderer;