// Header guard
#ifndef HASH_TABLE_H
#define HASH_TABLE_H


// Header files
#include "common.h"
#include <memory>
#include <new>

using namespace std;


// Classes

// Hash table class
template<const uint32_t size, const bool isSet = false> class HashTable final {

	// Private
	private:
	
		// Hash table entry structure
		struct HashTableEntry {
		
			// Key
			uint32_t key;
			
			// Value
			uint32_t value;
		};
		
	// Public
	public:
	
		// No value
		static constexpr const uint32_t NO_VALUE = __builtin_expect(isSet, true) ? 0 : UINT32_MAX;
		
		// Allocated memory size
		static constexpr const size_t ALLOCATED_MEMORY_SIZE = bit_ceil(size + 1) * sizeof(HashTableEntry);
		
		// Constructor
		__attribute__((always_inline)) inline explicit HashTable() noexcept;
		
		// Bool operator
		__attribute__((always_inline)) inline explicit operator bool() const noexcept;
		
		// Set unique
		__attribute__((always_inline)) inline void setUnique(const uint32_t key, const uint32_t value) noexcept;
		__attribute__((always_inline)) inline void setUnique(const uint32_t key) noexcept;
		
		// Set unique and get index
		__attribute__((always_inline)) inline uint32_t setUniqueAndGetIndex(const uint32_t key, const uint32_t value) noexcept;
		__attribute__((always_inline)) inline uint32_t setUniqueAndGetIndex(const uint32_t key) noexcept;
		
		// Replace
		__attribute__((always_inline)) inline uint32_t replace(const uint32_t key, const uint32_t value) noexcept;
		
		// Remove most recent set unique
		__attribute__((always_inline)) inline void removeMostRecentSetUique(const uint32_t index) noexcept;
		
		// Clear
		__attribute__((always_inline)) inline void clear() noexcept;
		
		// Contains
		__attribute__((always_inline)) inline bool contains(const uint32_t key) const noexcept;
		
		// Get
		__attribute__((always_inline)) inline uint32_t get(const uint32_t key) const noexcept;
		
	// Private
	private:
	
		// Entries
		const unique_ptr<HashTableEntry[], void(*)(HashTableEntry [])> entries;
};


// Supporting function implementation

// Constructor
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline HashTable<size, isSet>::HashTable() noexcept :

	// Create entries
	entries(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) HashTableEntry[bit_ceil(size + 1)], [](HashTableEntry entries[bit_ceil(size + 1)]) __attribute__((always_inline)) noexcept {
	
		// Free entries
		operator delete[](entries, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
	})
{

	// Throw error if size is invalid
	static_assert(size && size <= bit_ceil(UINT32_MAX >> 1) - 1, "Hash table size is invalid");
	
	// Check if creating entries was successful
	if(entries) [[likely]] {
	
		// Ensure memory is fully allocated
		setBufferGuaranteed(entries.get(), NO_VALUE, bit_ceil(size + 1) * sizeof(HashTableEntry));
	}
}

// Bool operator
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline HashTable<size, isSet>::operator bool() const noexcept {

	// Return if creating entries was successful
	return entries.get();
}

// Set unique
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline void HashTable<size, isSet>::setUnique(const uint32_t key, const uint32_t value) noexcept {

	// Throw error if is set
	static_assert(!isSet, "Set unique can't be used with a set");
	
	// Get key's index
	uint32_t index = key % bit_ceil(size + 1);
	
	// Loop while entry at index exists
	while(entries[index].value != NO_VALUE) [[unlikely]] {
	
		// Check if index isn't for the last entry
		if(index != bit_ceil(size + 1) - 1) [[likely]] {
		
			// Increment index
			++index;
		}
		
		// Otherwise
		else [[unlikely]] {
		
			// Set index to the first entry
			index = 0;
		}
	}
	
	// Set entry at index to the value
	entries[index] = {key, value};
}

// Set unique
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline void HashTable<size, isSet>::setUnique(const uint32_t key) noexcept {

	// Throw error if isn't set
	static_assert(isSet, "Set unique can't be used with a map");
	
	// Get key's index
	uint32_t index = key % bit_ceil(size + 1);
	
	// Loop while entry at index exists
	while(entries[index].value != NO_VALUE) [[unlikely]] {
	
		// Check if index isn't for the last entry
		if(index != bit_ceil(size + 1) - 1) [[likely]] {
		
			// Increment index
			++index;
		}
		
		// Otherwise
		else [[unlikely]] {
		
			// Set index to the first entry
			index = 0;
		}
	}
	
	// Set entry at index to one
	entries[index] = {key, 1};
}

// Set unique and get index
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline uint32_t HashTable<size, isSet>::setUniqueAndGetIndex(const uint32_t key, const uint32_t value) noexcept {

	// Throw error if is set
	static_assert(!isSet, "Set unique and get index can't be used with a set");
	
	// Get key's index
	uint32_t index = key % bit_ceil(size + 1);
	
	// Loop while entry at index exists
	while(entries[index].value != NO_VALUE) [[unlikely]] {
	
		// Check if index isn't for the last entry
		if(index != bit_ceil(size + 1) - 1) [[likely]] {
		
			// Increment index
			++index;
		}
		
		// Otherwise
		else [[unlikely]] {
		
			// Set index to the first entry
			index = 0;
		}
	}
	
	// Set entry at index to the value
	entries[index] = {key, value};
	
	// Return index
	return index;
}

// Set unique and get index
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline uint32_t HashTable<size, isSet>::setUniqueAndGetIndex(const uint32_t key) noexcept {

	// Throw error if isn't set
	static_assert(isSet, "Set unique and get index can't be used with a map");
	
	// Get key's index
	uint32_t index = key % bit_ceil(size + 1);
	
	// Loop while entry at index exists
	while(entries[index].value != NO_VALUE) [[unlikely]] {
	
		// Check if index isn't for the last entry
		if(index != bit_ceil(size + 1) - 1) [[likely]] {
		
			// Increment index
			++index;
		}
		
		// Otherwise
		else [[unlikely]] {
		
			// Set index to the first entry
			index = 0;
		}
	}
	
	// Set entry at index to one
	entries[index] = {key, 1};
	
	// Return index
	return index;
}

// Replace
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline uint32_t HashTable<size, isSet>::replace(const uint32_t key, const uint32_t value) noexcept {

	// Throw error if is set
	static_assert(!isSet, "Replace can't be used with a set");
	
	// Get key's index
	uint32_t index = key % bit_ceil(size + 1);
	
	// Loop while entry at index exists
	while(entries[index].value != NO_VALUE) [[unlikely]] {
	
		// Check if entry has the same key
		if(entries[index].key == key) [[likely]] {
		
			// Get current value
			const uint32_t currentValue = entries[index].value;
			
			// Set entry at index to the value
			entries[index].value = value;
			
			// Return current value
			return currentValue;
		}
		
		// Check if index isn't for the last entry
		if(index != bit_ceil(size + 1) - 1) [[likely]] {
		
			// Increment index
			++index;
		}
		
		// Otherwise
		else [[unlikely]] {
		
			// Set index to the first entry
			index = 0;
		}
	}
	
	// Set entry at index to the value
	entries[index] = {key, value};
	
	// Return no value
	return NO_VALUE;
}

// Remove most recent set unique
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline void HashTable<size, isSet>::removeMostRecentSetUique(const uint32_t index) noexcept {

	// Set entry at index to no value
	__builtin_assume(index < (static_cast<uint32_t>(1) << (sizeof(uint32_t) * BITS_IN_A_BYTE - __builtin_clz(size))));
	entries[index].value = NO_VALUE;
}

// Clear
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline void HashTable<size, isSet>::clear() noexcept {

	// Check if is set
	if(isSet) [[likely]] {
	
		// Set entries to no value
		__builtin_memset_inline(entries.get(), NO_VALUE, bit_ceil(size + 1) * sizeof(HashTableEntry));
	}
	
	// Otherwise
	else [[unlikely]] {
	
		// Set entries to no value
		__builtin_memset(entries.get(), NO_VALUE, bit_ceil(size + 1) * sizeof(HashTableEntry));
	}
}

// Contains
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline bool HashTable<size, isSet>::contains(const uint32_t key) const noexcept {

	// Go through all existing entries starting at the key's index
	for(uint32_t index = key % bit_ceil(size + 1); entries[index].value != NO_VALUE;) [[unlikely]] {
	
		// Check if entry has the same key
		if(entries[index].key == key) [[likely]] {
		
			// Return true
			return true;
		}
		
		// Check if index isn't for the last entry
		if(index != bit_ceil(size + 1) - 1) [[likely]] {
		
			// Increment index
			++index;
		}
		
		// Otherwise
		else [[unlikely]] {
		
			// Set index to the first entry
			index = 0;
		}
	}
	
	// Return false
	return false;
}

// Get
template<const uint32_t size, const bool isSet> __attribute__((always_inline)) inline uint32_t HashTable<size, isSet>::get(const uint32_t key) const noexcept {

	// Throw error if is set
	static_assert(!isSet, "Get can't be used with a set");
	
	// Go through all existing entries starting at the key's index
	for(uint32_t index = key % bit_ceil(size + 1); entries[index].value != NO_VALUE;) [[unlikely]] {
	
		// Check if entry has the same key
		if(entries[index].key == key) [[likely]] {
		
			// Return entry's value
			return entries[index].value;
		}
		
		// Check if index isn't for the last entry
		if(index != bit_ceil(size + 1) - 1) [[likely]] {
		
			// Increment index
			++index;
		}
		
		// Otherwise
		else [[unlikely]] {
		
			// Set index to the first entry
			index = 0;
		}
	}
	
	// Return no value
	return NO_VALUE;
}


#endif
