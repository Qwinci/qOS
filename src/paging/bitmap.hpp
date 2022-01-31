#pragma once
#include <cstdint>
#include <cstddef>

class Bitmap {
public:
	class reference {
		reference(uint64_t* entry, size_t index) : entry{entry}, index{index} {}
	public:
		~reference() = default;

		reference& operator=(bool value);
		reference& operator=(const reference& value) = default;
		operator bool() const; // NOLINT(google-explicit-constructor)
	private:
		uint64_t* entry;
		size_t index;

		friend class Bitmap;
	};

	Bitmap(uintptr_t address, size_t size);
	Bitmap() = default;
	constexpr bool operator[](size_t index) const;
	reference operator[](size_t index);

private:
	uint64_t* buffer;
};