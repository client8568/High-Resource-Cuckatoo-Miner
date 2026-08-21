// Header guard
#ifndef SIPHASH24_H
#define SIPHASH24_H


// Header files
#include "common.h"

using namespace std;


// Constants

// Number of SipHash keys
#define NUMBER_OF_SIPHASH_KEYS 4

// SipRound rotation
#define SIP_ROUND_ROTATION 21


// Function prototypes

// SipHash-2-4
__attribute__((always_inline)) static inline uint32_t sipHash24(const uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &sipHashKeys, const uint64_t nonce) noexcept;

// SipRound
__attribute__((always_inline)) static inline void sipRound(uint64_t states[NUMBER_OF_SIPHASH_KEYS]) noexcept;


// Supporting function implementation

// SipHash-2-4
__attribute__((always_inline)) static inline uint32_t sipHash24(const uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) &sipHashKeys, const uint64_t nonce) noexcept {

	// Initialize states from SipHash keys
	uint64_t states[NUMBER_OF_SIPHASH_KEYS] = {sipHashKeys[0], sipHashKeys[1], sipHashKeys[2], sipHashKeys[3]};
	
	// Perform hash on states
	__builtin_assume(nonce < NUMBER_OF_EDGES * 2 - 1);
	states[3] ^= nonce;
	sipRound(states);
	sipRound(states);
	__builtin_assume(nonce < NUMBER_OF_EDGES * 2 - 1);
	states[0] ^= nonce;
	states[2] ^= 255;
	sipRound(states);
	sipRound(states);
	sipRound(states);
	states[0] += states[1];
	states[2] += states[3];
	states[1] = __builtin_rotateleft64(states[1], 13);
	states[3] = __builtin_rotateleft64(states[3], 16);
	states[1] ^= states[0];
	states[3] ^= states[2];
	states[2] += states[1];
	states[1] = __builtin_rotateleft64(states[1], 17);
	states[3] = __builtin_rotateleft64(states[3], SIP_ROUND_ROTATION);
	
	// Return result
	return states[1] ^ states[2] ^ (states[2] >> (sizeof(uint32_t) * BITS_IN_A_BYTE)) ^ states[3];
}

// SipRound
__attribute__((always_inline)) static inline void sipRound(uint64_t states[NUMBER_OF_SIPHASH_KEYS]) noexcept {

	// Perform SipRound on states
	states[0] += states[1];
	states[2] += states[3];
	states[1] = __builtin_rotateleft64(states[1], 13);
	states[3] = __builtin_rotateleft64(states[3], 16);
	states[1] ^= states[0];
	states[3] ^= states[2];
	states[0] = __builtin_rotateleft64(states[0], 32);
	states[2] += states[1];
	states[0] += states[3];
	states[1] = __builtin_rotateleft64(states[1], 17);
	states[3] = __builtin_rotateleft64(states[3], SIP_ROUND_ROTATION);
	states[1] ^= states[2];
	states[3] ^= states[0];
	states[2] = __builtin_rotateleft64(states[2], 32);
}


#endif
