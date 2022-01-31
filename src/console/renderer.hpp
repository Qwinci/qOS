#pragma once
#include "bootInfo.hpp"

class Renderer {
public:
	explicit Renderer(FrameBuffer& frameBuffer) : frameBuffer{frameBuffer} {}

	void operator<<(char c);

private:
	int x{0}, y{0};
	FrameBuffer frameBuffer;
};

extern Renderer globalRenderer;