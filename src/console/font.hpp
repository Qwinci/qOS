#pragma once

struct PSF1Font {
	uint32_t magic;
	uint32_t version;
	uint32_t headerSize;
	uint32_t flags;
	uint32_t numGlyph;
	uint32_t bytesPerGlyph;
	uint32_t height;
	uint32_t width;
};