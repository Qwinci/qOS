#include "bitmap.hpp"

Bitmap::Bitmap(uintptr_t address, size_t size) {
	buffer = reinterpret_cast<uint64_t*>(address);

	for (size_t i = 0; i < size / sizeof(uint64_t) + 1; ++i) {
		buffer[i] = 0;
	}
}

Bitmap::reference Bitmap::operator[](size_t index) {
	return reference {&buffer[index / 64], index % 64};;
}

constexpr bool Bitmap::operator[](size_t index) const {
	return buffer[index / 64] & 1 << (index % 64);
}

void Bitmap::changeBufferAddress(uintptr_t address) {
	buffer = reinterpret_cast<uint64_t*>(address);
}

Bitmap::reference &Bitmap::reference::operator=(bool value) {
	if (value) {
		*entry |= 1 << index;
	}
	else {
		*entry &= ~(1 << index);
	}
	return *this;
}

Bitmap::reference::operator bool() const {
	return *entry & 1 << index;
}
