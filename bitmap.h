// Header guard
#ifndef BITMAP_H
#define BITMAP_H


// Header files
#include "common.h"
#include <memory>
#include <new>

using namespace std;


// Classes

// Bitmap class
template<const uint64_t size> class Bitmap final {

	// Public
	public:
	
		// Allocated memory size
		static constexpr const size_t ALLOCATED_MEMORY_SIZE = size / BITS_IN_A_BYTE;
		
		// Constructor
		__attribute__((always_inline)) inline explicit Bitmap() noexcept;
		
		// Bool operator
		__attribute__((always_inline)) inline explicit operator bool() const noexcept;
		
		// Clear
		__attribute__((always_inline)) inline void clear() noexcept;
		
		// Set bit
		__attribute__((always_inline)) inline void setBit(const uint64_t index) noexcept;
		
		// Is bit set
		__attribute__((always_inline)) inline bool isBitSet(const uint64_t index) const noexcept;
		
	// Private
	private:
	
		// Buffer
		unique_ptr<uint64_t[], void(*)(uint64_t [])> buffer;
};


// Supporting function implementation

// Constructor
template<const uint64_t size> __attribute__((always_inline)) inline Bitmap<size>::Bitmap() noexcept :

	// Create buffer
	buffer(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint64_t[size / (sizeof(uint64_t) * BITS_IN_A_BYTE)], [](uint64_t buffer[size / (sizeof(uint64_t) * BITS_IN_A_BYTE)]) __attribute__((always_inline)) noexcept {
	
		// Free buffer
		operator delete[](buffer, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
	})
{

	// Throw error if size is invalid
	static_assert(size && size % (sizeof(uint64_t) * BITS_IN_A_BYTE) == 0, "Bitmap size is invalid");
	
	// Check if creating buffer was successful
	if(buffer) [[likely]] {
	
		// Ensure memory is fully allocated
		setBufferGuaranteed(buffer.get(), 0, size / BITS_IN_A_BYTE);
	}
}

// Bool operator
template<const uint64_t size> __attribute__((always_inline)) inline Bitmap<size>::operator bool() const noexcept {

	// Return if creating buffer was successful
	return buffer.get();
}

// Clear
template<const uint64_t size> __attribute__((always_inline)) inline void Bitmap<size>::clear() noexcept {

	// Clear buffer
	__builtin_memset(buffer.get(), 0, size / BITS_IN_A_BYTE);
}

// Set bit
template<const uint64_t size> __attribute__((always_inline)) void inline Bitmap<size>::setBit(const uint64_t index) noexcept {

	// Set bit in buffer
	__builtin_assume(index < size);
	buffer[index / (sizeof(uint64_t) * BITS_IN_A_BYTE)] |= static_cast<uint64_t>(1) << (index % (sizeof(uint64_t) * BITS_IN_A_BYTE));
}

// Is bit set
template<const uint64_t size> __attribute__((always_inline)) bool inline Bitmap<size>::isBitSet(const uint64_t index) const noexcept {

	// Return if bit is set in buffer
	__builtin_assume(index < size);
	return buffer[index / (sizeof(uint64_t) * BITS_IN_A_BYTE)] & (static_cast<uint64_t>(1) << (index % (sizeof(uint64_t) * BITS_IN_A_BYTE)));
}


#endif
