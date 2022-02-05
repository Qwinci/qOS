#include "renderer.hpp"
#include "font.hpp"
#include <cstddef>

Renderer& Renderer::operator<<(char c) {
	if (c == '\n') {
		xOff = 0;
		yOff += 16;
	}
	else if (c == '\t') {
		if (xOff + 4 * 8 > frameBuffer.width) {
			xOff = 0;
			yOff += 16;
		}
		else {
			xOff += 4 * 8;
		}
	}
	else {
		putChar(c, xOff, yOff, fgColor, bgColor);
		if (xOff + 8 >= frameBuffer.width) {
			xOff = 0;
			yOff += 16;
		}
		else {
			xOff += 8;
		}

		if (yOff + 16 > frameBuffer.height) {
			for (int y = 0; y < yOff; ++y) {
				for (int x = 0; x < frameBuffer.width; ++x) {
					*(uint32_t*)(frameBuffer.address + frameBuffer.width * frameBuffer.bpp / 8 * y + frameBuffer.bpp / 8 * x)
							= *(uint32_t*)(frameBuffer.address + frameBuffer.width * frameBuffer.bpp / 8 * (y + 16) + frameBuffer.bpp / 8 * x);
				}
			}
			yOff -= 16;
		}
	}
	return *this;
}
Renderer &Renderer::operator<<(const char *string) {
	for (int i = 0; string[i] != 0; ++i) {
		operator<<(string[i]);
	}
	return *this;
}

Renderer& Renderer::printNumber(uint64_t number) {
	if (number == 0) {
		operator<<('0');
		return *this;
	}
	switch (mode) {
		case Mode::Normal:
		{
			size_t length = 0;
			for (uint64_t newNumber = number; newNumber > 0; newNumber /= 10) ++length;
			char string[length + 1];
			string[length] = 0;
			while (number > 0) {
				string[--length] = static_cast<char>('0' + number % 10);
				number /= 10;
			}
			operator<<(string);
			break;
		}
		case Mode::Hex:
		{
			size_t length = 0;
			for (uint64_t newNumber = number; newNumber > 0; newNumber >>= 4) ++length;
			char string[length + 1];
			string[length] = 0;
			while (number > 0) {
				char remainder = static_cast<char>(number & 0xF);
				string[--length] = remainder > 9 ? static_cast<char>('A' + remainder - 10) : static_cast<char>('0' + remainder);
				number >>= 4;
			}
			operator<<(string);
			break;
		}
		case Mode::Binary:
		{
			size_t length = 0;
			for (uint64_t newNumber = number; newNumber > 0; newNumber >>= 1) ++length;
			char string[length + 1];
			string[length] = 0;
			while (number > 0) {
				string[--length] = static_cast<char>('0' + (number & 1));
				number >>= 1;
			}
			operator<<(string);
			break;
		}
	}
	return *this;
}
Renderer& Renderer::printNumber(int64_t number) {
	if (number == 0) {
		operator<<('0');
		return *this;
	}
	switch (mode) {
		case Mode::Normal:
		{
			size_t length = 0;
			for (uint64_t newNumber = number; newNumber > 0; newNumber /= 10) ++length;
			char string[length + 1];
			string[length] = 0;
			while (number > 0) {
				string[--length] = static_cast<char>('0' + number % 10);
				number /= 10;
			}
			operator<<(string);
			break;
		}
		case Mode::Hex:
		{
			size_t length = 0;
			for (uint64_t newNumber = number; newNumber > 0; newNumber >>= 4) ++length;
			char string[length + 1];
			string[length] = 0;
			while (number > 0) {
				char remainder = static_cast<char>(number & 0xF);
				string[--length] = remainder > 9 ? static_cast<char>('A' + remainder - 10) : static_cast<char>('0' + remainder);
				number >>= 4;
			}
			operator<<(string);
			break;
		}
		case Mode::Binary:
		{
			size_t length = 0;
			for (uint64_t newNumber = number; newNumber > 0; newNumber >>= 1) ++length;
			char string[length + 1];
			string[length] = 0;
			while (number > 0) {
				string[--length] = static_cast<char>('0' + (number & 1));
				number >>= 1;
			}
			operator<<(string);
			break;
		}
	}
	return *this;
}

void Renderer::putChar(unsigned char character, int charX, int charY, uint32_t fg, uint32_t bg) const {
	auto* fontHeader = reinterpret_cast<PSF1Font*>(fontStart);
	uint32_t index = character * fontHeader->bytesPerGlyph;
	for (uint32_t y = 0; y < fontHeader->height; ++y) {
		uint8_t currentLine = reinterpret_cast<unsigned char*>(fontStart + fontHeader->headerSize)[index + y];
		for (uint32_t x = 0; x < fontHeader->width; ++x) {
			uint8_t currentBit = (currentLine >> (8 - x)) & 1;
			*(uint32_t*)(frameBuffer.address + frameBuffer.pitch * (y + charY) + frameBuffer.bpp / 8 * (x + charX))
				= currentBit ? fg : bg;
		}
	}
}

void Renderer::setColor(uint32_t color) {
	fgColor = color;
}

void Renderer::setBGColor(uint32_t color) {
	bgColor = color;
}

void Renderer::clear(uint32_t clearColor) {
	for (int y = 0; y < frameBuffer.height; ++y) {
		for (int x = 0; x < frameBuffer.width; ++x) {
			*(uint32_t*)(frameBuffer.address + frameBuffer.width * frameBuffer.bpp / 8 * y + frameBuffer.bpp / 8 * x)
			= clearColor;
		}
	}
	xOff = 0;
	yOff = 0;
}

Renderer &Renderer::operator<<(Mode newMode) {
	mode = newMode;
	return *this;
}

Renderer &Renderer::operator<<(uint8_t number) {
	return printNumber(static_cast<uint64_t>(number));
}

Renderer &Renderer::operator<<(uint16_t number) {
	return printNumber(static_cast<uint64_t>(number));
}

Renderer &Renderer::operator<<(uint32_t number) {
	return printNumber(static_cast<uint64_t>(number));
}

Renderer &Renderer::operator<<(uint64_t number) {
	return printNumber(static_cast<uint64_t>(number));
}

Renderer &Renderer::operator<<(int16_t number) {
	return printNumber(static_cast<int64_t>(number));
}

Renderer &Renderer::operator<<(int32_t number) {
	return printNumber(static_cast<int64_t>(number));
}

Renderer &Renderer::operator<<(int64_t number) {
	return printNumber(static_cast<int64_t>(number));
}

Renderer &Renderer::operator<<(bool value) {
	if (value) {
		operator<<("true");
	}
	else {
		operator<<("false");
	}
	return *this;
}
