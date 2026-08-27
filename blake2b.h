// Header guard
#ifndef BLAKE2B_H
#define BLAKE2B_H


// Header files
#include "common.h"
#include "siphash24.h"

using namespace std;


// Constants

// BLAKE2b buffer size
#define BLAKE2B_BUFFER_SIZE 128

// BLAKE2b number of rounds
#define BLAKE2B_NUMBER_OF_ROUNDS 12

// BLAKE2b initial state
static constexpr const uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) BLAKE2B_INITIAL_STATE[] = {

	{0x6A09E667F2BDC928, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1},
	{0x510E527FADE682D1, 0x9B05688C2B3E6C1F, 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179},
	{0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1},
	{0x510E527FADE68251, 0x9B05688C2B3E6C1F, 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179}
};

// BLAKE2b sigma
static constexpr const int BLAKE2B_SIGMA[][16] = {

	{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
	{14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
	{11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
	{7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
	{9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
	{2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
	{12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
	{13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
	{6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
	{10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
	{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
	{14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}
};


// Function prototypes

// BLAKE2b
__attribute__((always_inline)) static inline void blake2b(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &result, const uint8_t header[HEADER_SIZE_EXCLUDING_NONCE], NonceType nonce) noexcept;

// BLAKE2b step
__attribute__((always_inline)) static inline void blake2bStep(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ a, uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ b, uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ c, uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ d, const uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ x, const uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ y) noexcept;


// Supporting function implementation

// BLAKE2b
__attribute__((always_inline)) static inline void blake2b(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &result, const uint8_t header[HEADER_SIZE_EXCLUDING_NONCE], NonceType nonce) noexcept {

	// Set buffer to beginning of header
	uint64_t buffer[BLAKE2B_BUFFER_SIZE / sizeof(uint64_t)];
	__builtin_memcpy_inline(buffer, header, min(sizeof(buffer), static_cast<size_t>(HEADER_SIZE_EXCLUDING_NONCE)));
	
	// Set initial states
	uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) a = BLAKE2B_INITIAL_STATE[0];
	uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) b = BLAKE2B_INITIAL_STATE[1];
	uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) c = BLAKE2B_INITIAL_STATE[2];
	uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) x;
	uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) y;
	
	// Check if header and nonce can fit in one BLAKE2b buffer
	#if HEADER_SIZE_EXCLUDING_NONCE + NONCE_SIZE <= BLAKE2B_BUFFER_SIZE
	
		// Set initial states
		uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) d = {BLAKE2B_INITIAL_STATE[3][0] ^ (sizeof(buffer) ^ (HEADER_SIZE_EXCLUDING_NONCE + sizeof(nonce))), BLAKE2B_INITIAL_STATE[3][1], BLAKE2B_INITIAL_STATE[3][2] ^ UINT64_MAX, BLAKE2B_INITIAL_STATE[3][3]};
		
		// Check if nonce is big endian in the header
		#if NONCE_IN_HEADER_IS_BIG_ENDIAN
		
			// Put nonce in big endian format
			#if NONCE_SIZE == 2
				nonce = __builtin_bswap16(nonce);
			#elif NONCE_SIZE == 4
				nonce = __builtin_bswap32(nonce);
			#elif NONCE_SIZE == 8
				nonce = __builtin_bswap64(nonce);
			#endif
		#endif
		
		// Append nonce to buffer
		__builtin_memcpy_inline(&reinterpret_cast<uint8_t *>(buffer)[HEADER_SIZE_EXCLUDING_NONCE], &nonce, sizeof(nonce));
		
		// Pad remainder of buffer with zeros
		__builtin_memset_inline(&reinterpret_cast<uint8_t *>(buffer)[HEADER_SIZE_EXCLUDING_NONCE + sizeof(nonce)], 0, sizeof(buffer) - (HEADER_SIZE_EXCLUDING_NONCE + sizeof(nonce)));
		
	// Otherwise
	#else
	
		// Set initial states
		uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) d = BLAKE2B_INITIAL_STATE[3];
		
		// Go through all rounds
		#pragma clang loop unroll(full)
		for(int i = 0; i < BLAKE2B_NUMBER_OF_ROUNDS; ++i) [[likely]] {
		
			// Set x and y for column step
			x = {buffer[BLAKE2B_SIGMA[i][0]], buffer[BLAKE2B_SIGMA[i][2]], buffer[BLAKE2B_SIGMA[i][4]], buffer[BLAKE2B_SIGMA[i][6]]};
			y = {buffer[BLAKE2B_SIGMA[i][1]], buffer[BLAKE2B_SIGMA[i][3]], buffer[BLAKE2B_SIGMA[i][5]], buffer[BLAKE2B_SIGMA[i][7]]};
			
			// Perform column step
			blake2bStep(a, b, c, d, x, y);
			
			// Update b, c, and d for diagonal step
			b = __builtin_shufflevector(b, b, 1, 2, 3, 0);
			c = __builtin_shufflevector(c, c, 2, 3, 0, 1);
			d = __builtin_shufflevector(d, d, 3, 0, 1, 2);
			
			// Set x, and y for diagonal step
			x = {buffer[BLAKE2B_SIGMA[i][8]], buffer[BLAKE2B_SIGMA[i][10]], buffer[BLAKE2B_SIGMA[i][12]], buffer[BLAKE2B_SIGMA[i][14]]};
			y = {buffer[BLAKE2B_SIGMA[i][9]], buffer[BLAKE2B_SIGMA[i][11]], buffer[BLAKE2B_SIGMA[i][13]], buffer[BLAKE2B_SIGMA[i][15]]};
			
			// Perform diagonal step
			blake2bStep(a, b, c, d, x, y);
			
			// Update b, c, and d for column step
			b = __builtin_shufflevector(b, b, 3, 0, 1, 2);
			c = __builtin_shufflevector(c, c, 2, 3, 0, 1);
			d = __builtin_shufflevector(d, d, 1, 2, 3, 0);
		}
		
		// Set buffer to end of header
		__builtin_memcpy_inline(buffer, &header[sizeof(buffer)], HEADER_SIZE_EXCLUDING_NONCE - sizeof(buffer));
		
		// Check if nonce is big endian in the header
		#if NONCE_IN_HEADER_IS_BIG_ENDIAN
		
			// Put nonce in big endian format
			#if NONCE_SIZE == 2
				nonce = __builtin_bswap16(nonce);
			#elif NONCE_SIZE == 4
				nonce = __builtin_bswap32(nonce);
			#elif NONCE_SIZE == 8
				nonce = __builtin_bswap64(nonce);
			#endif
		#endif
		
		// Append nonce to buffer
		__builtin_memcpy_inline(&reinterpret_cast<uint8_t *>(buffer)[HEADER_SIZE_EXCLUDING_NONCE - sizeof(buffer)], &nonce, sizeof(nonce));
		
		// Pad remainder of buffer with zeros
		__builtin_memset_inline(&reinterpret_cast<uint8_t *>(buffer)[HEADER_SIZE_EXCLUDING_NONCE - sizeof(buffer) + sizeof(nonce)], 0, sizeof(buffer) - (HEADER_SIZE_EXCLUDING_NONCE - sizeof(buffer) + sizeof(nonce)));
		
		// Updates states
		result = a ^= BLAKE2B_INITIAL_STATE[0] ^ c;
		b ^= BLAKE2B_INITIAL_STATE[1] ^ d;
		c = BLAKE2B_INITIAL_STATE[2];
		d = {BLAKE2B_INITIAL_STATE[3][0] ^ (sizeof(buffer) ^ (HEADER_SIZE_EXCLUDING_NONCE + sizeof(nonce))), BLAKE2B_INITIAL_STATE[3][1], BLAKE2B_INITIAL_STATE[3][2] ^ UINT64_MAX, BLAKE2B_INITIAL_STATE[3][3]};
	#endif
	
	// Go through all but the last round
	#pragma clang loop unroll(full)
	for(int i = 0; i < BLAKE2B_NUMBER_OF_ROUNDS - 1; ++i) [[likely]] {
	
		// Set x and y for column step
		x = {buffer[BLAKE2B_SIGMA[i][0]], buffer[BLAKE2B_SIGMA[i][2]], buffer[BLAKE2B_SIGMA[i][4]], buffer[BLAKE2B_SIGMA[i][6]]};
		y = {buffer[BLAKE2B_SIGMA[i][1]], buffer[BLAKE2B_SIGMA[i][3]], buffer[BLAKE2B_SIGMA[i][5]], buffer[BLAKE2B_SIGMA[i][7]]};
		
		// Perform column step
		blake2bStep(a, b, c, d, x, y);
		
		// Update b, c, and d for diagonal step
		b = __builtin_shufflevector(b, b, 1, 2, 3, 0);
		c = __builtin_shufflevector(c, c, 2, 3, 0, 1);
		d = __builtin_shufflevector(d, d, 3, 0, 1, 2);
		
		// Set x, and y for diagonal step
		x = {buffer[BLAKE2B_SIGMA[i][8]], buffer[BLAKE2B_SIGMA[i][10]], buffer[BLAKE2B_SIGMA[i][12]], buffer[BLAKE2B_SIGMA[i][14]]};
		y = {buffer[BLAKE2B_SIGMA[i][9]], buffer[BLAKE2B_SIGMA[i][11]], buffer[BLAKE2B_SIGMA[i][13]], buffer[BLAKE2B_SIGMA[i][15]]};
		
		// Perform diagonal step
		blake2bStep(a, b, c, d, x, y);
		
		// Update b, c, and d for column step
		b = __builtin_shufflevector(b, b, 3, 0, 1, 2);
		c = __builtin_shufflevector(c, c, 2, 3, 0, 1);
		d = __builtin_shufflevector(d, d, 1, 2, 3, 0);
	}
	
	// Set x and y for column step
	x = {buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][0]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][2]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][4]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][6]]};
	y = {buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][1]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][3]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][5]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][7]]};
	
	// Perform column step
	blake2bStep(a, b, c, d, x, y);
	
	// Update b, c, and d for diagonal step
	b = __builtin_shufflevector(b, b, 1, 2, 3, 0);
	c = __builtin_shufflevector(c, c, 2, 3, 0, 1);
	d = __builtin_shufflevector(d, d, 3, 0, 1, 2);
	
	// Set x, and y for diagonal step
	x = {buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][8]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][10]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][12]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][14]]};
	y = {buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][9]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][11]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][13]], buffer[BLAKE2B_SIGMA[BLAKE2B_NUMBER_OF_ROUNDS - 1][15]]};
	
	// Perform diagonal step
	a += b + x;
	d ^= a;
	d = (d >> 32) | (d << ((sizeof(uint64_t) * BITS_IN_A_BYTE) - 32));
	c += d;
	b ^= c;
	b = (b >> 24) | (b << ((sizeof(uint64_t) * BITS_IN_A_BYTE) - 24));
	a += b + y;
	d ^= a;
	d = (d >> 16) | (d << ((sizeof(uint64_t) * BITS_IN_A_BYTE) - 16));
	c += d;
	
	// Update c for column step
	c = __builtin_shufflevector(c, c, 2, 3, 0, 1);
	
	// Check if header and nonce can fit in one BLAKE2b buffer
	#if HEADER_SIZE_EXCLUDING_NONCE + NONCE_SIZE <= BLAKE2B_BUFFER_SIZE
	
		// Get result
		result = a ^ BLAKE2B_INITIAL_STATE[0] ^ c;
		
	// Otherwise
	#else
	
		// Get result
		result ^= a ^ c;
	#endif
}

// BLAKE2b step
__attribute__((always_inline)) static inline void blake2bStep(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ a, uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ b, uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ c, uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ d, const uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ x, const uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &__restrict__ y) noexcept {

	// Perform step on values
	a += b + x;
	d ^= a;
	d = (d >> 32) | (d << ((sizeof(uint64_t) * BITS_IN_A_BYTE) - 32));
	c += d;
	b ^= c;
	b = (b >> 24) | (b << ((sizeof(uint64_t) * BITS_IN_A_BYTE) - 24));
	a += b + y;
	d ^= a;
	d = (d >> 16) | (d << ((sizeof(uint64_t) * BITS_IN_A_BYTE) - 16));
	c += d;
	b ^= c;
	b = (b >> 63) | (b << ((sizeof(uint64_t) * BITS_IN_A_BYTE) - 63));
}


#endif
