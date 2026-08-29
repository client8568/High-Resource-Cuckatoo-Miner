// Header files
#include "common.h"
#include <algorithm>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <execution>
#include <functional>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <new>
#include <random>
#include <semaphore>
#include "bitmap.h"
#include "blake2b.h"
#include "cuckatoo.h"
#include "hash_table.h"
#include "siphash24.h"

// Check if using an Apple device
#ifdef __APPLE__

	// Metal target version (Metal v4.0)
	#define METAL_TARGET_VERSION MTL::LanguageVersion4_0
	
	// Header files
	#include "metal/metal.h"
	
// Otherwise
#else

	// OpenCL target version (OpenCL v1.2)
	#define CL_TARGET_OPENCL_VERSION 120
	
	// Header files
	#include <CL/cl.h>
#endif

using namespace std;


// Constants

// GPU number of coarse buckets per dimension
#define GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION (1 << GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)

// GPU max number of edges per coarse bucket after trimming round
static constexpr const array GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND = []() __attribute__((always_inline)) constexpr noexcept {

	// Create GPU max number of edges per coarse bucket after trimming round
	array<uint32_t, GPU_TRIMMING_ROUNDS> gpuMaxNumberOfEdgesPerCoarseBucketAfterTrimmingRound;
	
	// Go through all GPU trimming rounds
	for(int i = 0; i < GPU_TRIMMING_ROUNDS; ++i) [[likely]] {
	
		// Set the GPU max number of edges per coarse bucket after the trimming round
		gpuMaxNumberOfEdgesPerCoarseBucketAfterTrimmingRound[i] = ceilAsUint32(static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(i, false)) / GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
	}
	
	// Return GPU max number of edges per coarse bucket after trimming round
	return gpuMaxNumberOfEdgesPerCoarseBucketAfterTrimmingRound;
}();

// Check if using max RAM for GPU trimming
#if GPU_TRIMMING_USE_MAX_RAM

	// GPU coarse bucket item size
	#define GPU_COARSE_BUCKET_ITEM_SIZE (sizeof(uint32_t) + sizeof(uint32_t))
	
// Otherwise
#else

	// GPU coarse bucket item size
	#define GPU_COARSE_BUCKET_ITEM_SIZE sizeof(uint32_t)
#endif

// Check if using more RAM for GPU trimming
#if GPU_TRIMMING_USE_MORE_RAM

	// GPU max number of edges per coarse bucket
	#define GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET static_cast<uint32_t>((ceilAsUint32(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[0] * (PERCENT_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND * (1 + NUMBER_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND_ADDITIONAL_TOLERANCE_PERCENT) * ((sizeof(uint32_t) + sizeof(uint32_t)) / GPU_COARSE_BUCKET_ITEM_SIZE))) + (hardware_destructive_interference_size / GPU_COARSE_BUCKET_ITEM_SIZE - 1)) & ~(hardware_destructive_interference_size / GPU_COARSE_BUCKET_ITEM_SIZE - 1))
	
// Otherwise
#else

	// GPU max number of edges per coarse bucket
	#define GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET static_cast<uint32_t>((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[0] + (hardware_destructive_interference_size / GPU_COARSE_BUCKET_ITEM_SIZE - 1)) & ~(hardware_destructive_interference_size / GPU_COARSE_BUCKET_ITEM_SIZE - 1))
#endif

// GPU number of initial fine buckets per dimension
#define GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION (1 << GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING)

// GPU number of fine buckets per dimension
#define GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION (1 << GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING)

// Check if using min RAM for GPU trimming or using less RAM for GPU trimming
#if GPU_TRIMMING_USE_MIN_RAM || GPU_TRIMMING_USE_LESS_RAM

	// GPU fine bucket item size
	#define GPU_FINE_BUCKET_ITEM_SIZE sizeof(uint32_t)
	
// Otherwise
#else

	// GPU fine bucket item size
	#define GPU_FINE_BUCKET_ITEM_SIZE (sizeof(uint32_t) + sizeof(uint32_t))
#endif

// Check if using less RAM for GPU trimming
#if GPU_TRIMMING_USE_LESS_RAM

	// GPU max number of edges per initial fine bucket
	#define GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET static_cast<uint32_t>((ceilAsUint32(ceilAsUint32(static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(0)) / (GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION)) * (PERCENT_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND * (1 + NUMBER_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND_ADDITIONAL_TOLERANCE_PERCENT) * ((sizeof(uint32_t) + sizeof(uint32_t)) / GPU_FINE_BUCKET_ITEM_SIZE))) + (hardware_destructive_interference_size / GPU_FINE_BUCKET_ITEM_SIZE - 1)) & ~(hardware_destructive_interference_size / GPU_FINE_BUCKET_ITEM_SIZE - 1))
	
	// GPU max number of edges per fine bucket
	#define GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET static_cast<uint32_t>((ceilAsUint32(ceilAsUint32(static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(0)) / (GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION)) * (PERCENT_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND * (1 + NUMBER_OF_EDGES_REMAINING_AFTER_ONE_TRIMMING_ROUND_ADDITIONAL_TOLERANCE_PERCENT) * ((sizeof(uint32_t) + sizeof(uint32_t)) / GPU_FINE_BUCKET_ITEM_SIZE))) + (hardware_destructive_interference_size / GPU_FINE_BUCKET_ITEM_SIZE - 1)) & ~(hardware_destructive_interference_size / GPU_FINE_BUCKET_ITEM_SIZE - 1))
	
// Otherwise
#else

	// GPU max number of edges per initial fine bucket
	#define GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET static_cast<uint32_t>((ceilAsUint32(static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(0)) / (GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION)) + (hardware_destructive_interference_size / GPU_FINE_BUCKET_ITEM_SIZE - 1)) & ~(hardware_destructive_interference_size / GPU_FINE_BUCKET_ITEM_SIZE - 1))
	
	// GPU max number of edges per fine bucket
	#define GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET static_cast<uint32_t>((ceilAsUint32(static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(0)) / (GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION)) + (hardware_destructive_interference_size / GPU_FINE_BUCKET_ITEM_SIZE - 1)) & ~(hardware_destructive_interference_size / GPU_FINE_BUCKET_ITEM_SIZE - 1))
#endif

// GPU number of least significant bits ignored during fine bucket sorting
#define GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING (EDGE_BITS - GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING - min(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING, GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING))

// GPU bitmap size
#define GPU_BITMAP_SIZE ((1 << GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) / BITS_IN_A_BYTE)

// CPU number of most significant bits used for coarse bucket sorting
#define CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING

// CPU number of coarse buckets per dimension
#define CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION (1 << CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)

// CPU number of least significant bits ignored during coarse bucket sorting
#define CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING (EDGE_BITS - CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)

// CPU coarse bucket item size
#define CPU_COARSE_BUCKET_ITEM_SIZE (sizeof(uint32_t) + sizeof(uint32_t))

// CPU max number of edges per coarse bucket
#define CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET static_cast<uint32_t>((ceilAsUint32(static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(GPU_TRIMMING_ROUNDS, false)) / (CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)) + (hardware_destructive_interference_size / CPU_COARSE_BUCKET_ITEM_SIZE - 1)) & ~(hardware_destructive_interference_size / CPU_COARSE_BUCKET_ITEM_SIZE - 1))

// CPU coarse bucket index mask
#define CPU_COARSE_BUCKET_INDEX_MASK (CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION - 1)

// CPU number of fine buckets per dimension
#define CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION (1 << CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING)

// CPU number of least significant bits ignored during fine bucket sorting
#define CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING (EDGE_BITS - CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING - CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING)

// CPU max number of edges per fine bucket
#define CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET ceilAsUint32(static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(GPU_TRIMMING_ROUNDS, false)) / (CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION))

// CPU fine bucket index mask
#define CPU_FINE_BUCKET_INDEX_MASK (CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION - 1)

// CPU trimming number of items per bitmap
#define CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP (1 << CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING)

// CPU trimming bitmap item mask
#define CPU_TRIMMING_BITMAP_ITEM_MASK (CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP - 1)

// Trimming rounds before compressing
#define TRIMMING_ROUNDS_BEFORE_COMPRESSING (GPU_TRIMMING_ROUNDS + CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING)

// CPU number of items per compressed lookup table first partition
#define CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION static_cast<uint32_t>((ceilAsUint32((static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(TRIMMING_ROUNDS_BEFORE_COMPRESSING + 1)) / 2) / CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) + (hardware_destructive_interference_size / sizeof(uint32_t) - 1)) & ~(hardware_destructive_interference_size / sizeof(uint32_t) - 1))

// CPU number of items per compressed lookup table second partition
#define CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION static_cast<uint32_t>((ceilAsUint32((static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(TRIMMING_ROUNDS_BEFORE_COMPRESSING + 2)) / 2) / CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) + (hardware_destructive_interference_size / sizeof(uint32_t) - 1)) & ~(hardware_destructive_interference_size / sizeof(uint32_t) - 1))

// CPU max compressed lookup table key
#define CPU_MAX_COMPRESSED_LOOKUP_TABLE_KEY ((UINT16_MAX - (0x0101 + 1)) >> 1)

// CPU compressed item size
#define CPU_COMPRESSED_ITEM_SIZE sizeof(uint16_t)

// CPU compressed item mask
#define CPU_COMPRESSED_ITEM_MASK UINT16_MAX

// CPU number of items per compressed bitmap
#define CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_BITMAP ((CPU_MAX_COMPRESSED_LOOKUP_TABLE_KEY + 1) << 1)

// Check if using max RAM for CPU trimming
#if CPU_TRIMMING_USE_MAX_RAM

	// CPU number of trimming rounds before shrinking coarse buckets
	#define CPU_NUMBER_OF_TRIMMING_ROUNDS_BEFORE_SHRINKING_COARSE_BUCKETS (GPU_TRIMMING_ROUNDS + 1)
	
	// CPU shrunk coarse bucket item size
	#define CPU_SHRUNK_COARSE_BUCKET_ITEM_SIZE CPU_COARSE_BUCKET_ITEM_SIZE
	
// Otherwise check if using more RAM for CPU trimming
#elif CPU_TRIMMING_USE_MORE_RAM

	// CPU number of trimming rounds before shrinking coarse buckets
	#define CPU_NUMBER_OF_TRIMMING_ROUNDS_BEFORE_SHRINKING_COARSE_BUCKETS (GPU_TRIMMING_ROUNDS + 2)
	
	// CPU shrunk coarse bucket item size
	#define CPU_SHRUNK_COARSE_BUCKET_ITEM_SIZE CPU_COARSE_BUCKET_ITEM_SIZE
	
// Otherwise
#else

	// CPU number of trimming rounds before shrinking coarse buckets
	#define CPU_NUMBER_OF_TRIMMING_ROUNDS_BEFORE_SHRINKING_COARSE_BUCKETS (TRIMMING_ROUNDS_BEFORE_COMPRESSING + 2)
	
	// CPU shrunk coarse bucket item size
	#define CPU_SHRUNK_COARSE_BUCKET_ITEM_SIZE (CPU_COMPRESSED_ITEM_SIZE * 2)
#endif

// CPU max number of edges per coarse bucket before shrinking coarse buckets
#define CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS static_cast<uint32_t>((ceilAsUint32(static_cast<double>(maxNumberOfEdgesRemainingAfterTimmingRounds(CPU_NUMBER_OF_TRIMMING_ROUNDS_BEFORE_SHRINKING_COARSE_BUCKETS, false)) / (CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)) + (hardware_destructive_interference_size / CPU_SHRUNK_COARSE_BUCKET_ITEM_SIZE - 1)) & ~(hardware_destructive_interference_size / CPU_SHRUNK_COARSE_BUCKET_ITEM_SIZE - 1))

// GPU number of recovering edges
#define GPU_NUMBER_OF_RECOVERING_EDGES ((NUMBER_OF_EDGES - static_cast<uint64_t>(NUMBER_OF_EDGES * CPU_RECOVERING_PERCENT) + GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM * GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / (GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM * GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM * GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)

// Max number of CPU searching threads
#define MAX_NUMBER_OF_CPU_SEARCHING_THREADS 4

// CPU searching threads first edge percent
static constexpr const double CPU_SEARCHING_THREADS_FIRST_EDGE_PERCENT[][MAX_NUMBER_OF_CPU_SEARCHING_THREADS] = {

	// One searching thread
	{
	
		// First thread
		0
	},
	
	// Two searching threads
	{
	
		// First thread
		0,
		
		// Second thread
		0.9188
	},
	
	// Three searching threads
	{
	
		// First thread
		0,
		
		// Second thread
		0.8645,
		
		// Third thread
		0.9555
	},
	
	// Four searching threads
	{
	
		// First thread
		0,
		
		// Second thread
		0.8217,
		
		// Third thread
		0.9204,
		
		// Fourth thread
		0.9692
	}
};

// CPU number of recovering edges
#define CPU_NUMBER_OF_RECOVERING_EDGES (NUMBER_OF_EDGES - GPU_NUMBER_OF_RECOVERING_EDGES)

// CPU number of items per recovering bitmap
#define CPU_NUMBER_OF_ITEMS_PER_RECOVERING_BITMAP (static_cast<uint32_t>(1) << CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP)

// CPU number of least significant bits ignored in recovering bitmap
#define CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_IN_RECOVERING_BITMAP (EDGE_BITS - CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP)

// Node mask
#define NODE_MASK (UINT32_MAX >> (sizeof(uint32_t) * BITS_IN_A_BYTE - EDGE_BITS))


// Structures

// Trim edges parameters structure
struct TrimEdgesParameters {

	// SipHash keys
	uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) sipHashKeys;
};


// Checks

// Check if compiler isn't clang
#ifndef __clang__

	// Throw error
	#error Compiler isn't Clang
#endif

// Throw error if host isn't little endian
static_assert(endian::native == endian::little, "Host isn't little endian");

// Throw error if host isn't at least 64-bit
static_assert(sizeof(size_t) >= sizeof(uint64_t), "Host isn't at least 64-bit");

// Throw error if edge bits is invalid
static_assert(EDGE_BITS >= MIN_EDGE_BITS && EDGE_BITS <= MAX_EDGE_BITS, "Edge bits is invalid");

// Throw error if GPU trimming rounds is invalid
static_assert(GPU_TRIMMING_ROUNDS >= 3 && GPU_TRIMMING_ROUNDS <= static_cast<unsigned int>(INT_MAX) + 1, "GPU trimming rounds is invalid");

// Throw error if CPU trimming rounds is invalid
static_assert(CPU_TRIMMING_ROUNDS >= static_cast<uint64_t>(CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING) + 2 && CPU_TRIMMING_ROUNDS <= static_cast<unsigned int>(INT_MAX) * 2, "CPU trimming rounds is invalid");

// Throw error if there's not enough trimming rounds
static_assert(MAX_NUMBER_OF_EDGES_AFTER_TRIMMING <= UINT_MAX / CUCKATOO_NODE_CONNECTIONS_PER_EDGE, "Not enough trimming rounds");

// Throw error if there's too many trimming rounds
static_assert(MAX_NUMBER_OF_EDGES_AFTER_TRIMMING >= SOLUTION_SIZE, "Too many trimming rounds");

// Throw error if solution size is invalid
static_assert(SOLUTION_SIZE > 0 && SOLUTION_SIZE <= INT_MAX && SOLUTION_SIZE % 2 == 0, "Solution size is invalid");

// Throw error if nonce size is invalid
static_assert(NONCE_SIZE == sizeof(uint8_t) || NONCE_SIZE == sizeof(uint16_t) || NONCE_SIZE == sizeof(uint32_t) || NONCE_SIZE == sizeof(uint64_t), "Nonce size is invalid");

// Throw error if header size excluding nonce is invalid
static_assert(HEADER_SIZE_EXCLUDING_NONCE > 0 && HEADER_SIZE_EXCLUDING_NONCE <= BLAKE2B_BUFFER_SIZE * 2 - NONCE_SIZE, "Header size excluding nonce is invalid");

// Throw error if GPU number of most significant bits used for coarse bucket sorting is invalid
static_assert(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING > 0 && GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING < EDGE_BITS / 2, "GPU number of most significant bits used for coarse bucket sorting is invalid");

// Throw error if GPU number of most significant bits used for initial fine bucket sorting is invalid
static_assert(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING > 0 && GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING < EDGE_BITS / 2 && GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING + GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING <= sizeof(uint16_t) * BITS_IN_A_BYTE, "GPU number of most significant bits used for initial fine bucket sorting is invalid");

// Throw error if GPU number of most significant bits used for fine bucket sorting is invalid
static_assert(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING > 0 && GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING < EDGE_BITS / 2 && GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING + GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING <= sizeof(uint16_t) * BITS_IN_A_BYTE, "GPU number of most significant bits used for fine bucket sorting is invalid");

// Throw error if GPU coarse bucket sort edges kernel number of work items per work group is invalid
static_assert(GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP >= GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION && GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= UINT_MAX && has_single_bit(static_cast<unsigned int>(GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU coarse bucket sort edges kernel number of work items per work group is invalid");

// Throw error if GPU fine bucket sort initial edges kernel number of work items per work group is invalid
static_assert(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP >= GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION && GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= UINT_MAX && has_single_bit(static_cast<unsigned int>(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU fine bucket sort initial edges kernel number of work items per work group is invalid");

// Throw error if GPU trim initial edges kernel number of work items per work group is invalid
static_assert(GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP >= GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION && GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= UINT_MAX && has_single_bit(static_cast<unsigned int>(GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU trim initial edges kernel number of work items per work group is invalid");

// Throw error if GPU fine bucket sort intermediate edges kernel number of work items per work group is invalid
static_assert(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP >= GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION && GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= UINT_MAX && has_single_bit(static_cast<unsigned int>(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU fine bucket sort intermediate edges kernel number of work items per work group is invalid");

// Throw error if GPU trim intermediate edges kernel number of work items per work group is invalid
static_assert(GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP >= GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION && GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= UINT_MAX && has_single_bit(static_cast<unsigned int>(GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU trim intermediate edges kernel number of work items per work group is invalid");

// Throw error if GPU fine bucket sort final edges kernel number of work items per work group is invalid
static_assert(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP >= GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION && GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= UINT_MAX && has_single_bit(static_cast<unsigned int>(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU fine bucket sort final edges kernel number of work items per work group is invalid");

// Throw error if GPU trim final edges kernel number of work items per work group is invalid
static_assert(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP >= GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION && GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= UINT_MAX && has_single_bit(static_cast<unsigned int>(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU trim final edges kernel number of work items per work group is invalid");

// Throw error if GPU trim final edges and transfer edges kernel number of work items per work group is invalid
static_assert(GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP >= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION && GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= UINT_MAX && has_single_bit(static_cast<unsigned int>(GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU trim final edges and transfer edges kernel number of work items per work group is invalid");

// Throw error if CPU number of most significant bits used for fine bucket sorting is invalid
static_assert(CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING > 0 && CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING < EDGE_BITS / 2, "CPU number of most significant bits used for fine bucket sorting is invalid");

// Throw error if CPU trimming vector scale factor is invalid
static_assert(CPU_TRIMMING_VECTOR_SCALE_FACTOR > 0 && CPU_TRIMMING_VECTOR_SCALE_FACTOR <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION && has_single_bit(static_cast<unsigned int>(CPU_TRIMMING_VECTOR_SCALE_FACTOR)), "CPU trimming vector scale factor is invalid");

// Throw error if CPU trimming rounds before compressing is invalid
static_assert(CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING >= 2 && CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING <= static_cast<unsigned int>(INT_MAX) * 2, "CPU trimming rounds before compressing is invalid");

// Throw error if there's not enough trimming rounds before compressing
static_assert(CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION <= CPU_MAX_COMPRESSED_LOOKUP_TABLE_KEY + 1, "Not enough trimming rounds before compressing");

// Throw error if GPU recover edges kernel number of edges per work item is invalid
static_assert(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM > 0 && GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM <= GPU_NUMBER_OF_RECOVERING_EDGES && has_single_bit(static_cast<uint32_t>(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM)), "GPU recover edges kernel number of edges per work item is invalid");

// Throw error if GPU recover edges kernel number of work items per work group is invalid
static_assert(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP > 0 && GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP <= GPU_NUMBER_OF_RECOVERING_EDGES / GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM && has_single_bit(static_cast<uint32_t>(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)), "GPU recover edges kernel number of work items per work group is invalid");

// Throw error if GPU recover edges kernel number of recovered edge candidates per work item is invalid
static_assert(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM > 0 && GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM <= GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM && GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM <= UINT16_MAX, "GPU recover edges kernel number of recovered edge candidates per work item is invalid");

// Throw error if CPU recovering percent is invalid
static_assert(CPU_RECOVERING_PERCENT > 0 && CPU_RECOVERING_PERCENT < 1 && GPU_NUMBER_OF_RECOVERING_EDGES > 0 && GPU_NUMBER_OF_RECOVERING_EDGES < NUMBER_OF_EDGES && CPU_NUMBER_OF_RECOVERING_EDGES > 0 && CPU_NUMBER_OF_RECOVERING_EDGES < NUMBER_OF_EDGES, "CPU recovering percent is invalid");

// Throw error if CPU number of most significant bits used for recovering bitmap is invalid
static_assert(CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP > 0 && CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_RECOVERING_BITMAP <= EDGE_BITS, "CPU number of most significant bits used for recovering bitmap is invalid");

// Throw error if CPU recovering vector scale factor is invalid
static_assert(CPU_RECOVERING_VECTOR_SCALE_FACTOR > 0 && CPU_RECOVERING_VECTOR_SCALE_FACTOR <= CPU_NUMBER_OF_RECOVERING_EDGES && has_single_bit(static_cast<uint32_t>(CPU_RECOVERING_VECTOR_SCALE_FACTOR)) && CPU_NUMBER_OF_RECOVERING_EDGES % CPU_RECOVERING_VECTOR_SCALE_FACTOR == 0 && GPU_NUMBER_OF_RECOVERING_EDGES % CPU_RECOVERING_VECTOR_SCALE_FACTOR == 0 && CPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR <= UINT_MAX, "CPU recovering vector scale factor is invalid");

// Throw error if GPU set memory size additional space megabytes is invalid
static_assert(GPU_SET_MEMORY_SIZE_ADDITIONAL_SPACE_MEGABYTES >= 0, "GPU set memory size additional space megabytes is invalid");

// Throw error if stratum server default address is invalid
static_assert(TO_STRING(STRATUM_SERVER_DEFAULT_ADDRESS) && sizeof(TO_STRING(STRATUM_SERVER_DEFAULT_ADDRESS)) >= sizeof('\0') && !TO_STRING(STRATUM_SERVER_DEFAULT_ADDRESS)[sizeof(TO_STRING(STRATUM_SERVER_DEFAULT_ADDRESS)) - sizeof('\0')] && __builtin_strlen(TO_STRING(STRATUM_SERVER_DEFAULT_ADDRESS)) == sizeof(TO_STRING(STRATUM_SERVER_DEFAULT_ADDRESS)) - sizeof('\0'), "Stratum server default address is invalid");

// Throw error if stratum server default port is invalid
static_assert(TO_STRING(STRATUM_SERVER_DEFAULT_PORT) && sizeof(TO_STRING(STRATUM_SERVER_DEFAULT_PORT)) >= sizeof('\0') && !TO_STRING(STRATUM_SERVER_DEFAULT_PORT)[sizeof(TO_STRING(STRATUM_SERVER_DEFAULT_PORT)) - sizeof('\0')] && __builtin_strlen(TO_STRING(STRATUM_SERVER_DEFAULT_PORT)) == sizeof(TO_STRING(STRATUM_SERVER_DEFAULT_PORT)) - sizeof('\0'), "Stratum server default port is invalid");

// Check if using Windows
#ifdef _WIN32

	// Throw error if stratum server read timeout seconds is invalid
	static_assert(STRATUM_SERVER_READ_TIMEOUT_SECONDS > 0 && STRATUM_SERVER_READ_TIMEOUT_SECONDS <= numeric_limits<DWORD>::max(), "Stratum server read timeout seconds is invalid");
	
	// Throw error if stratum server write timeout seconds is invalid
	static_assert(STRATUM_SERVER_WRITE_TIMEOUT_SECONDS > 0 && STRATUM_SERVER_WRITE_TIMEOUT_SECONDS <= numeric_limits<DWORD>::max(), "Stratum server write timeout seconds is invalid");
	
	// Throw error if stratum server reconnect after failure delay seconds is invalid
	static_assert(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS > 0 && STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS <= numeric_limits<DWORD>::max() / MILLISECONDS_IN_A_SECOND && STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND != INFINITE, "Stratum server reconnect after failure delay seconds is invalid");
	
// Otherwise
#else

	// Throw error if stratum server read timeout seconds is invalid
	static_assert(STRATUM_SERVER_READ_TIMEOUT_SECONDS > 0 && STRATUM_SERVER_READ_TIMEOUT_SECONDS <= numeric_limits<decltype(timeval().tv_sec)>::max(), "Stratum server read timeout seconds is invalid");
	
	// Throw error if stratum server write timeout seconds is invalid
	static_assert(STRATUM_SERVER_WRITE_TIMEOUT_SECONDS > 0 && STRATUM_SERVER_WRITE_TIMEOUT_SECONDS <= numeric_limits<decltype(timeval().tv_sec)>::max(), "Stratum server write timeout seconds is invalid");
	
	// Throw error if stratum server reconnect after failure delay seconds is invalid
	static_assert(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS > 0 && STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS <= UINT_MAX, "Stratum server reconnect after failure delay seconds is invalid");
#endif

// Throw error if stratum server receive buffer size kilobytes is invalid
static_assert(STRATUM_SERVER_RECEIVE_BUFFER_SIZE_KILOBYTES > 0 && STRATUM_SERVER_RECEIVE_BUFFER_SIZE_KILOBYTES <= SIZE_MAX / BYTES_IN_A_KILOBYTE, "Stratum server receive buffer size kilobytes is invalid");

// Throw error if stratum server send keep alive request interval seconds is invalid
static_assert(STRATUM_SERVER_SEND_KEEP_ALIVE_REQUEST_INTERVAL_SECONDS > 0 && STRATUM_SERVER_SEND_KEEP_ALIVE_REQUEST_INTERVAL_SECONDS <= chrono::seconds::max().count(), "Stratum server send keep alive request interval seconds is invalid");

// Throw error if starting nonce is invalid
static_assert(STARTING_NONCE >= 0 && STARTING_NONCE <= numeric_limits<NonceType>::max(), "Starting nonce is invalid");

// Throw error if starting header is invalid
static_assert(STARTING_HEADER_SIZE <= HEADER_SIZE_EXCLUDING_NONCE && TO_STRING(STARTING_HEADER) && sizeof(TO_STRING(STARTING_HEADER)) >= sizeof('\0') && !TO_STRING(STARTING_HEADER)[sizeof(TO_STRING(STARTING_HEADER)) - sizeof('\0')] && __builtin_strlen(TO_STRING(STARTING_HEADER)) == sizeof(TO_STRING(STARTING_HEADER)) - sizeof('\0'), "Starting header is invalid");

// Throw error if stop after number of graphs is invalid
static_assert(STOP_AFTER_NUMBER_OF_GRAPHS >= 0 && STOP_AFTER_NUMBER_OF_GRAPHS <= UINT64_MAX, "Stop after number of graphs is invalid");

// Check if using max, more, less, or min RAM for GPU trimming together
#if (GPU_TRIMMING_USE_MAX_RAM && GPU_TRIMMING_USE_MORE_RAM) || (GPU_TRIMMING_USE_MAX_RAM && GPU_TRIMMING_USE_LESS_RAM) || (GPU_TRIMMING_USE_MAX_RAM && GPU_TRIMMING_USE_MIN_RAM) || (GPU_TRIMMING_USE_MORE_RAM && GPU_TRIMMING_USE_LESS_RAM) || (GPU_TRIMMING_USE_MORE_RAM && GPU_TRIMMING_USE_MIN_RAM) || (GPU_TRIMMING_USE_LESS_RAM && GPU_TRIMMING_USE_MIN_RAM)

	// Throw error
	#error GPU trimming use max RAM, GPU trimming use more RAM, GPU trimming use less RAM, and GPU trimming use min RAM are mutually exclusive
#endif

// Check if using max and more RAM for CPU trimming together
#if CPU_TRIMMING_USE_MAX_RAM && CPU_TRIMMING_USE_MORE_RAM

	// Throw error
	#error CPU trimming use max RAM and CPU trimming use more RAM are mutually exclusive
#endif

// Check if displaying tuning times and mining to a stratum server
#if DISPLAY_TUNING_TIMES && MINE_TO_A_STRATUM_SERVER

	// Display message
	#warning Displaying tuning times will slow down your mining rate
#endif

// Check if recovering edges for every graph and mining to a stratum server
#if RECOVER_EDGES_FOR_EVERY_GRAPH && MINE_TO_A_STRATUM_SERVER

	// Display message
	#warning Recovering edges for every graph will slow down your mining rate
#endif

// Check if CPU trimming bounds checking isn't set to avoid conditional statements and mining to a stratum server
#if !CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS && MINE_TO_A_STRATUM_SERVER

	// Display message
	#warning CPU bounds checking not avoiding conditional statements will slow down your mining rate
#endif

// Check if starting nonce is set and mining to a stratum server
#if STARTING_NONCE != 0 && MINE_TO_A_STRATUM_SERVER

	// Display message
	#warning Setting the starting nonce will have no affect while mining to a stratum server
#endif

// Check if starting header is set and mining to a stratum server
#if STARTING_HEADER_SIZE != 0 && MINE_TO_A_STRATUM_SERVER

	// Display message
	#warning Setting the starting header will have no affect while mining to a stratum server
#endif

// Check if stopping after a specified number of graphs and mining to a stratum server
#if STOP_AFTER_NUMBER_OF_GRAPHS != 0 && MINE_TO_A_STRATUM_SERVER

	// Display message
	#warning Stopping after a specified number of graphs will stop mining after awhile
#endif


// Function prototypes

// Main
__attribute__((always_inline)) int main(const int argc, char *argv[]) noexcept;

// Check if mining to a stratum server
#if MINE_TO_A_STRATUM_SERVER

	// Read job message
	__attribute__((always_inline)) static inline bool readJobMessage(const char *__restrict__ message, uint8_t jobHeader[HEADER_SIZE_EXCLUDING_NONCE], uint64_t &__restrict__ jobHeight, uint64_t &__restrict__ jobId) noexcept;
#endif


// Supporting function implementation

// Main
__attribute__((always_inline)) int main(const int argc, char *argv[]) noexcept {

	// Display message
	cout << TO_STRING(NAME) " v" TO_STRING(VERSION) " configured to perform cuckatoo" TO_STRING(EDGE_BITS) " with " TO_STRING(GPU_TRIMMING_ROUNDS) " edge trimming rounds done on the GPU followed by " TO_STRING(CPU_TRIMMING_ROUNDS) " edge trimming rounds done on the CPU followed by searching at most " << MAX_NUMBER_OF_EDGES_AFTER_TRIMMING << " remaining edges on the CPU for a cycle of " TO_STRING(SOLUTION_SIZE) " edges" << endl;
	
	// Check if mining to a stratum server
	#if MINE_TO_A_STRATUM_SERVER
	
		// Create stratum server settings
		const char *stratumServerAddress = TO_STRING(STRATUM_SERVER_DEFAULT_ADDRESS);
		const char *stratumServerPort = TO_STRING(STRATUM_SERVER_DEFAULT_PORT);
		const char *stratumServerUsername = nullptr;
	#endif
	
	// Create block
	{
	
		// Set options
		const option options[] = {
		
			// Version
			{"version", no_argument, nullptr, 'v'},
			
			// Check if mining to a stratum server
			#if MINE_TO_A_STRATUM_SERVER
			
				// Stratum server address
				{"stratum_server_address", required_argument, nullptr, 'a'},
				
				// Stratum server port
				{"stratum_server_port", required_argument, nullptr, 'p'},
				
				// Stratum server username
				{"stratum_server_username", required_argument, nullptr, 'u'},
				
			#endif
			
			// Help
			{"help", no_argument, nullptr, 'h'},
			
			// End
			{}
		};
		
		// Check if mining to a stratum server
		bool displayHelp = false;
		bool helpRequested = false;
		int option;
		#if MINE_TO_A_STRATUM_SERVER
		
			// Go through all options while not displaying help
			while(argv && (option = getopt_long(argc, argv, "va:p:u:h", options, nullptr)) != -1 && !displayHelp) [[likely]] {
			
		// Otherwise
		#else
		
			// Go through all options while not displaying help
			while(argv && (option = getopt_long(argc, argv, "vh", options, nullptr)) != -1 && !displayHelp) [[likely]] {
		#endif
		
			// Check option
			switch(option) {
			
				// Version
				case 'v':
				
					// Return success
					return EXIT_SUCCESS;
					
				// Check if mining to a stratum server
				#if MINE_TO_A_STRATUM_SERVER
				
					// Stratum server address
					case 'a':
					
						// Check if option is invalid
						if(!optarg || !*optarg) [[unlikely]] {
						
							// Display message
							cout << '"' << argv[0] << "\": invalid stratum server address -- '" << (__builtin_expect(optarg != nullptr, true) ? optarg : "") << '\'' << endl;
							
							// Set display help to true
							displayHelp = true;
						}
						
						// Set stratum server address to the option
						stratumServerAddress = optarg;
						
						// Break
						break;
						
					// Stratum server port
					case 'p': {
					
						// Check if option is invalid
						char *end;
						errno = 0;
						const unsigned long optionAsNumber = __builtin_expect(optarg != nullptr, true) ? strtoul(optarg, &end, DECIMAL_NUMBER_BASE) : 0;
						if(!optarg || end == optarg || *end || !isdigit(optarg[0]) || (optarg[0] == '0' && isdigit(optarg[1])) || errno || !optionAsNumber || optionAsNumber > UINT16_MAX) [[unlikely]] {
						
							// Display message
							cout << '"' << argv[0] << "\": invalid stratum server port -- '" << (__builtin_expect(optarg != nullptr, true) ? optarg : "") << '\'' << endl;
							
							// Set display help to true
							displayHelp = true;
						}
						
						// Set stratum server port to the option
						stratumServerPort = optarg;
						
						// Break
						break;
					}
					
					// Stratum server username
					case 'u':
					
						// Check if option is invalid
						if(!optarg || !*optarg) [[unlikely]] {
						
							// Display message
							cout << '"' << argv[0] << "\": invalid stratum server username -- '" << (__builtin_expect(optarg != nullptr, true) ? optarg : "") << '\'' << endl;
							
							// Set display help to true
							displayHelp = true;
						}
						
						// Otherwise
						else [[likely]] {
						
							// Go through all characters in the option
							const char *character = optarg;
							do [[likely]] {
							
								// Check if character is invalid
								__builtin_assume_dereferenceable(character, sizeof(*character));
								if(!isprint(*character) || *character == '"' || *character == '\\') [[unlikely]] {
								
									cout << '"' << argv[0] << "\": invalid stratum server username -- '" << optarg << '\'' << endl;
									
									// Set display help to true
									displayHelp = true;
									
									// Break
									break;
								}
								
								// Go to next character
								++character;
								
							} while(*character);
						}
						
						// Set stratum server username to the option
						stratumServerUsername = optarg;
						
						// Break
						break;
				#endif
				
				// Help
				case 'h':
				
					// Set help requested to true
					helpRequested = true;
					
					// Set display help to true
					displayHelp = true;
					
					// Break
					break;
					
				// Default
				default:
				
					// Set display help to true
					displayHelp = true;
					
					// Break
					break;
			}
		}
		
		// Check if displaying help
		if(displayHelp) [[unlikely]] {
		
			// Display message
			cout << endl << "Usage:" << endl << "\t\"" << (__builtin_expect(argv != nullptr, true) ? argv[0] : "") << "\" [options]" << endl << endl;
			cout << "Options:" << endl;
			cout << "\t-v, --version\t\t\tDisplay version and build information" << endl;
			
			// Check if mining to a stratum server
			#if MINE_TO_A_STRATUM_SERVER
			
				// Display message
				cout << "\t-a, --stratum_server_address\tThe address of the stratum server to connect to (default: " TO_STRING(STRATUM_SERVER_DEFAULT_ADDRESS) ")" << endl;
				cout << "\t-p, --stratum_server_port\tThe port of the stratum server to connect to (default: " TO_STRING(STRATUM_SERVER_DEFAULT_PORT) ")" << endl;
				cout << "\t-u, --stratum_server_username\tThe optional username to use when logging into the stratum server" << endl;
			#endif
			
			// Display message
			cout << "\t-h, --help\t\t\tDisplay help information" << endl;
			
			// Return success if help was requested otherwise return failure
			return __builtin_expect(helpRequested, true) ? EXIT_SUCCESS : EXIT_FAILURE;
		}
	}
	
	// Get number of high performance CPU cores
	const unsigned int numberOfHighPerformanceCpuCores = getNumberOfHighPerformanceCpuCores();
	
	// Set number of CPU trimming threads
	const unsigned int numberOfCpuTrimmingThreads = min(numberOfHighPerformanceCpuCores, static_cast<unsigned int>(CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION));
	
	// Set number of CPU searching threads
	const unsigned int numberOfCpuSearchingThreads = min(max(numberOfHighPerformanceCpuCores - 1, static_cast<unsigned int>(1)), static_cast<unsigned int>(MAX_NUMBER_OF_CPU_SEARCHING_THREADS));
	
	// Set number of CPU recovering threads
	const unsigned int numberOfCpuRecoveringThreads = min(numberOfHighPerformanceCpuCores, static_cast<unsigned int>(CPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR));
	
	// Display message
	cout << "Creating " << numberOfCpuTrimmingThreads << " CPU threads for edge trimming" << endl << "Creating " << numberOfCpuSearchingThreads << " CPU threads for edge searching" << endl << "Creating " << numberOfCpuRecoveringThreads << " CPU threads for edge recovering" << endl;
	
	// Create block
	{
	
		// Set total CPU memory allocated
		const size_t totalCpuMemoryAllocated = MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * NUMBER_OF_EDGE_COMPONENTS * sizeof(uint32_t) + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_SHRUNK_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) * CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) * CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION + (CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint64_t) * CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET + max(CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / 2) * sizeof(uint32_t) + max(CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_BITMAP)) * numberOfCpuTrimmingThreads + (MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE * sizeof(CuckatooNodeConnection) + HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::ALLOCATED_MEMORY_SIZE * 2 + HashTable<SOLUTION_SIZE / 2, true>::ALLOCATED_MEMORY_SIZE * 2) * numberOfCpuSearchingThreads + Bitmap<CPU_NUMBER_OF_ITEMS_PER_RECOVERING_BITMAP>::ALLOCATED_MEMORY_SIZE;
		
		// Display message
		cout << "Allocating " << (static_cast<double>(totalCpuMemoryAllocated) / BYTES_IN_A_GIGABYTE) << " GB of CPU memory" << endl;
	}
	
	// Set CPU threads initialized successfully to true
	alignas(hardware_destructive_interference_size) bool cpuThreadsInitializedSuccessfully = true;
	
	// Set close CPU threads to false
	alignas(hardware_destructive_interference_size) bool closeCpuThreads = false;
	
	// Create CPU trimming threads coarse buckets one
	alignas(hardware_destructive_interference_size) uint64_t (* cpuTrimmingThreadsCoarseBucketsOne)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET];
	
	// Create CPU trimming threads number of edges per coarse bucket one
	alignas(hardware_destructive_interference_size) uint32_t (* cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Create remaining edges
	alignas(hardware_destructive_interference_size) const unique_ptr<uint32_t[], void(*)(uint32_t [])> remainingEdges(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint32_t[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * NUMBER_OF_EDGE_COMPONENTS], [](uint32_t remainingEdges[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * NUMBER_OF_EDGE_COMPONENTS]) __attribute__((always_inline)) noexcept {
	
		// Free remaining edges
		operator delete[](remainingEdges, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
	});
	
	// Create number of remaining edges
	alignas(hardware_destructive_interference_size) uint32_t numberOfRemainingEdges;
	
	// Create CPU trimming threads mutex
	alignas(hardware_destructive_interference_size) mutex cpuTrimmingThreadsMutex;
	
	// Lock CPU trimming threads lock
	unique_lock cpuTrimmingThreadsLock(cpuTrimmingThreadsMutex);
	
	// Set start CPU trimming threads trigger toggle to false
	alignas(hardware_destructive_interference_size) bool startCpuTrimmingThreadsTriggerToggle = false;
	
	// Create start CPU trimming threads conditional variable
	alignas(hardware_destructive_interference_size) condition_variable startCpuTrimmingThreadsConditionalVariable;
	
	// Set CPU trimming threads finished to false
	alignas(hardware_destructive_interference_size) bool cpuTrimmingThreadsFinished = false;
	
	// Create CPU trimming threads finished conditional variable
	alignas(hardware_destructive_interference_size) condition_variable cpuTrimmingThreadsFinishedConditionalVariable;
	
	// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
	#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
	
		// Create CPU trimming threads coarse buckets two
		alignas(hardware_destructive_interference_size) const unique_ptr<uint64_t[][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS], void(*)(uint64_t [][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS])> cpuTrimmingThreadsCoarseBucketsTwo(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint64_t[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS], [](uint64_t cpuTrimmingThreadsCoarseBucketsTwo[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS]) __attribute__((always_inline)) noexcept {
		
			// Free CPU trimming threads coarse buckets two
			operator delete[](cpuTrimmingThreadsCoarseBucketsTwo, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
		});
		
	// Otherwise
	#else
	
		// Create CPU trimming threads coarse buckets two
		alignas(hardware_destructive_interference_size) const unique_ptr<uint32_t[][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS], void(*)(uint32_t [][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS])> cpuTrimmingThreadsCoarseBucketsTwo(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint32_t[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS], [](uint32_t cpuTrimmingThreadsCoarseBucketsTwo[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS]) __attribute__((always_inline)) noexcept {
		
			// Free CPU trimming threads coarse buckets two
			operator delete[](cpuTrimmingThreadsCoarseBucketsTwo, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
		});
	#endif
	
	// Create CPU trimming threads number of edges per coarse bucket two
	alignas(hardware_destructive_interference_size) const unique_ptr<uint32_t[][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION], void(*)(uint32_t [][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION])> cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo(new(static_cast<align_val_t>(alignof(uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))))), nothrow) uint32_t[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION], [](uint32_t cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]) __attribute__((always_inline)) noexcept {
	
		// Free CPU trimming threads number of edges per coarse bucket two
		operator delete[](cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo, static_cast<align_val_t>(alignof(uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))))), nothrow);
	});
	
	// Create CPU trimming threads compressed lookup table first partition
	alignas(hardware_destructive_interference_size) const unique_ptr<uint32_t[][CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION], void(*)(uint32_t [][CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION])> cpuTrimmingThreadsCompressedLookupTableFirstPartition(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint32_t[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION], [](uint32_t cpuTrimmingThreadsCompressedLookupTableFirstPartition[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION]) __attribute__((always_inline)) noexcept {
	
		// Free CPU trimming threads compressed lookup table first partition
		operator delete[](cpuTrimmingThreadsCompressedLookupTableFirstPartition, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
	});
	
	// Create CPU trimming threads compressed lookup table second partition
	alignas(hardware_destructive_interference_size) const unique_ptr<uint32_t[][CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION], void(*)(uint32_t [][CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION])> cpuTrimmingThreadsCompressedLookupTableSecondPartition(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint32_t[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION], [](uint32_t cpuTrimmingThreadsCompressedLookupTableSecondPartition[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION]) __attribute__((always_inline)) noexcept {
	
		// Free CPU trimming threads compressed lookup table second partition
		operator delete[](cpuTrimmingThreadsCompressedLookupTableSecondPartition, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
	});
	
	// Go through all CPU trimming threads
	alignas(hardware_destructive_interference_size) unsigned int numberOfCpuTrimmingThreadsFinished = 0;
	alignas(hardware_destructive_interference_size) barrier cpuTrimmingThreadsBarrier(numberOfCpuTrimmingThreads);
	thread cpuTrimmingThreads[numberOfCpuTrimmingThreads];
	
	__builtin_assume(numberOfCpuTrimmingThreads >= 1 && numberOfCpuTrimmingThreads <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
	for(unsigned int i = 0; i < numberOfCpuTrimmingThreads; ++i) [[likely]] {
	
		// Create CPU trimming thread
		cpuTrimmingThreads[i] = thread([numberOfCpuTrimmingThreads, &cpuThreadsInitializedSuccessfully, &closeCpuThreads, &cpuTrimmingThreadsCoarseBucketsOne, &cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne, remainingEdges = remainingEdges.get(), &numberOfRemainingEdges, &cpuTrimmingThreadsMutex, &startCpuTrimmingThreadsTriggerToggle, &startCpuTrimmingThreadsConditionalVariable, &cpuTrimmingThreadsFinished, &cpuTrimmingThreadsFinishedConditionalVariable, cpuTrimmingThreadsCoarseBucketsTwo = cpuTrimmingThreadsCoarseBucketsTwo.get(), cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo = cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo.get(), cpuTrimmingThreadsCompressedLookupTableFirstPartition = cpuTrimmingThreadsCompressedLookupTableFirstPartition.get(), cpuTrimmingThreadsCompressedLookupTableSecondPartition = cpuTrimmingThreadsCompressedLookupTableSecondPartition.get(), &numberOfCpuTrimmingThreadsFinished, &cpuTrimmingThreadsBarrier, cpuTrimmingThreadIndex = i]() __attribute__((always_inline)) noexcept {
		
			// Lock CPU trimming threads lock
			unique_lock cpuTrimmingThreadsLock(cpuTrimmingThreadsMutex);
			
			// Create fine buckets
			const unique_ptr<uint64_t[][CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET], void(*)(uint64_t [][CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET])> fineBuckets(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint64_t[CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET], [](uint64_t fineBuckets[CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET]) __attribute__((always_inline)) noexcept {
			
				// Free fine buckets
				operator delete[](fineBuckets, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
			});
			
			// Create number of edges per fine bucket
			const unique_ptr<uint32_t[], void(*)(uint32_t [])> numberOfEdgesPerFineBucket(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint32_t[max(CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / 2)], [](uint32_t numberOfEdgesPerFineBucket[max(CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / 2)]) __attribute__((always_inline)) noexcept {
			
				// Free number of edges per fine bucket
				operator delete[](numberOfEdgesPerFineBucket, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
			});
			
			// Create bitmap
			const unique_ptr<uint8_t[], void(*)(uint8_t [])> bitmap(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) uint8_t[max(CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_BITMAP)], [](uint8_t bitmap[max(CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_BITMAP)]) __attribute__((always_inline)) noexcept {
			
				// Free bitmap
				operator delete[](bitmap, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
			});
			
			// Create block
			{
			
				// Set CPU trimming thread initializing failed to if setting this CPU trimming thread's priority and affinity failed or if allocating memory failed
				const bool cpuTrimmingThreadInitializingFailed = !setThreadPriorityAndAffinity(cpuTrimmingThreadIndex) || !remainingEdges || !cpuTrimmingThreadsCoarseBucketsTwo || !cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo || !cpuTrimmingThreadsCompressedLookupTableFirstPartition || !cpuTrimmingThreadsCompressedLookupTableSecondPartition || !fineBuckets || !numberOfEdgesPerFineBucket || !bitmap;
				
				// Check if initializing this CPU trimming thread failed
				if(cpuTrimmingThreadInitializingFailed) [[unlikely]] {
				
					// Set CPU threads initialized successfully to false
					cpuThreadsInitializedSuccessfully = false;
				}
				
				// Check if all CPU trimming threads have finished initializing
				if(++numberOfCpuTrimmingThreadsFinished == numberOfCpuTrimmingThreads) [[unlikely]] {
				
					// Reset number of CPU trimming threads finished
					numberOfCpuTrimmingThreadsFinished = 0;
					
					// Notify that CPU trimming threads have finished initializing
					cpuTrimmingThreadsFinished = true;
					cpuTrimmingThreadsLock.unlock();
					cpuTrimmingThreadsFinishedConditionalVariable.notify_one();
				}
				
				// Otherwise
				else [[likely]] {
				
					// Unlock CPU trimming threads lock
					cpuTrimmingThreadsLock.unlock();
				}
				
				// Check if initializing this CPU trimming thread failed
				if(cpuTrimmingThreadInitializingFailed) [[unlikely]] {
				
					// Return
					return;
				}
				
				// Ensure memory is fully allocated
				setBufferGuaranteed(fineBuckets.get(), 0, CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint64_t) * CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET);
				setBufferGuaranteed(numberOfEdgesPerFineBucket.get(), 0, max(CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / 2) * sizeof(uint32_t));
				setBufferGuaranteed(bitmap.get(), 0, max(CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_BITMAP));
			}
			
			// Loop forever
			const int bucketStart = static_cast<double>(CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) / numberOfCpuTrimmingThreads * cpuTrimmingThreadIndex;
			const int bucketEnd = __builtin_expect(cpuTrimmingThreadIndex != numberOfCpuTrimmingThreads - 1, true) ? static_cast<double>(CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) / numberOfCpuTrimmingThreads * (cpuTrimmingThreadIndex + 1) : CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION;
			uint64_t *nextEdgeIndex[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
			
			for(bool startCpuTrimmingThreadTriggerTrue = true;; startCpuTrimmingThreadTriggerTrue = !startCpuTrimmingThreadTriggerTrue) [[likely]] {
			
				// Wait until starting CPU trimming threads is triggered
				cpuTrimmingThreadsLock.lock();
				startCpuTrimmingThreadsConditionalVariable.wait(cpuTrimmingThreadsLock, [&startCpuTrimmingThreadsTriggerToggle, startCpuTrimmingThreadTriggerTrue]() __attribute__((always_inline)) noexcept -> bool {
				
					// Return if starting CPU trimming threads
					return startCpuTrimmingThreadsTriggerToggle == startCpuTrimmingThreadTriggerTrue;
				});
				
				// Get if closing CPU trimming threads
				const bool closingCpuTrimmingThreads = closeCpuThreads;
				cpuTrimmingThreadsLock.unlock();
				
				// Check if closing CPU trimming threads
				if(closingCpuTrimmingThreads) [[unlikely]] {
				
					// Return
					return;
				}
				
				// Check if displaying tuning times
				#if DISPLAY_TUNING_TIMES
				
					// Check if this is the first CPU trimming thread
					if(!cpuTrimmingThreadIndex) [[unlikely]] {
					
						// Go through all groups of coarse buckets
						uint32_t currentRemainingNumberOfEdges = 0;
						for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
						
							// Add the number of edges in the group's buckets to the current remaining number of edges
							currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						}
						
						// Display message
						cpuTrimmingThreadsLock.lock();
						cout << "Trimming rounds 1 through " TO_STRING(GPU_TRIMMING_ROUNDS) " finished by the GPU with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
						cpuTrimmingThreadsLock.unlock();
					}
					
					// Create start and end time
					chrono::steady_clock::time_point endTime;
					chrono::steady_clock::time_point startTime = chrono::steady_clock::now();
				#endif
				
				// Check if using max RAM for CPU trimming
				#if CPU_TRIMMING_USE_MAX_RAM
				
					// Go through all groups of coarse buckets
					uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR))) largestCoarseBucketSizes;
					for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
					
						// Get the largest coarse bucket size from the group's buckets
						largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
					}
					
					// Get the current largest coarse bucket size from all the coarse buckets
					uint32_t currentLargestCoarseBucketSize = min(__builtin_reduce_max(largestCoarseBucketSizes), CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS);
				#endif
				
				// Go through all rows of this thread's coarse buckets
				__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
				for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
				
					// Set fine bucket's number of edges to zero
					__builtin_memset_inline(numberOfEdgesPerFineBucket.get(), 0, CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t));
					
					// Go through all coarse buckets in the row's column
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Check if using max RAM for CPU trimming
						#if CPU_TRIMMING_USE_MAX_RAM
						
							// Set next edge index to the beginning of the coarse bucket
							nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
							
						// Otherwise
						#else
						
							// Set next edge index to the beginning of the coarse bucket
							nextEdgeIndex[j] = cpuTrimmingThreadsCoarseBucketsOne[j][i];
						#endif
						
						// Go through all edges in the coarse bucket
						for(uint32_t k = 0, l = cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[j][i]; k < l; ++k) [[likely]] {
						
							// Get edge's nodes
							const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[j][i][k];
							
							// Get fine bucket's index from the edge's node in the first partition
							const int fineBucketIndex = (nodes >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & CPU_FINE_BUCKET_INDEX_MASK;
							
							// Check if CPU trimming bounds checking is set to avoid conditional statements
							#if CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS
							
								// Put edge's nodes in the fine bucket at the index
								fineBuckets[fineBucketIndex][numberOfEdgesPerFineBucket[fineBucketIndex]] = nodes;
								
								// Increment the fine bucket's number of edges if it isn't full
								numberOfEdgesPerFineBucket[fineBucketIndex] += numberOfEdgesPerFineBucket[fineBucketIndex] < CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET - 1;
								
							// Otherwise
							#else
							
								// Check if fine bucket at the index isn't full
								if(numberOfEdgesPerFineBucket[fineBucketIndex] < CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) [[likely]] {
								
									// Put edge's nodes in the fine bucket at the index
									fineBuckets[fineBucketIndex][numberOfEdgesPerFineBucket[fineBucketIndex]++] = nodes;
								}
								
								// Otherwise
								else [[unlikely]] {
								
									// Display message
									cpuTrimmingThreadsLock.lock();
									cout << "Lost edge at first partition fine bucket sort" << endl;
									cpuTrimmingThreadsLock.unlock();
								}
							#endif
						}
					}
					
					// Go through all fine buckets
					#if CPU_TRIMMING_USE_MAX_RAM
						uint32_t numberOfEdges[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION] = {};
					#endif
					for(int j = 0; j < CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Clear bitmap
						fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, 0);
						
						// Go through all edges in the fine bucket
						const uint32_t &l = numberOfEdgesPerFineBucket[j];
						for(uint32_t k = 0; k < l; ++k) [[likely]] {
						
							// Enable edge's node in the first partition in the bitmap
							bitmap[fineBuckets[j][k] & CPU_TRIMMING_BITMAP_ITEM_MASK] = 1;
						}
						
						// Go through all edges in the fine bucket
						for(uint32_t k = 0; k < l; ++k) [[likely]] {
						
							// Get edge's nodes
							const uint64_t &nodes = fineBuckets[j][k];
							
							// Get coarse bucket's index from the edge's node in the second partition
							const int coarseBucketIndex = nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE + CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING);
							
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Check if CPU trimming bounds checking is set to avoid conditional statements
								#if CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS
								
									// Put edge's nodes in the coarse bucket at the index
									nextEdgeIndex[coarseBucketIndex][numberOfEdges[coarseBucketIndex]] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
									
									// Increment the number of edges in the coarse bucket if the edge's node in the first partition has a pair in the bitmap and the coarse bucket isn't full
									numberOfEdges[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK] & (numberOfEdges[coarseBucketIndex] < currentLargestCoarseBucketSize - 1);
									
								// Otherwise
								#else
								
									// Check if coarse bucket at the index isn't full
									if(numberOfEdges[coarseBucketIndex] < currentLargestCoarseBucketSize) [[likely]] {
									
										// Put edge's nodes in the coarse bucket at the index
										nextEdgeIndex[coarseBucketIndex][numberOfEdges[coarseBucketIndex]] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
										
										// Increment the number of edges in the coarse bucket if the edge's node in the first partition has a pair in the bitmap
										numberOfEdges[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK];
									}
									
									// Otherwise
									else [[unlikely]] {
									
										// Display message
										cpuTrimmingThreadsLock.lock();
										cout << "Lost edge at second partition coarse bucket sort" << endl;
										cpuTrimmingThreadsLock.unlock();
									}
								#endif
								
							// Otherwise
							#else
							
								// Put edge's nodes in the coarse bucket at the index
								*nextEdgeIndex[coarseBucketIndex] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
								
								// Increment the coarse bucket's next edge index if the edge's node in the first partition has a pair in the bitmap
								nextEdgeIndex[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK];
							#endif
						}
					}
					
					// Go through all coarse buckets in the row's column
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Check if using max RAM for CPU trimming
						#if CPU_TRIMMING_USE_MAX_RAM
						
							// Set coarse bucket's number of edges based on its next edge index
							cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[j][i] = numberOfEdges[j];
							
						// Otherwise
						#else
						
							// Set coarse bucket's number of edges based on its next edge index
							cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[j][i] = nextEdgeIndex[j] - cpuTrimmingThreadsCoarseBucketsOne[j][i];
						#endif
					}
				}
				
				// Synchronize CPU trimming threads
				cpuTrimmingThreadsBarrier.arrive_and_wait();
				
				// Check if displaying tuning times
				#if DISPLAY_TUNING_TIMES
				
					// Get end time
					endTime = chrono::steady_clock::now();
					
					// Check if this is the first CPU trimming thread
					if(!cpuTrimmingThreadIndex) [[unlikely]] {
					
						// Go through all groups of coarse buckets
						uint32_t currentRemainingNumberOfEdges = 0;
						for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
						
							// Add the number of edges in the group's buckets to the current remaining number of edges
							currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						}
						
						// Display message
						cpuTrimmingThreadsLock.lock();
						cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + 1) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
						cpuTrimmingThreadsLock.unlock();
					}
					
					// Get start time
					startTime = chrono::steady_clock::now();
				#endif
				
				// Check if using max RAM for CPU trimming
				#if CPU_TRIMMING_USE_MAX_RAM
				
					// Save previous largest coarse bucket size
					uint32_t previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
					
					// Go through all groups of coarse buckets
					for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
					
						// Get the largest coarse bucket size from the group's buckets
						largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
					}
					
					// Get the current largest coarse bucket size from all the coarse buckets
					currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
				
				// Otherwise check if using more RAM for CPU trimming
				#elif CPU_TRIMMING_USE_MORE_RAM
				
					// Go through all groups of coarse buckets
					uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR))) largestCoarseBucketSizes;
					for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
					
						// Get the largest coarse bucket size from the group's buckets
						largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
					}
					
					// Get the current largest coarse bucket size from all the coarse buckets
					uint32_t currentLargestCoarseBucketSize = min(__builtin_reduce_max(largestCoarseBucketSizes), CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS);
				#endif
				
				// Go through all columns of this thread's coarse buckets
				__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
				for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
				
					// Set fine bucket's number of edges to zero
					__builtin_memset_inline(numberOfEdgesPerFineBucket.get(), 0, CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t));
					
					// Go through all coarse buckets in the column's row
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Check if using max RAM for CPU trimming
						#if CPU_TRIMMING_USE_MAX_RAM
						
							// Set next edge index to the beginning of the coarse bucket
							nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
							
						// Otherwise check if using more RAM for CPU trimming
						#elif CPU_TRIMMING_USE_MORE_RAM
						
							// Set next edge index to the beginning of the coarse bucket
							nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
							
						// Otherwise
						#else
						
							// Set next edge index to the beginning of the coarse bucket
							nextEdgeIndex[j] = cpuTrimmingThreadsCoarseBucketsOne[i][j];
						#endif
						
						// Go through all edges in the coarse bucket
						for(uint32_t k = 0, l = cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i][j]; k < l; ++k) [[likely]] {
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Get edge's nodes
								const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
								
							// Otherwise
							#else
							
								// Get edge's nodes
								const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[i][j][k];
							#endif
							
							// Get fine bucket's index from the edge's node in the second partition
							const int fineBucketIndex = (nodes >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & CPU_FINE_BUCKET_INDEX_MASK;
							
							// Check if CPU trimming bounds checking is set to avoid conditional statements
							#if CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS
							
								// Put edge's nodes in the fine bucket at the index
								fineBuckets[fineBucketIndex][numberOfEdgesPerFineBucket[fineBucketIndex]] = nodes;
								
								// Increment the fine bucket's number of edges if it isn't full
								numberOfEdgesPerFineBucket[fineBucketIndex] += numberOfEdgesPerFineBucket[fineBucketIndex] < CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET - 1;
								
							// Otherwise
							#else
							
								// Check if fine bucket at the index isn't full
								if(numberOfEdgesPerFineBucket[fineBucketIndex] < CPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) [[likely]] {
								
									// Put edge's nodes in the fine bucket at the index
									fineBuckets[fineBucketIndex][numberOfEdgesPerFineBucket[fineBucketIndex]++] = nodes;
								}
								
								// Otherwise
								else [[unlikely]] {
								
									// Display message
									cpuTrimmingThreadsLock.lock();
									cout << "Lost edge at second partition fine bucket sort" << endl;
									cpuTrimmingThreadsLock.unlock();
								}
							#endif
						}
					}
					
					// Go through all fine buckets
					#if CPU_TRIMMING_USE_MORE_RAM
						uint32_t numberOfEdges[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION] = {};
					#endif
					for(int j = 0; j < CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Clear bitmap
						fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, 0);
						
						// Go through all edges in the fine bucket
						const uint32_t &l = numberOfEdgesPerFineBucket[j];
						for(uint32_t k = 0; k < l; ++k) [[likely]] {
						
							// Enable edge's node in the second partition in the bitmap
							bitmap[fineBuckets[j][k] & CPU_TRIMMING_BITMAP_ITEM_MASK] = 1;
						}
						
						// Go through all edges in the fine bucket
						for(uint32_t k = 0; k < l; ++k) [[likely]] {
						
							// Get edge's nodes
							const uint64_t &nodes = fineBuckets[j][k];
							
							// Get coarse bucket's index from the edge's node in the first partition
							const int coarseBucketIndex = nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE + CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING);
							
							// Check if using more RAM for CPU trimming
							#if CPU_TRIMMING_USE_MORE_RAM
							
								// Check if CPU trimming bounds checking is set to avoid conditional statements
								#if CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS
								
									// Put edge's nodes in the coarse bucket at the index
									nextEdgeIndex[coarseBucketIndex][numberOfEdges[coarseBucketIndex]] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
									
									// Increment the number of edges in the coarse bucket if the edge's node in the second partition has a pair in the bitmap and the coarse bucket isn't full
									numberOfEdges[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK] & (numberOfEdges[coarseBucketIndex] < currentLargestCoarseBucketSize - 1);
									
								// Otherwise
								#else
								
									// Check if coarse bucket at the index isn't full
									if(numberOfEdges[coarseBucketIndex] < currentLargestCoarseBucketSize) [[likely]] {
									
										// Put edge's nodes in the coarse bucket at the index
										nextEdgeIndex[coarseBucketIndex][numberOfEdges[coarseBucketIndex]] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
										
										// Increment the number of edges in the coarse bucket if the edge's node in the second partition has a pair in the bitmap
										numberOfEdges[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK];
									}
									
									// Otherwise
									else [[unlikely]] {
									
										// Display message
										cpuTrimmingThreadsLock.lock();
										cout << "Lost edge at first partition coarse bucket sort" << endl;
										cpuTrimmingThreadsLock.unlock();
									}
								#endif
								
							// Otherwise
							#else
							
								// Put edge's nodes in the coarse bucket at the index
								*nextEdgeIndex[coarseBucketIndex] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
								
								// Increment the coarse bucket's next edge index if the edge's node in the second partition has a pair in the bitmap
								nextEdgeIndex[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK];
							#endif
						}
					}
					
					// Go through all coarse buckets in the column's row
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Check if using max RAM for CPU trimming
						#if CPU_TRIMMING_USE_MAX_RAM
						
							// Set coarse bucket's number of edges based on its next edge index
							cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
							
						// Otherwise check if using more RAM for CPU trimming
						#elif CPU_TRIMMING_USE_MORE_RAM
						
							// Set coarse bucket's number of edges based on its next edge index
							cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[j][i] = numberOfEdges[j];
							
						// Otherwise
						#else
						
							// Set coarse bucket's number of edges based on its next edge index
							cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[j][i] = nextEdgeIndex[j] - cpuTrimmingThreadsCoarseBucketsOne[i][j];
						#endif
					}
				}
				
				// Synchronize CPU trimming threads
				cpuTrimmingThreadsBarrier.arrive_and_wait();
				
				// Check if displaying tuning times
				#if DISPLAY_TUNING_TIMES
				
					// Get end time
					endTime = chrono::steady_clock::now();
					
					// Check if this is the first CPU trimming thread
					if(!cpuTrimmingThreadIndex) [[unlikely]] {
					
						// Go through all groups of coarse buckets
						uint32_t currentRemainingNumberOfEdges = 0;
						for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
						
							// Add the number of edges in the group's buckets to the current remaining number of edges
							currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						}
						
						// Display message
						cpuTrimmingThreadsLock.lock();
						cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + 2) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
						cpuTrimmingThreadsLock.unlock();
					}
				#endif
				
				// Go through all remaining CPU trimming rounds before compressing
				for(int trimmingRound = 2 / 2; trimmingRound < CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING / 2; ++trimmingRound) [[likely]] {
				
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get start time
						startTime = chrono::steady_clock::now();
					#endif
					
					// Check if using max RAM for CPU trimming
					#if CPU_TRIMMING_USE_MAX_RAM 
					
						// Save previous largest coarse bucket size
						previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
						
					// Otherwise check if using more RAM for CPU trimming
					#elif CPU_TRIMMING_USE_MORE_RAM
					
						// Save previous largest coarse bucket size
						uint32_t previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
					#endif
					
					// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
					#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
						
						// Go through all groups of coarse buckets
						for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						}
						
						// Get the current largest coarse bucket size from all the coarse buckets
						currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
					#endif
					
					// Go through all rows of this thread's coarse buckets
					__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
					for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
					
						// Set fine bucket's number of edges to zero
						__builtin_memset_inline(numberOfEdgesPerFineBucket.get(), 0, CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t));
						
						// Go through all coarse buckets in the row's column
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = cpuTrimmingThreadsCoarseBucketsOne[j][i];
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint32_t k = 0, l = cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i][j]; k < l; ++k) [[likely]] {
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									
								// Otherwise
								#else
								
									// Get edge's nodes
									const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[j][i][k];
								#endif
								
								// Get fine bucket's index from the edge's node in the first partition
								const int fineBucketIndex = (nodes >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & CPU_FINE_BUCKET_INDEX_MASK;
								
								// Put edge's nodes in the fine bucket at the index
								fineBuckets[fineBucketIndex][numberOfEdgesPerFineBucket[fineBucketIndex]++] = nodes;
							}
						}
						
						// Go through all fine buckets
						for(int j = 0; j < CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Clear bitmap
							fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, 0);
							
							// Go through all edges in the fine bucket
							const uint32_t &l = numberOfEdgesPerFineBucket[j];
							for(uint32_t k = 0; k < l; ++k) [[likely]] {
							
								// Enable edge's node in the first partition in the bitmap
								bitmap[fineBuckets[j][k] & CPU_TRIMMING_BITMAP_ITEM_MASK] = 1;
							}
							
							// Go through all edges in the fine bucket
							for(uint32_t k = 0; k < l; ++k) [[likely]] {
							
								// Get edge's nodes
								const uint64_t &nodes = fineBuckets[j][k];
								
								// Get coarse bucket's index from the edge's node in the second partition
								const int coarseBucketIndex = nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE + CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING);
								
								// Put edge's nodes in the coarse bucket at the index
								*nextEdgeIndex[coarseBucketIndex] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
								
								// Increment the coarse bucket's next edge index if the edge's node in the first partition has a pair in the bitmap
								nextEdgeIndex[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK];
							}
						}
						
						// Go through all coarse buckets in the row's column
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[j][i] = nextEdgeIndex[j] - cpuTrimmingThreadsCoarseBucketsOne[j][i];
							#endif
						}
					}
					
					// Synchronize CPU trimming threads
					cpuTrimmingThreadsBarrier.arrive_and_wait();
					
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get end time
						endTime = chrono::steady_clock::now();
						
						// Check if this is the first CPU trimming thread
						if(!cpuTrimmingThreadIndex) [[unlikely]] {
						
							// Go through all groups of coarse buckets
							uint32_t currentRemainingNumberOfEdges = 0;
							for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
							
								// Add the number of edges in the group's buckets to the current remaining number of edges
								currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
							}
							
							// Display message
							cpuTrimmingThreadsLock.lock();
							cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + trimmingRound * 2 + 1) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
							cpuTrimmingThreadsLock.unlock();
						}
						
						// Get start time
						startTime = chrono::steady_clock::now();
					#endif
					
					// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
					#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
					
						// Save previous largest coarse bucket size
						previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
						
						// Go through all groups of coarse buckets
						for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						}
						
						// Get the current largest coarse bucket size from all the coarse buckets
						currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
					#endif
					
					// Go through all columns of this thread's coarse buckets
					__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
					for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
					
						// Set fine bucket's number of edges to zero
						__builtin_memset_inline(numberOfEdgesPerFineBucket.get(), 0, CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t));
						
						// Go through all coarse buckets in the column's row
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise
							#else
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = cpuTrimmingThreadsCoarseBucketsOne[i][j];
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint32_t k = 0, l = cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i][j]; k < l; ++k) [[likely]] {
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									
								// Otherwise
								#else
								
									// Get edge's nodes
									const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[i][j][k];
								#endif
								
								// Get fine bucket's index from the edge's node in the second partition
								const int fineBucketIndex = (nodes >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & CPU_FINE_BUCKET_INDEX_MASK;
								
								// Put edge's nodes in the fine bucket at the index
								fineBuckets[fineBucketIndex][numberOfEdgesPerFineBucket[fineBucketIndex]++] = nodes;
							}
						}
						
						// Go through all fine buckets
						for(int j = 0; j < CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Clear bitmap
							fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, 0);
							
							// Go through all edges in the fine bucket
							const uint32_t &l = numberOfEdgesPerFineBucket[j];
							for(uint32_t k = 0; k < l; ++k) [[likely]] {
							
								// Enable edge's node in the second partition in the bitmap
								bitmap[fineBuckets[j][k] & CPU_TRIMMING_BITMAP_ITEM_MASK] = 1;
							}
							
							// Go through all edges in the fine bucket
							for(uint32_t k = 0; k < l; ++k) [[likely]] {
							
								// Get edge's nodes
								const uint64_t &nodes = fineBuckets[j][k];
								
								// Get coarse bucket's index from the edge's node in the first partition
								const int coarseBucketIndex = nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE + CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING);
								
								// Put edge's nodes in the coarse bucket at the index
								*nextEdgeIndex[coarseBucketIndex] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
								
								// Increment the coarse bucket's next edge index if the edge's node in the second partition has a pair in the bitmap
								nextEdgeIndex[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK];
							}
						}
						
						// Go through all coarse buckets in the column's row
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise
							#else
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[j][i] = nextEdgeIndex[j] - cpuTrimmingThreadsCoarseBucketsOne[i][j];
							#endif
						}
					}
					
					// Synchronize CPU trimming threads
					cpuTrimmingThreadsBarrier.arrive_and_wait();
					
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get end time
						endTime = chrono::steady_clock::now();
						
						// Check if this is the first CPU trimming thread
						if(!cpuTrimmingThreadIndex) [[unlikely]] {
						
							// Go through all groups of coarse buckets
							uint32_t currentRemainingNumberOfEdges = 0;
							for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
							
								// Add the number of edges in the group's buckets to the current remaining number of edges
								currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
							}
							
							// Display message
							cpuTrimmingThreadsLock.lock();
							cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + trimmingRound * 2 + 2) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
							cpuTrimmingThreadsLock.unlock();
						}
					#endif
				}
				
				// Check if CPU trimming rounds before compressing is odd
				#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
				
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get start time
						startTime = chrono::steady_clock::now();
					#endif
					
					// Check if using max RAM for CPU trimming
					#if CPU_TRIMMING_USE_MAX_RAM 
					
						// Save previous largest coarse bucket size
						previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
						
					// Otherwise check if using more RAM for CPU trimming
					#elif CPU_TRIMMING_USE_MORE_RAM
					
						// Save previous largest coarse bucket size
						uint32_t previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
					#endif
					
					// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
					#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
						
						// Go through all groups of coarse buckets
						for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						}
						
						// Get the current largest coarse bucket size from all the coarse buckets
						currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
					#endif
					
					// Go through all rows of this thread's coarse buckets
					__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
					for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
					
						// Set fine bucket's number of edges to zero
						__builtin_memset_inline(numberOfEdgesPerFineBucket.get(), 0, CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t));
						
						// Go through all coarse buckets in the row's column
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = cpuTrimmingThreadsCoarseBucketsOne[j][i];
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint32_t k = 0, l = cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i][j]; k < l; ++k) [[likely]] {
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									
								// Otherwise
								#else
								
									// Get edge's nodes
									const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[j][i][k];
								#endif
								
								// Get fine bucket's index from the edge's node in the first partition
								const int fineBucketIndex = (nodes >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & CPU_FINE_BUCKET_INDEX_MASK;
								
								// Put edge's nodes in the fine bucket at the index
								fineBuckets[fineBucketIndex][numberOfEdgesPerFineBucket[fineBucketIndex]++] = nodes;
							}
						}
						
						// Go through all fine buckets
						for(int j = 0; j < CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Clear bitmap
							fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, 0);
							
							// Go through all edges in the fine bucket
							const uint32_t &l = numberOfEdgesPerFineBucket[j];
							for(uint32_t k = 0; k < l; ++k) [[likely]] {
							
								// Enable edge's node in the first partition in the bitmap
								bitmap[fineBuckets[j][k] & CPU_TRIMMING_BITMAP_ITEM_MASK] = 1;
							}
							
							// Go through all edges in the fine bucket
							for(uint32_t k = 0; k < l; ++k) [[likely]] {
							
								// Get edge's nodes
								const uint64_t &nodes = fineBuckets[j][k];
								
								// Get coarse bucket's index from the edge's node in the second partition
								const int coarseBucketIndex = nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE + CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING);
								
								// Put edge's nodes in the coarse bucket at the index
								*nextEdgeIndex[coarseBucketIndex] = __builtin_rotateleft64(nodes, sizeof(uint32_t) * BITS_IN_A_BYTE);
								
								// Increment the coarse bucket's next edge index if the edge's node in the first partition has a pair in the bitmap
								nextEdgeIndex[coarseBucketIndex] += bitmap[(nodes ^ 1) & CPU_TRIMMING_BITMAP_ITEM_MASK];
							}
						}
						
						// Go through all coarse buckets in the row's column
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set coarse bucket's number of edges based on its next edge index
								cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[j][i] = nextEdgeIndex[j] - cpuTrimmingThreadsCoarseBucketsOne[j][i];
							#endif
						}
					}
					
					// Synchronize CPU trimming threads
					cpuTrimmingThreadsBarrier.arrive_and_wait();
					
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get end time
						endTime = chrono::steady_clock::now();
						
						// Check if this is the first CPU trimming thread
						if(!cpuTrimmingThreadIndex) [[unlikely]] {
						
							// Go through all groups of coarse buckets
							uint32_t currentRemainingNumberOfEdges = 0;
							for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
							
								// Add the number of edges in the group's buckets to the current remaining number of edges
								currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
							}
							
							// Display message
							cpuTrimmingThreadsLock.lock();
							cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
							cpuTrimmingThreadsLock.unlock();
						}
					#endif
				#endif
				
				// Check if displaying tuning times
				#if DISPLAY_TUNING_TIMES
				
					// Get start time
					startTime = chrono::steady_clock::now();
				#endif
				
				// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
				#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
				
					// Check if CPU trimming rounds before compressing is odd
					#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
					
						// Save previous largest coarse bucket size
						previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
						
					// Otherwise
					#else
					
						// Check if using max RAM for CPU trimming
						#if CPU_TRIMMING_USE_MAX_RAM 
						
							// Save previous largest coarse bucket size
							previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
							
						// Otherwise check if using more RAM for CPU trimming
						#elif CPU_TRIMMING_USE_MORE_RAM
						
							// Save previous largest coarse bucket size
							uint32_t previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
						#endif
					#endif
					
					// Go through all groups of coarse buckets
					for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
							
						// Otherwise
						#else
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint32_t __attribute__((vector_size(sizeof(uint32_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						#endif
					}
					
					// Get the current largest coarse bucket size from all the coarse buckets
					currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
				#endif
				
				// Go through all rows of this thread's coarse buckets
				__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
				for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
				
					// Set fine bucket's number of edges to zero
					__builtin_memset_inline(numberOfEdgesPerFineBucket.get(), 0, CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t));
					
					// Go through all coarse buckets in the row's column
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise
							#else
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = cpuTrimmingThreadsCoarseBucketsOne[i][j];
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint32_t k = 0, l = cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo[i][j]; k < l; ++k) [[likely]] {
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									
								// Otherwise
								#else
								
									// Get edge's nodes
									const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[i][j][k];
								#endif
								
						// Otherwise
						#else
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set next edge index to the beginning of the coarse bucket
								nextEdgeIndex[j] = cpuTrimmingThreadsCoarseBucketsOne[j][i];
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint32_t k = 0, l = cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne[i][j]; k < l; ++k) [[likely]] {
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									
								// Otherwise
								#else
								
									// Get edge's nodes
									const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[j][i][k];
								#endif
						#endif
							
							// Get fine bucket's index from the edge's node in the first partition
							const int fineBucketIndex = (nodes >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & CPU_FINE_BUCKET_INDEX_MASK;
							
							// Put edge's nodes in the fine bucket at the index
							fineBuckets[fineBucketIndex][numberOfEdgesPerFineBucket[fineBucketIndex]++] = nodes;
						}
					}
					
					// Go through all fine buckets
					int numberOfCompressedItems = 0;
					for(int j = 0; j < CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Clear bitmap
						fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, 0);
						
						// Go through all edges in the fine bucket
						const uint32_t &l = numberOfEdgesPerFineBucket[j];
						for(uint32_t k = 0; k < l; ++k) [[likely]] {
						
							// Enable edge's node in the first partition in the bitmap
							bitmap[fineBuckets[j][k] & CPU_TRIMMING_BITMAP_ITEM_MASK] = 0x01;
						}
						
						// Go through all edges in the fine bucket
						for(uint32_t k = 0; k < l; ++k) [[likely]] {
						
							// Get edge's nodes
							const uint64_t &nodes = fineBuckets[j][k];
							
							// Get location of the bitmap value for the edge's node in the first partition and its pair
							uint16_t *bitmapCurrentPair = &reinterpret_cast<uint16_t *>(bitmap.get())[(nodes & CPU_TRIMMING_BITMAP_ITEM_MASK) / sizeof(uint16_t)];
							
							// Check if CPU trimming bounds checking is set to avoid conditional statements
							#if CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS
							
								// Get bitmap value for the node pair if the lookup table isn't full
								const uint16_t bitmapCurrentPairValue = *bitmapCurrentPair * (numberOfCompressedItems < static_cast<int>(CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION) - 1);
								
							// Otherwise
							#else
							
								// Get bitmap value for the node pair
								const uint16_t bitmapCurrentPairValue = *bitmapCurrentPair;
								
								// Check if the node has a pair and they haven't been compressed and the lookup table is full
								if((bitmapCurrentPairValue == 0x0101) & (numberOfCompressedItems >= static_cast<int>(CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION))) [[unlikely]] {
								
									// Display message
									cpuTrimmingThreadsLock.lock();
									cout << "Lost edge at compressing first partition" << endl;
									cpuTrimmingThreadsLock.unlock();
									
									// Go to next edge
									continue;
								}
							#endif
							
							// Record the node pair's uncompressed value in the lookup table for the first partition
							cpuTrimmingThreadsCompressedLookupTableFirstPartition[i][numberOfCompressedItems] = nodes;
							
							// Increment number of compressed items if the node has a pair and they haven't been compressed
							numberOfCompressedItems += bitmapCurrentPairValue == 0x0101;
							
							// Set that the node pair has been compressed by putting its compressed value in the bitmap if the node has a pair and they haven't been compressed
							*bitmapCurrentPair += numberOfCompressedItems * (bitmapCurrentPairValue == 0x0101);
							
							// Get compressed value for the edge's node in the first partition
							const uint16_t compressedValue = ((*bitmapCurrentPair - (0x0101 + 1)) << 1) | (nodes & 1);
							
							// Get coarse bucket's index from the edge's node in the second partition
							const int coarseBucketIndex = nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE + CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING);
							
							// Put edge's nodes in the coarse bucket at the index
							*nextEdgeIndex[coarseBucketIndex] = (((nodes & (static_cast<uint32_t>(CPU_COARSE_BUCKET_INDEX_MASK) << CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING)) | compressedValue) << (sizeof(uint32_t) * BITS_IN_A_BYTE)) | (nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE));
							
							// Increment the coarse bucket's next edge index if the edge's node in the first partition has a pair in the bitmap
							nextEdgeIndex[coarseBucketIndex] += bitmapCurrentPairValue >= 0x0101;
						}
					}
					
					// Go through all coarse buckets in the row's column
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise
							#else
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = nextEdgeIndex[j] - cpuTrimmingThreadsCoarseBucketsOne[i][j];
							#endif
							
						// Otherwise
						#else
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = nextEdgeIndex[j] - reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = nextEdgeIndex[j] - cpuTrimmingThreadsCoarseBucketsOne[j][i];
							#endif
						#endif
					}
				}
				
				// Synchronize CPU trimming threads
				cpuTrimmingThreadsBarrier.arrive_and_wait();
				
				// Check if displaying tuning times
				#if DISPLAY_TUNING_TIMES
				
					// Get end time
					endTime = chrono::steady_clock::now();
					
					// Check if this is the first CPU trimming thread
					if(!cpuTrimmingThreadIndex) [[unlikely]] {
					
						// Go through all groups of coarse buckets
						uint32_t currentRemainingNumberOfEdges = 0;
						for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++i) [[likely]] {
						
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Add the number of edges in the group's buckets to the current remaining number of edges
								currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
								
							// Otherwise
							#else
							
								// Add the number of edges in the group's buckets to the current remaining number of edges
								currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
							#endif
						}
						
						// Display message
						cpuTrimmingThreadsLock.lock();
						cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING + 1) << " and compressing first partition edges finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
						cpuTrimmingThreadsLock.unlock();
					}
					
					// Get start time
					startTime = chrono::steady_clock::now();
				#endif
				
				// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
				#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
				
					// Save previous largest coarse bucket size
					previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
					
				// Otherwise
				#else
				
					// Create largest coarse bucket sizes 
					uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR))) largestCoarseBucketSizes;
				#endif
				
				// Go through all groups of coarse buckets
				for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
				
					// Check if CPU trimming rounds before compressing is odd
					#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
					
						// Get the largest coarse bucket size from the group's buckets
						largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						
					// Otherwise
					#else
					
						// Get the largest coarse bucket size from the group's buckets
						largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
					#endif
				}
				
				// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
				#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
				
					// Get the current largest coarse bucket size from all the coarse buckets
					currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
					
				// Otherwise
				#else
				
					// Get the current largest coarse bucket size from all the coarse buckets
					uint16_t currentLargestCoarseBucketSize = min(__builtin_reduce_max(largestCoarseBucketSizes), static_cast<uint16_t>(CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS));
				#endif
				
				// Go through all columns of this thread's coarse buckets
				__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
				for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
				
					// Set fine bucket's number of edges to zero
					__builtin_memset_inline(reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()), 0, CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint16_t));
					
					// Go through all coarse buckets in the column's row
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set next edge index to the beginning of the coarse bucket
								reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint16_t k = 0, l = reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i][j]; k < l; ++k) [[likely]] {
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									
								// Otherwise
								#else
								
									// Get edge's nodes
									const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[j][i][k];
								#endif
								
						// Otherwise
						#else
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set next edge index to the beginning of the coarse bucket
								reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set next edge index to the beginning of the coarse bucket
								reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint16_t k = 0, l = reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i][j]; k < l; ++k) [[likely]] {
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Get edge's nodes
									const uint64_t &nodes = reinterpret_cast<const uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									
								// Otherwise
								#else
								
									// Get edge's nodes
									const uint64_t &nodes = cpuTrimmingThreadsCoarseBucketsOne[i][j][k];
								#endif
						#endif
						
							// Get fine bucket's index from the edge's node in the second partition
							const int fineBucketIndex = (nodes >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & CPU_FINE_BUCKET_INDEX_MASK;
							
							// Put edge's nodes in the fine bucket at the index
							fineBuckets[fineBucketIndex][reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get())[fineBucketIndex]++] = nodes;
						}
					}
					
					// Go through all fine buckets
					int numberOfCompressedItems = 0;
					#if !CPU_TRIMMING_USE_MAX_RAM && !CPU_TRIMMING_USE_MORE_RAM
						uint16_t numberOfEdges[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION] = {};
					#endif
					for(int j = 0; j < CPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Clear bitmap
						fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_TRIMMING_NUMBER_OF_ITEMS_PER_BITMAP, 0);
						
						// Go through all edges in the fine bucket
						const uint16_t &l = reinterpret_cast<const uint16_t *>(numberOfEdgesPerFineBucket.get())[j];
						for(uint16_t k = 0; k < l; ++k) [[likely]] {
						
							// Enable edge's node in the second partition in the bitmap
							bitmap[fineBuckets[j][k] & CPU_TRIMMING_BITMAP_ITEM_MASK] = 0x01;
						}
						
						// Go through all edges in the fine bucket
						for(uint16_t k = 0; k < l; ++k) [[likely]] {
						
							// Get edge's nodes
							const uint64_t &nodes = fineBuckets[j][k];
							
							// Check if not using max RAM for CPU trimming and using more RAM for CPU trimming
							#if !CPU_TRIMMING_USE_MAX_RAM && !CPU_TRIMMING_USE_MORE_RAM
							
								// Get coarse bucket's index from the edge's node in the first partition
								const int coarseBucketIndex = nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE + CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING);
							#endif
							
							// Get location of the bitmap value for the edge's node in the second partition and its pair
							uint16_t *bitmapCurrentPair = &reinterpret_cast<uint16_t *>(bitmap.get())[(nodes & CPU_TRIMMING_BITMAP_ITEM_MASK) / sizeof(uint16_t)];
							
							// Check if CPU trimming bounds checking is set to avoid conditional statements
							#if CPU_TRIMMING_BOUNDS_CHECKING_AVOIDS_CONDITIONAL_STATEMENTS
							
								// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
								
									// Get bitmap value for the node pair if the lookup table isn't full
									const uint16_t bitmapCurrentPairValue = *bitmapCurrentPair * (numberOfCompressedItems < static_cast<int>(CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION) - 1);
									
								// Otherwise
								#else
								
									// Get bitmap value for the node pair if the lookup table isn't full and the coarse bucket at the index isn't full
									const uint16_t bitmapCurrentPairValue = *bitmapCurrentPair * ((numberOfCompressedItems < static_cast<int>(CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION) - 1) & (numberOfEdges[coarseBucketIndex] < currentLargestCoarseBucketSize - 1));
								#endif
								
							// Otherwise
							#else
							
								// Get bitmap value for the node pair
								const uint16_t bitmapCurrentPairValue = *bitmapCurrentPair;
								
								// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
								
									// Check if the node has a pair and they haven't been compressed and the lookup table is full
									if((bitmapCurrentPairValue == 0x0101) & (numberOfCompressedItems >= static_cast<int>(CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION))) [[unlikely]] {
									
								// Otherwise
								#else
								
									// Check if the node has a pair and they haven't been compressed and the lookup table is full or the coarse bucket at the index is full
									if((bitmapCurrentPairValue == 0x0101) & ((numberOfCompressedItems >= static_cast<int>(CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION)) | (numberOfEdges[coarseBucketIndex] >= currentLargestCoarseBucketSize))) [[unlikely]] {
								#endif
								
									// Display message
									cpuTrimmingThreadsLock.lock();
									cout << "Lost edge at compressing second partition" << endl;
									cpuTrimmingThreadsLock.unlock();
									
									// Go to next edge
									continue;
								}
							#endif
							
							// Record the node pair's uncompressed value in the lookup table for the second partition
							cpuTrimmingThreadsCompressedLookupTableSecondPartition[i][numberOfCompressedItems] = nodes;
							
							// Increment number of compressed items if the node has a pair and they haven't been compressed
							numberOfCompressedItems += bitmapCurrentPairValue == 0x0101;
							
							// Set that the node pair has been compressed by putting its compressed value in the bitmap if the node has a pair and they haven't been compressed
							*bitmapCurrentPair += numberOfCompressedItems * (bitmapCurrentPairValue == 0x0101);
							
							// Get compressed value for the edge's node in the second partition
							const uint16_t compressedValue = ((*bitmapCurrentPair - (0x0101 + 1)) << 1) | (nodes & 1);
							
							// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
							
								// Get coarse bucket's index from the edge's node in the first partition
								const int coarseBucketIndex = nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE + CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING);
								
								// Put edge's nodes in the coarse bucket at the index
								*reinterpret_cast<uint32_t **>(nextEdgeIndex)[coarseBucketIndex] = (static_cast<uint32_t>(compressedValue) << (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) | ((nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE)) & CPU_COMPRESSED_ITEM_MASK);
								
								// Increment the coarse bucket's next edge index if the edge's node in the second partition has a pair in the bitmap
								reinterpret_cast<uint32_t **>(nextEdgeIndex)[coarseBucketIndex] += bitmapCurrentPairValue >= 0x0101;
								
							// Otherwise
							#else
							
								// Put edge's nodes in the coarse bucket at the index
								reinterpret_cast<uint32_t **>(nextEdgeIndex)[coarseBucketIndex][numberOfEdges[coarseBucketIndex]] = (static_cast<uint32_t>(compressedValue) << (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) | ((nodes >> (sizeof(uint32_t) * BITS_IN_A_BYTE)) & CPU_COMPRESSED_ITEM_MASK);
								
								// Increment the number of edges in the coarse bucket if the edge's node in the second partition has a pair in the bitmap
								numberOfEdges[coarseBucketIndex] += bitmapCurrentPairValue >= 0x0101;
							#endif
						}
					}
					
					// Go through all coarse buckets in the column's row
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise
							#else
							
								// Set coarse bucket's number of edges based on its number of edges
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = numberOfEdges[j];
							#endif
							
						// Otherwise
						#else
						
							// Check if using max RAM for CPU trimming
							#if CPU_TRIMMING_USE_MAX_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								
							// Otherwise check if using more RAM for CPU trimming
							#elif CPU_TRIMMING_USE_MORE_RAM
							
								// Set coarse bucket's number of edges based on its next edge index
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = reinterpret_cast<uint32_t **>(nextEdgeIndex)[j] - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								
							// Otherwise
							#else
							
								// Set coarse bucket's number of edges based on its number of edges
								reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = numberOfEdges[j];
							#endif
						#endif
					}
				}
				
				// Synchronize CPU trimming threads
				cpuTrimmingThreadsBarrier.arrive_and_wait();
				
				// Check if displaying tuning times
				#if DISPLAY_TUNING_TIMES
				
					// Get end time
					endTime = chrono::steady_clock::now();
					
					// Check if this is the first CPU trimming thread
					if(!cpuTrimmingThreadIndex) [[unlikely]] {
					
						// Go through all groups of coarse buckets
						uint32_t currentRemainingNumberOfEdges = 0;
						for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++i) [[likely]] {
						
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Add the number of edges in the group's buckets to the current remaining number of edges
								currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
								
							// Otherwise
							#else
							
								// Add the number of edges in the group's buckets to the current remaining number of edges
								currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
							#endif
						}
						
						// Display message
						cpuTrimmingThreadsLock.lock();
						cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING + 2) << " and compressing second partition edges finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
						cpuTrimmingThreadsLock.unlock();
					}
				#endif
				
				// Check if CPU trimming rounds before compressing is odd
				#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
				
					// Go through all remaining CPU trimming rounds
					for(int trimmingRound = (CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING + 2) / 2; trimmingRound < (CPU_TRIMMING_ROUNDS - 1) / 2; ++trimmingRound) [[likely]] {
					
				// Otherwise
				#else
				
					// Go through all remaining CPU trimming rounds
					for(int trimmingRound = (CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING + 2) / 2; trimmingRound < CPU_TRIMMING_ROUNDS / 2; ++trimmingRound) [[likely]] {
				#endif
				
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get start time
						startTime = chrono::steady_clock::now();
					#endif
					
					// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
					#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
					
						// Save previous largest coarse bucket size
						previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
						
					// Otherwise
					#else
					
						// Save previous largest coarse bucket size
						uint16_t previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
					#endif
					
					// Go through all groups of coarse buckets
					for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
							
						// Otherwise
						#else
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						#endif
					}
					
					// Get the current largest coarse bucket size from all the coarse buckets
					currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
					
					// Go through all rows of this thread's coarse buckets
					__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
					for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the number of edges for all buckets in the row's column
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
							
						// Otherwise
						#else
						
							// Get the number of edges for all buckets in the row's column
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
						#endif
						
						// Clear bitmap
						fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_BITMAP, 0);
						
						// Go through all coarse buckets in the row's column
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Go through all edges in the coarse bucket
							for(uint16_t k = 0, l = reinterpret_cast<const uint16_t *>(numberOfEdgesPerFineBucket.get())[j]; k < l; ++k) [[likely]] {
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise check if using more RAM for CPU trimming
									#elif CPU_TRIMMING_USE_MORE_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise
									#else
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
									#endif
									
								// Otherwise
								#else
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise
									#else
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
									#endif
								#endif
							}
						}
						
						// Go through all coarse buckets in the row's column
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise
								#else
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								#endif
								
							// Otherwise
							#else
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise
								#else
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								#endif
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint16_t k = 0, l = reinterpret_cast<const uint16_t *>(numberOfEdgesPerFineBucket.get())[j]; k < l; ++k) [[likely]] {
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
										
									// Otherwise check if using more RAM for CPU trimming
									#elif CPU_TRIMMING_USE_MORE_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									#endif
									
								// Otherwise
								#else
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									#endif
								#endif
								
								// Put edge's nodes in the coarse bucket
								*currentNextEdgeIndex = __builtin_rotateleft32(nodes, CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE);
								
								// Increment the current next edge index if the edge's node in the first partition has a pair in the bitmap
								currentNextEdgeIndex += bitmap[(nodes ^ 1) & CPU_COMPRESSED_ITEM_MASK];
							}
							
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Check if using more RAM for CPU trimming
								#if CPU_TRIMMING_USE_MORE_RAM
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise
								#else
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								#endif
								
							// Otherwise
							#else
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise
								#else
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								#endif
							#endif
						}
					}
					
					// Synchronize CPU trimming threads
					cpuTrimmingThreadsBarrier.arrive_and_wait();
					
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get end time
						endTime = chrono::steady_clock::now();
						
						// Check if this is the first CPU trimming thread
						if(!cpuTrimmingThreadIndex) [[unlikely]] {
						
							// Go through all groups of coarse buckets
							uint32_t currentRemainingNumberOfEdges = 0;
							for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++i) [[likely]] {
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Add the number of edges in the group's buckets to the current remaining number of edges
									currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
									
								// Otherwise
								#else
								
									// Add the number of edges in the group's buckets to the current remaining number of edges
									currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
								#endif
							}
							
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Display message
								cpuTrimmingThreadsLock.lock();
								cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + trimmingRound * 2 + 2) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
								cpuTrimmingThreadsLock.unlock();
								
							// Otherwise
							#else
							
								// Display message
								cpuTrimmingThreadsLock.lock();
								cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + trimmingRound * 2 + 1) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
								cpuTrimmingThreadsLock.unlock();
							#endif
						}
						
						// Get start time
						startTime = chrono::steady_clock::now();
					#endif
					
					// Save previous largest coarse bucket size
					previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
					
					// Go through all groups of coarse buckets
					for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
							
						// Otherwise
						#else
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						#endif
					}
					
					// Get the current largest coarse bucket size from all the coarse buckets
					currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
					
					// Go through all columns of this thread's coarse buckets
					__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
					for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the number of edges for all buckets in the column's row
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
							
						// Otherwise
						#else
						
							// Get the number of edges for all buckets in the column's row
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
						#endif
						
						// Clear bitmap
						fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_BITMAP, 0);
						
						// Go through all coarse buckets in the column's row
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Go through all edges in the coarse bucket
							for(uint16_t k = 0, l = reinterpret_cast<const uint16_t *>(numberOfEdgesPerFineBucket.get())[j]; k < l; ++k) [[likely]] {
								
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise check if using more RAM for CPU trimming
									#elif CPU_TRIMMING_USE_MORE_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise
									#else
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
									#endif
									
								// Otherwise
								#else
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise
									#else
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
									#endif
								#endif
							}
						}
						
						// Go through all coarse buckets in the column's row
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
									
								// Otherwise
								#else
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								#endif
								
							// Otherwise
							#else
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
									
								// Otherwise
								#else
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								#endif
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint16_t k = 0, l = reinterpret_cast<const uint16_t *>(numberOfEdgesPerFineBucket.get())[j]; k < l; ++k) [[likely]] {
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
										
									// Otherwise check if using more RAM for CPU trimming
									#elif CPU_TRIMMING_USE_MORE_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									#endif
									
								// Otherwise
								#else
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									#endif
								#endif
								
								// Put edge's nodes in the coarse bucket
								*currentNextEdgeIndex = __builtin_rotateleft32(nodes, CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE);
								
								// Increment the current next edge index if the edge's node in the second partition has a pair in the bitmap
								currentNextEdgeIndex += bitmap[(nodes ^ 1) & CPU_COMPRESSED_ITEM_MASK];
							}
							
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Check if using more RAM for CPU trimming
								#if CPU_TRIMMING_USE_MORE_RAM
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
									
								// Otherwise
								#else
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								#endif
								
							// Otherwise
							#else
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
									
								// Otherwise
								#else
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
								#endif
							#endif
						}
					}
					
					// Synchronize CPU trimming threads
					cpuTrimmingThreadsBarrier.arrive_and_wait();
					
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get end time
						endTime = chrono::steady_clock::now();
						
						// Check if this is the first CPU trimming thread
						if(!cpuTrimmingThreadIndex) [[unlikely]] {
						
							// Go through all groups of coarse buckets
							uint32_t currentRemainingNumberOfEdges = 0;
							for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++i) [[likely]] {
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Add the number of edges in the group's buckets to the current remaining number of edges
									currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
									
								// Otherwise
								#else
								
									// Add the number of edges in the group's buckets to the current remaining number of edges
									currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
								#endif
							}
							
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Display message
								cpuTrimmingThreadsLock.lock();
								cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + trimmingRound * 2 + 3) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
								cpuTrimmingThreadsLock.unlock();
								
							// Otherwise
							#else
							
								// Display message
								cpuTrimmingThreadsLock.lock();
								cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + trimmingRound * 2 + 2) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
								cpuTrimmingThreadsLock.unlock();
							#endif
						}
					#endif
				}
				
				// Check if CPU trimming rounds is exclusively odd or CPU trimming rounds before compressing is exclusively odd
				#if ((CPU_TRIMMING_ROUNDS % 2) ^ (CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2)) == 1
				
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get start time
						startTime = chrono::steady_clock::now();
					#endif
					
					// Check if using max RAM for CPU trimming or using more RAM for CPU trimming
					#if CPU_TRIMMING_USE_MAX_RAM || CPU_TRIMMING_USE_MORE_RAM
					
						// Save previous largest coarse bucket size
						previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
						
					// Otherwise
					#else
					
						// Save previous largest coarse bucket size
						const uint16_t previousLargestCoarseBucketSize = currentLargestCoarseBucketSize;
					#endif
					
					// Go through all groups of coarse buckets
					for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION / CPU_TRIMMING_VECTOR_SCALE_FACTOR; ++i) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
							
						// Otherwise
						#else
						
							// Get the largest coarse bucket size from the group's buckets
							largestCoarseBucketSizes[i] = __builtin_reduce_max(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_TRIMMING_VECTOR_SCALE_FACTOR))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i * CPU_TRIMMING_VECTOR_SCALE_FACTOR]));
						#endif
					}
					
					// Get the current largest coarse bucket size from all the coarse buckets
					currentLargestCoarseBucketSize = __builtin_reduce_max(largestCoarseBucketSizes);
					
					// Go through all rows of this thread's coarse buckets
					__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
					for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the number of edges for all buckets in the row's column
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
							
						// Otherwise
						#else
						
							// Get the number of edges for all buckets in the row's column
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
						#endif
						
						// Clear bitmap
						fill(execution::unseq, bitmap.get(), bitmap.get() + CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_BITMAP, 0);
						
						// Go through all coarse buckets in the row's column
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Go through all edges in the coarse bucket
							for(uint16_t k = 0, l = reinterpret_cast<const uint16_t *>(numberOfEdgesPerFineBucket.get())[j]; k < l; ++k) [[likely]] {
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise check if using more RAM for CPU trimming
									#elif CPU_TRIMMING_USE_MORE_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise
									#else
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
									#endif
									
								// Otherwise
								#else
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
										
									// Otherwise
									#else
									
										// Enable edge's node in the first partition in the bitmap
										bitmap[reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k] & CPU_COMPRESSED_ITEM_MASK] = 1;
									#endif
								#endif
							}
						}
						
						// Go through all coarse buckets in the row's column
						for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
						
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
									
								// Otherwise check if using more RAM for CPU trimming
								#elif CPU_TRIMMING_USE_MORE_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise
								#else
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								#endif
								
							// Otherwise
							#else
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise
								#else
								
									// Set current next edge index to the beginning of the coarse bucket
									uint32_t *currentNextEdgeIndex = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								#endif
							#endif
							
							// Go through all edges in the coarse bucket
							for(uint16_t k = 0, l = reinterpret_cast<const uint16_t *>(numberOfEdgesPerFineBucket.get())[j]; k < l; ++k) [[likely]] {
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
										
									// Otherwise check if using more RAM for CPU trimming
									#elif CPU_TRIMMING_USE_MORE_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									#endif
									
								// Otherwise
								#else
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][previousLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									#endif
								#endif
								
								// Put edge's nodes in the coarse bucket
								*currentNextEdgeIndex = __builtin_rotateleft32(nodes, CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE);
								
								// Increment the current next edge index if the edge's node in the first partition has a pair in the bitmap
								currentNextEdgeIndex += bitmap[(nodes ^ 1) & CPU_COMPRESSED_ITEM_MASK];
							}
							
							// Check if CPU trimming rounds before compressing is odd
							#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
							
								// Check if using more RAM for CPU trimming
								#if CPU_TRIMMING_USE_MORE_RAM
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise
								#else
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								#endif
								
							// Otherwise
							#else
							
								// Check if using max RAM for CPU trimming
								#if CPU_TRIMMING_USE_MAX_RAM
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[j][i];
									
								// Otherwise
								#else
								
									// Set coarse bucket's number of edges based on the current next edge index
									reinterpret_cast<uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[j][i] = currentNextEdgeIndex - reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[j][i];
								#endif
							#endif
						}
					}
					
					// Synchronize CPU trimming threads
					cpuTrimmingThreadsBarrier.arrive_and_wait();
					
					// Check if displaying tuning times
					#if DISPLAY_TUNING_TIMES
					
						// Get end time
						endTime = chrono::steady_clock::now();
						
						// Check if this is the first CPU trimming thread
						if(!cpuTrimmingThreadIndex) [[unlikely]] {
						
							// Go through all groups of coarse buckets
							uint32_t currentRemainingNumberOfEdges = 0;
							for(int i = 0; i < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++i) [[likely]] {
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Add the number of edges in the group's buckets to the current remaining number of edges
									currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
									
								// Otherwise
								#else
								
									// Add the number of edges in the group's buckets to the current remaining number of edges
									currentRemainingNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
								#endif
							}
							
							// Display message
							cpuTrimmingThreadsLock.lock();
							cout << "Trimming round " << (GPU_TRIMMING_ROUNDS + CPU_TRIMMING_ROUNDS) << " finished by the CPU in " << static_cast<chrono::duration<double, milli>>(endTime - startTime) << " with " << currentRemainingNumberOfEdges << " edges remaining" << endl;
							cpuTrimmingThreadsLock.unlock();
						}
					#endif
				#endif
				
				// Go through all rows of all previous thread's coarse buckets
				uint32_t previousNumberOfEdges = 0;
				__builtin_assume(bucketStart >= 0 && bucketStart < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
				for(int i = 0; i < bucketStart; ++i) [[likely]] {
				
					// Check if CPU trimming rounds is exclusively odd or CPU trimming rounds before compressing is exclusively odd
					#if ((CPU_TRIMMING_ROUNDS % 2) ^ (CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2)) == 1
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Add the number of edges in the row's column's buckets to the previous number of edges
							previousNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
							
						// Otherwise
						#else
						
							// Add the number of edges in the row's column's buckets to the previous number of edges
							previousNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
						#endif
						
					// Otherwise
					#else
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Add the number of edges in the row's column's buckets to the previous number of edges
							previousNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
							
						// Otherwise
						#else
						
							// Add the number of edges in the row's column's buckets to the previous number of edges
							previousNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
						#endif
					#endif
				}
				
				// Go through all rows of this thread's coarse buckets
				uint32_t currentNumberOfEdges = previousNumberOfEdges;
				__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
				for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
				
					// Check if CPU trimming rounds is exclusively odd or CPU trimming rounds before compressing is exclusively odd
					#if ((CPU_TRIMMING_ROUNDS % 2) ^ (CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2)) == 1
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Add the number of edges in the row's column's buckets to the current number of edges
							currentNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
							
						// Otherwise
						#else
						
							// Add the number of edges in the row's column's buckets to the current number of edges
							currentNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
						#endif
						
					// Otherwise
					#else
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Add the number of edges in the row's column's buckets to the current number of edges
							currentNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i]));
							
						// Otherwise
						#else
						
							// Add the number of edges in the row's column's buckets to the current number of edges
							currentNumberOfEdges += __builtin_reduce_add(*reinterpret_cast<const uint16_t __attribute__((vector_size(sizeof(uint16_t) * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION))) *>(reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i]));
						#endif
					#endif
				}
				
				// Get total number of edges
				const uint32_t totalNumberOfEdges = currentNumberOfEdges;
				
				// Prevent current number of edges from exceeding the max number of edges after trimming
				currentNumberOfEdges = min(currentNumberOfEdges, static_cast<uint32_t>(MAX_NUMBER_OF_EDGES_AFTER_TRIMMING)) - min(previousNumberOfEdges, static_cast<uint32_t>(MAX_NUMBER_OF_EDGES_AFTER_TRIMMING));
				
				// Go through all rows of this thread's coarse buckets
				__builtin_assume(bucketEnd > bucketStart && bucketEnd <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
				for(int i = bucketStart; i < bucketEnd; ++i) [[likely]] {
				
					// Check if CPU trimming rounds is exclusively odd or CPU trimming rounds before compressing is exclusively odd
					#if ((CPU_TRIMMING_ROUNDS % 2) ^ (CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2)) == 1
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the number of edges for all buckets in the row's column
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
							
						// Otherwise
						#else
						
							// Get the number of edges for all buckets in the row's column
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
						#endif
						
					// Otherwise
					#else
					
						// Check if CPU trimming rounds before compressing is odd
						#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
						
							// Get the number of edges for all buckets in the row's column
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
							
						// Otherwise
						#else
						
							// Get the number of edges for all buckets in the row's column
							copy(execution::unseq, reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i], reinterpret_cast<const uint16_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne)[i] + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, reinterpret_cast<uint16_t *>(numberOfEdgesPerFineBucket.get()));
						#endif
					#endif
					
					// Go through all coarse buckets in the row's column
					for(int j = 0; j < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; ++j) [[likely]] {
					
						// Get bucket's number of applicable edges
						const uint32_t numberOfApplicableEdges = min(static_cast<uint32_t>(reinterpret_cast<const uint16_t *>(numberOfEdgesPerFineBucket.get())[j]), currentNumberOfEdges);
						
						// Update current number of edges
						currentNumberOfEdges -= numberOfApplicableEdges;
						
						// Go through all applicable edges in the bucket
						for(uint32_t k = 0; k < numberOfApplicableEdges; ++k, ++previousNumberOfEdges) [[likely]] {
						
							// Check if CPU trimming rounds is exclusively odd or CPU trimming rounds before compressing is exclusively odd
							#if ((CPU_TRIMMING_ROUNDS % 2) ^ (CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2)) == 1
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Check if using more RAM for CPU trimming
									#if CPU_TRIMMING_USE_MORE_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									#endif
									
									// Check if GPU trimming rounds is even
									#if GPU_TRIMMING_ROUNDS % 2 == 0
									
										// Set next remaining edge to the edge's uncompressed nodes
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS] = (cpuTrimmingThreadsCompressedLookupTableSecondPartition[i][(nodes & CPU_COMPRESSED_ITEM_MASK) >> 1] & ~1) | (nodes & 1);
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS + 1] = (cpuTrimmingThreadsCompressedLookupTableFirstPartition[j][nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE + 1)] & ~1) | ((nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) & 1);
										
									// Otherwise
									#else
									
										// Set next remaining edge to the edge's uncompressed nodes
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS] = (cpuTrimmingThreadsCompressedLookupTableFirstPartition[j][nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE + 1)] & ~1) | ((nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) & 1);
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS + 1] = (cpuTrimmingThreadsCompressedLookupTableSecondPartition[i][(nodes & CPU_COMPRESSED_ITEM_MASK) >> 1] & ~1) | (nodes & 1);
									#endif
									
								// Otherwise
								#else
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
									#endif
									
									// Check if GPU trimming rounds is even
									#if GPU_TRIMMING_ROUNDS % 2 == 0
									
										// Set next remaining edge to the edge's uncompressed nodes
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS] = (cpuTrimmingThreadsCompressedLookupTableFirstPartition[j][nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE + 1)] & ~1) | ((nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) & 1);
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS + 1] = (cpuTrimmingThreadsCompressedLookupTableSecondPartition[i][(nodes & CPU_COMPRESSED_ITEM_MASK) >> 1] & ~1) | (nodes & 1);
										
									// Otherwise
									#else
									
										// Set next remaining edge to the edge's uncompressed nodes
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS] = (cpuTrimmingThreadsCompressedLookupTableSecondPartition[i][(nodes & CPU_COMPRESSED_ITEM_MASK) >> 1] & ~1) | (nodes & 1);
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS + 1] = (cpuTrimmingThreadsCompressedLookupTableFirstPartition[j][nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE + 1)] & ~1) | ((nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) & 1);
									#endif
								#endif
								
							// Otherwise
							#else
							
								// Check if CPU trimming rounds before compressing is odd
								#if CPU_TRIMMING_ROUNDS_BEFORE_COMPRESSING % 2 == 1
								
									// Check if using more RAM for CPU trimming
									#if CPU_TRIMMING_USE_MORE_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									#endif
									
									// Check if GPU trimming rounds is even
									#if GPU_TRIMMING_ROUNDS % 2 == 0
									
										// Set next remaining edge to the edge's uncompressed nodes
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS] = (cpuTrimmingThreadsCompressedLookupTableSecondPartition[j][nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE + 1)] & ~1) | ((nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) & 1);
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS + 1] = (cpuTrimmingThreadsCompressedLookupTableFirstPartition[i][(nodes & CPU_COMPRESSED_ITEM_MASK) >> 1] & ~1) | (nodes & 1);
										
									// Otherwise
									#else
									
										// Set next remaining edge to the edge's uncompressed nodes
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS] = (cpuTrimmingThreadsCompressedLookupTableFirstPartition[i][(nodes & CPU_COMPRESSED_ITEM_MASK) >> 1] & ~1) | (nodes & 1);
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS + 1] = (cpuTrimmingThreadsCompressedLookupTableSecondPartition[j][nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE + 1)] & ~1) | ((nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) & 1);
									#endif
									
								// Otherwise
								#else
								
									// Check if using max RAM for CPU trimming
									#if CPU_TRIMMING_USE_MAX_RAM
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsOne)[i][j][k];
										
									// Otherwise
									#else
									
										// Get edge's nodes
										const uint32_t &nodes = reinterpret_cast<const uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][currentLargestCoarseBucketSize]>(cpuTrimmingThreadsCoarseBucketsTwo)[i][j][k];
									#endif
									
									// Check if GPU trimming rounds is even
									#if GPU_TRIMMING_ROUNDS % 2 == 0
									
										// Set next remaining edge to the edge's uncompressed nodes
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS] = (cpuTrimmingThreadsCompressedLookupTableFirstPartition[i][(nodes & CPU_COMPRESSED_ITEM_MASK) >> 1] & ~1) | (nodes & 1);
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS + 1] = (cpuTrimmingThreadsCompressedLookupTableSecondPartition[j][nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE + 1)] & ~1) | ((nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) & 1);
										
									// Otherwise
									#else
									
										// Set next remaining edge to the edge's uncompressed nodes
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS] = (cpuTrimmingThreadsCompressedLookupTableSecondPartition[j][nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE + 1)] & ~1) | ((nodes >> (CPU_COMPRESSED_ITEM_SIZE * BITS_IN_A_BYTE)) & 1);
										remainingEdges[previousNumberOfEdges * NUMBER_OF_EDGE_COMPONENTS + 1] = (cpuTrimmingThreadsCompressedLookupTableFirstPartition[i][(nodes & CPU_COMPRESSED_ITEM_MASK) >> 1] & ~1) | (nodes & 1);
									#endif
								#endif
							#endif
						}
					}
				}
				
				// Lock CPU trimming threads lock
				cpuTrimmingThreadsLock.lock();
				
				// Check if this is the last CPU trimming thread
				if(cpuTrimmingThreadIndex == numberOfCpuTrimmingThreads - 1) [[unlikely]] {
				
					// Set the number of remaining edges to the total number of edges
					numberOfRemainingEdges = min(totalNumberOfEdges, static_cast<uint32_t>(MAX_NUMBER_OF_EDGES_AFTER_TRIMMING));
				}
				
				// Check if all CPU trimming threads have finished
				if(++numberOfCpuTrimmingThreadsFinished == numberOfCpuTrimmingThreads) [[unlikely]] {
				
					// Reset number of CPU trimming threads finished
					numberOfCpuTrimmingThreadsFinished = 0;
					
					// Notify that CPU trimming threads have finished
					cpuTrimmingThreadsFinished = true;
					cpuTrimmingThreadsLock.unlock();
					cpuTrimmingThreadsFinishedConditionalVariable.notify_one();
				}
				
				// Otherwise
				else [[likely]] {
				
					// Unlock CPU trimming threads lock
					cpuTrimmingThreadsLock.unlock();
				}
			}
		});
	}
	
	// Wait until CPU trimming threads have finished initializing
	cpuTrimmingThreadsFinishedConditionalVariable.wait(cpuTrimmingThreadsLock, [&cpuTrimmingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
	
		// Return if CPU trimming threads have finished initializing
		return cpuTrimmingThreadsFinished;
	});
	
	// Create recover edges parameters
	alignas(hardware_destructive_interference_size) struct {
	
		// Solution node pairs first partition
		uint32_t solutionNodePairsFirstPartition[SOLUTION_SIZE / 2];
		
		// Solution SipHash keys
		uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) solutionSipHashKeys;
		
		// Solution nodes
		alignas(uint64_t) uint32_t solutionNodes[SOLUTION_SIZE * NUMBER_OF_EDGE_COMPONENTS];
		
	} recoverEdgesParameters;
	
	// Get recover edges parameters used size
	const size_t recoverEdgesParametersUsedSize = offsetof(decltype(recoverEdgesParameters), solutionNodes) + sizeof(recoverEdgesParameters.solutionNodes);
	
	// Create CPU searching threads mutex
	alignas(hardware_destructive_interference_size) mutex cpuSearchingThreadsMutex;
	
	// Lock CPU searching threads lock
	unique_lock cpuSearchingThreadsLock(cpuSearchingThreadsMutex);
	
	// Set start CPU searching threads trigger toggle to false
	alignas(hardware_destructive_interference_size) bool startCpuSearchingThreadsTriggerToggle = false;
	
	// Create start CPU searching threads conditional variable
	alignas(hardware_destructive_interference_size) condition_variable startCpuSearchingThreadsConditionalVariable;
	
	// Set CPU searching threads finished to false
	alignas(hardware_destructive_interference_size) bool cpuSearchingThreadsFinished = false;
	
	// Create CPU searching threads finished conditional variable
	alignas(hardware_destructive_interference_size) condition_variable cpuSearchingThreadsFinishedConditionalVariable;
	
	// Go through all CPU searching threads
	alignas(hardware_destructive_interference_size) unsigned int numberOfCpuSearchingThreadsFinished = 0;
	thread cpuSearchingThreads[numberOfCpuSearchingThreads];
	
	__builtin_assume(numberOfCpuSearchingThreads >= 1 && numberOfCpuSearchingThreads <= MAX_NUMBER_OF_CPU_SEARCHING_THREADS);
	for(unsigned int i = 0; i < numberOfCpuSearchingThreads; ++i) [[likely]] {
	
		// Create CPU searching thread
		cpuSearchingThreads[i] = thread([numberOfCpuSearchingThreads, &cpuThreadsInitializedSuccessfully, &closeCpuThreads, remainingEdges = remainingEdges.get(), &numberOfRemainingEdges, &recoverEdgesParameters, &cpuSearchingThreadsMutex, &startCpuSearchingThreadsTriggerToggle, &startCpuSearchingThreadsConditionalVariable, &cpuSearchingThreadsFinished, &cpuSearchingThreadsFinishedConditionalVariable, &numberOfCpuSearchingThreadsFinished, cpuSearchingThreadIndex = i]() __attribute__((always_inline)) noexcept {
		
			// Lock CPU searching threads lock
			unique_lock cpuSearchingThreadsLock(cpuSearchingThreadsMutex);
			
			// Create node connections
			const unique_ptr<CuckatooNodeConnection[], void(*)(CuckatooNodeConnection [])> nodeConnections(new(static_cast<align_val_t>(hardware_destructive_interference_size), nothrow) CuckatooNodeConnection[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE], [](CuckatooNodeConnection nodeConnections[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE]) __attribute__((always_inline)) noexcept {
			
				// Free node connections
				operator delete[](nodeConnections, static_cast<align_val_t>(hardware_destructive_interference_size), nothrow);
			});
			
			// Create newest node connections
			HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> newestNodeConnectionsFirstPartition;
			HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> newestNodeConnectionsSecondPartition;
			
			// Create visited node pairs
			HashTable<SOLUTION_SIZE / 2, true> visitedNodePairsFirstPartition;
			HashTable<SOLUTION_SIZE / 2, true> visitedNodePairsSecondPartition;
			
			// Create block
			{
			
				// Set CPU searching thread initializing failed to if setting this CPU searching thread's priority and affinity failed or if allocating memory failed
				const bool cpuSearchingThreadInitializingFailed = !setThreadPriorityAndAffinity(cpuSearchingThreadIndex) || !remainingEdges || !nodeConnections || !newestNodeConnectionsFirstPartition || !newestNodeConnectionsSecondPartition || !visitedNodePairsFirstPartition || !visitedNodePairsSecondPartition;
				
				// Check if initializing this CPU searching thread failed
				if(cpuSearchingThreadInitializingFailed) [[unlikely]] {
				
					// Set CPU threads initialized successfully to false
					cpuThreadsInitializedSuccessfully = false;
				}
				
				// Check if all CPU searching threads have finished initializing
				if(++numberOfCpuSearchingThreadsFinished == numberOfCpuSearchingThreads) [[unlikely]] {
				
					// Reset number of CPU searching threads finished
					numberOfCpuSearchingThreadsFinished = 0;
					
					// Notify that CPU searching threads have finished initializing
					cpuSearchingThreadsFinished = true;
					cpuSearchingThreadsLock.unlock();
					cpuSearchingThreadsFinishedConditionalVariable.notify_one();
				}
				
				// Otherwise
				else [[likely]] {
				
					// Unlock CPU searching threads lock
					cpuSearchingThreadsLock.unlock();
				}
				
				// Check if initializing this CPU searching thread failed
				if(cpuSearchingThreadInitializingFailed) [[unlikely]] {
				
					// Return
					return;
				}
				
				// Ensure memory is fully allocated
				setBufferGuaranteed(nodeConnections.get(), 0, MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE * sizeof(CuckatooNodeConnection));
			}
			
			// Loop forever
			const double firstEdgePercent = CPU_SEARCHING_THREADS_FIRST_EDGE_PERCENT[numberOfCpuSearchingThreads - 1][cpuSearchingThreadIndex];
			const double nextFirstEdgePercent = __builtin_expect(cpuSearchingThreadIndex != numberOfCpuSearchingThreads - 1, true) ? CPU_SEARCHING_THREADS_FIRST_EDGE_PERCENT[numberOfCpuSearchingThreads - 1][cpuSearchingThreadIndex + 1] : 1;
			alignas(uint64_t) uint32_t localSolutionNodes[SOLUTION_SIZE * NUMBER_OF_EDGE_COMPONENTS];
			
			for(bool startCpuSearchingThreadTriggerTrue = true;; startCpuSearchingThreadTriggerTrue = !startCpuSearchingThreadTriggerTrue) [[likely]] {
			
				// Wait until starting CPU searching threads is triggered
				cpuSearchingThreadsLock.lock();
				startCpuSearchingThreadsConditionalVariable.wait(cpuSearchingThreadsLock, [&startCpuSearchingThreadsTriggerToggle, startCpuSearchingThreadTriggerTrue]() __attribute__((always_inline)) noexcept -> bool {
				
					// Return if starting CPU searching threads
					return startCpuSearchingThreadsTriggerToggle == startCpuSearchingThreadTriggerTrue;
				});
				
				// Get if closing CPU searching threads
				const bool closingCpuSearchingThreads = closeCpuThreads;
				cpuSearchingThreadsLock.unlock();
				
				// Check if closing CPU searching threads
				if(closingCpuSearchingThreads) [[unlikely]] {
				
					// Return
					return;
				}
				
				// Get this thread's first searching edge
				const uint32_t firstSearchingEdge = firstEdgePercent * numberOfRemainingEdges;
				
				// Go through all remaining edges before this thread's first searching edge
				for(uint32_t i = 0; i < firstSearchingEdge * CUCKATOO_NODE_CONNECTIONS_PER_EDGE; i += CUCKATOO_NODE_CONNECTIONS_PER_EDGE) [[likely]] {
				
					// Get edge's nodes
					const uint32_t &node = remainingEdges[i];
					const uint32_t &otherNode = remainingEdges[i + 1];
					
					// Replace newest node connection for the node in the first partition
					nodeConnections[i] = {newestNodeConnectionsFirstPartition.replace(node, i), node};
					
					// Replace newest node connection for the node in the second partition
					nodeConnections[i + 1] = {newestNodeConnectionsSecondPartition.replace(otherNode, i + 1), otherNode};
				}
				
				// Search for a solution in this thread's edges
				const bool solutionFound = cuckatooGetSolution(localSolutionNodes, &remainingEdges[firstSearchingEdge * NUMBER_OF_EDGE_COMPONENTS], static_cast<uint32_t>(nextFirstEdgePercent * numberOfRemainingEdges) - firstSearchingEdge, nodeConnections.get(), firstSearchingEdge * CUCKATOO_NODE_CONNECTIONS_PER_EDGE, newestNodeConnectionsFirstPartition, newestNodeConnectionsSecondPartition, visitedNodePairsFirstPartition, visitedNodePairsSecondPartition);
				
				// Reset newest node connections
				newestNodeConnectionsFirstPartition.clear();
				newestNodeConnectionsSecondPartition.clear();
				
				// Check if displaying tuning times
				#if DISPLAY_TUNING_TIMES
				
					// Get time
					const chrono::steady_clock::time_point time = chrono::steady_clock::now();
					
					// Lock CPU searching threads lock
					cpuSearchingThreadsLock.lock();
					
					// Display message
					cout << "CPU searching thread " << (cpuSearchingThreadIndex + 1) << " done at " << fixed << static_cast<chrono::duration<double, milli>>(time.time_since_epoch()) << defaultfloat << endl;
					
				// Otherwise
				#else
				
					// Lock CPU searching threads lock
					cpuSearchingThreadsLock.lock();
				#endif
				
				// Check if a solution was found
				if(solutionFound) [[unlikely]] {
				
					// Set solution nodes to local solution nodes
					__builtin_memcpy_inline(recoverEdgesParameters.solutionNodes, localSolutionNodes, sizeof(localSolutionNodes));
				}
				
				// Check if all CPU searching threads have finished
				if(++numberOfCpuSearchingThreadsFinished == numberOfCpuSearchingThreads) [[unlikely]] {
				
					// Reset number of CPU searching threads finished
					numberOfCpuSearchingThreadsFinished = 0;
					
					// Notify that CPU searching threads have finished
					cpuSearchingThreadsFinished = true;
					cpuSearchingThreadsLock.unlock();
					cpuSearchingThreadsFinishedConditionalVariable.notify_one();
				}
				
				// Otherwise
				else [[likely]] {
				
					// Unlock CPU searching threads lock
					cpuSearchingThreadsLock.unlock();
				}
			}
		});
	}
	
	// Wait until CPU searching threads have finished initializing
	cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
	
		// Return if CPU searching threads have finished initializing
		return cpuSearchingThreadsFinished;
	});
	
	// Create CPU recovering threads bitmap
	alignas(hardware_destructive_interference_size) Bitmap<CPU_NUMBER_OF_ITEMS_PER_RECOVERING_BITMAP> cpuRecoveringThreadsBitmap;
	
	// Create solution edges
	alignas(hardware_destructive_interference_size) uint32_t solutionEdges[SOLUTION_SIZE];
	
	// Create CPU recovering threads mutex
	alignas(hardware_destructive_interference_size) mutex cpuRecoveringThreadsMutex;
	
	// Lock CPU recovering threads lock
	unique_lock cpuRecoveringThreadsLock(cpuRecoveringThreadsMutex);
	
	// Set start CPU recovering threads trigger toggle to false
	alignas(hardware_destructive_interference_size) bool startCpuRecoveringThreadsTriggerToggle = false;
	
	// Create start CPU recovering threads conditional variable
	alignas(hardware_destructive_interference_size) condition_variable startCpuRecoveringThreadsConditionalVariable;
	
	// Set CPU recovering threads finished to false
	alignas(hardware_destructive_interference_size) bool cpuRecoveringThreadsFinished = false;
	
	// Create CPU recovering threads finished conditional variable
	alignas(hardware_destructive_interference_size) condition_variable cpuRecoveringThreadsFinishedConditionalVariable;
	
	// Go through all CPU recovering threads
	alignas(hardware_destructive_interference_size) unsigned int numberOfCpuRecoveringThreadsFinished = 0;
	thread cpuRecoveringThreads[numberOfCpuRecoveringThreads];
	
	__builtin_assume(numberOfCpuRecoveringThreads >= 1 && numberOfCpuRecoveringThreads <= CPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR);
	for(unsigned int i = 0; i < numberOfCpuRecoveringThreads; ++i) [[likely]] {
	
		// Create CPU recovering thread
		cpuRecoveringThreads[i] = thread([numberOfCpuRecoveringThreads, &cpuThreadsInitializedSuccessfully, &closeCpuThreads, &recoverEdgesParameters, &cpuRecoveringThreadsBitmap, &solutionEdges, &cpuRecoveringThreadsMutex, &startCpuRecoveringThreadsTriggerToggle, &startCpuRecoveringThreadsConditionalVariable, &cpuRecoveringThreadsFinished, &cpuRecoveringThreadsFinishedConditionalVariable, &numberOfCpuRecoveringThreadsFinished, cpuRecoveringThreadIndex = i]() __attribute__((always_inline)) noexcept {
		
			// Lock CPU recovering threads lock
			unique_lock cpuRecoveringThreadsLock(cpuRecoveringThreadsMutex);
			
			// Create block
			{
			
				// Set CPU recovering thread initializing failed to if setting this CPU recovering thread's priority and affinity failed or if allocating memory failed
				const bool cpuRecoveringThreadInitializingFailed = !setThreadPriorityAndAffinity(cpuRecoveringThreadIndex) || !cpuRecoveringThreadsBitmap;
				
				// Check if initializing this CPU recovering thread failed
				if(cpuRecoveringThreadInitializingFailed) [[unlikely]] {
				
					// Set CPU threads initialized successfully to false
					cpuThreadsInitializedSuccessfully = false;
				}
				
				// Check if all CPU recovering threads have finished initializing
				if(++numberOfCpuRecoveringThreadsFinished == numberOfCpuRecoveringThreads) [[unlikely]] {
				
					// Reset number of CPU recovering threads finished
					numberOfCpuRecoveringThreadsFinished = 0;
					
					// Notify that CPU recovering threads have finished initializing
					cpuRecoveringThreadsFinished = true;
					cpuRecoveringThreadsLock.unlock();
					cpuRecoveringThreadsFinishedConditionalVariable.notify_one();
				}
				
				// Otherwise
				else [[likely]] {
				
					// Unlock CPU recovering threads lock
					cpuRecoveringThreadsLock.unlock();
				}
				
				// Check if initializing this CPU recovering thread failed
				if(cpuRecoveringThreadInitializingFailed) [[unlikely]] {
				
					// Return
					return;
				}
			}
			
			// Loop forever
			const uint64_t edgeStart = GPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR + ((CPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR) / numberOfCpuRecoveringThreads + CPU_RECOVERING_VECTOR_SCALE_FACTOR - 1) / CPU_RECOVERING_VECTOR_SCALE_FACTOR * CPU_RECOVERING_VECTOR_SCALE_FACTOR * cpuRecoveringThreadIndex;
			const uint64_t edgeEnd = GPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR + (__builtin_expect(cpuRecoveringThreadIndex != numberOfCpuRecoveringThreads - 1, true) ? ((CPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR) / numberOfCpuRecoveringThreads + CPU_RECOVERING_VECTOR_SCALE_FACTOR - 1) / CPU_RECOVERING_VECTOR_SCALE_FACTOR * CPU_RECOVERING_VECTOR_SCALE_FACTOR * (cpuRecoveringThreadIndex + 1) : (CPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR));
			uint32_t nodes[CPU_RECOVERING_VECTOR_SCALE_FACTOR];
			uint32_t localSolutionEdges[SOLUTION_SIZE] = {};
			
			for(bool startCpuRecoveringThreadTriggerTrue = true;; startCpuRecoveringThreadTriggerTrue = !startCpuRecoveringThreadTriggerTrue) [[likely]] {
			
				// Wait until starting CPU recovering threads is triggered
				cpuRecoveringThreadsLock.lock();
				startCpuRecoveringThreadsConditionalVariable.wait(cpuRecoveringThreadsLock, [&startCpuRecoveringThreadsTriggerToggle, startCpuRecoveringThreadTriggerTrue]() __attribute__((always_inline)) noexcept -> bool {
				
					// Return if starting CPU recovering threads
					return startCpuRecoveringThreadsTriggerToggle == startCpuRecoveringThreadTriggerTrue;
				});
				
				// Get if closing CPU recovering threads
				const bool closingCpuRecoveringThreads = closeCpuThreads;
				cpuRecoveringThreadsLock.unlock();
				
				// Check if closing CPU recovering threads
				if(closingCpuRecoveringThreads) [[unlikely]] {
				
					// Return
					return;
				}
				
				// Go through all of this thread's edges in groups
				__builtin_assume(edgeEnd > edgeStart && edgeEnd <= NUMBER_OF_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR);
				for(uint64_t i = edgeStart; i < edgeEnd; ++i) [[likely]] {
				
					// Go through all edges in the group
					for(int j = 0; j < CPU_RECOVERING_VECTOR_SCALE_FACTOR; ++j) [[likely]] {
					
						// Get edge
						const uint32_t edge = i * CPU_RECOVERING_VECTOR_SCALE_FACTOR + j;
						
						// Get edge's node
						__builtin_assume(edge < NUMBER_OF_EDGES);
						nodes[j] = sipHash24(recoverEdgesParameters.solutionSipHashKeys, static_cast<uint64_t>(edge) * 2) & NODE_MASK;
					}
					
					// Go through all edges in the group
					for(int j = 0; j < CPU_RECOVERING_VECTOR_SCALE_FACTOR; ++j) [[likely]] {
					
						// Get if edge's node is in the bitmap
						const bool inBitmap = cpuRecoveringThreadsBitmap.isBitSet(nodes[j] >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_IN_RECOVERING_BITMAP);
						
						// Check if edge is a recovered edge candidate
						const uint32_t *solutionIndex = find(execution::unseq, recoverEdgesParameters.solutionNodePairsFirstPartition, recoverEdgesParameters.solutionNodePairsFirstPartition + SOLUTION_SIZE / 2 * inBitmap, nodes[j] >> 1);
						if(solutionIndex != recoverEdgesParameters.solutionNodePairsFirstPartition + SOLUTION_SIZE / 2 * inBitmap) [[unlikely]] {
						
							// Get edge
							const uint32_t edge = i * CPU_RECOVERING_VECTOR_SCALE_FACTOR + j;
							
							// Get edge's other node
							__builtin_assume(edge < NUMBER_OF_EDGES);
							const uint64_t bothNodes = (static_cast<uint64_t>(nodes[j]) << (sizeof(uint32_t) * BITS_IN_A_BYTE)) | (sipHash24(recoverEdgesParameters.solutionSipHashKeys, static_cast<uint64_t>(edge) * 2 + 1) & NODE_MASK);
							
							// Set local solution edge to the edge if the edge's nodes match a solution nodes
							const int index = (solutionIndex - recoverEdgesParameters.solutionNodePairsFirstPartition) * NUMBER_OF_EDGE_COMPONENTS;
							localSolutionEdges[index] |= edge * (reinterpret_cast<const uint64_t *>(recoverEdgesParameters.solutionNodes)[index] == bothNodes);
							localSolutionEdges[index + 1] |= edge * (reinterpret_cast<const uint64_t *>(recoverEdgesParameters.solutionNodes)[index + 1] == bothNodes);
						}
					}
				}
				
				// Go through all local solution edges
				for(int i = 0; i < SOLUTION_SIZE; ++i) [[likely]] {
				
					// Check if local solution edge exists
					if(localSolutionEdges[i]) [[unlikely]] {
					
						// Set solution edge to the local solution edge
						solutionEdges[i] = localSolutionEdges[i];
					}
				}
				
				// Reset local solution edges
				__builtin_memset_inline(localSolutionEdges, 0, sizeof(localSolutionEdges));
				
				// Check if all CPU recovering threads have finished
				cpuRecoveringThreadsLock.lock();
				if(++numberOfCpuRecoveringThreadsFinished == numberOfCpuRecoveringThreads) [[unlikely]] {
				
					// Reset number of CPU recovering threads finished
					numberOfCpuRecoveringThreadsFinished = 0;
					
					// Notify that CPU recovering threads have finished
					cpuRecoveringThreadsFinished = true;
					cpuRecoveringThreadsLock.unlock();
					cpuRecoveringThreadsFinishedConditionalVariable.notify_one();
				}
				
				// Otherwise
				else [[likely]] {
				
					// Unlock CPU recovering threads lock
					cpuRecoveringThreadsLock.unlock();
				}
			}
		});
	}
	
	// Wait until CPU recovering threads have finished initializing
	cpuRecoveringThreadsFinishedConditionalVariable.wait(cpuRecoveringThreadsLock, [&cpuRecoveringThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
	
		// Return if CPU recovering threads have finished initializing
		return cpuRecoveringThreadsFinished;
	});
	
	// Set closing to false
	static volatile sig_atomic_t closing = false;
	
	// Check if displaying power usage
	#if DISPLAY_POWER_USAGE
	
		// Create total power used
		alignas(hardware_destructive_interference_size) double totalPowerUsed = 0;
		
		// Create total power samples
		alignas(hardware_destructive_interference_size) int totalPowerSamples = 0;
		
		// Create CPU recovering threads mutex
		alignas(hardware_destructive_interference_size) mutex powerUsageThreadMutex;
		
		// Create power usage thread lock
		unique_lock powerUsageThreadLock(powerUsageThreadMutex, defer_lock);
		
		// Check if using an Apple device
		#ifdef __APPLE__
		
			// Create power usage thread
			thread powerUsageThread([&totalPowerUsed, &totalPowerSamples, &powerUsageThreadMutex]() __attribute__((always_inline)) noexcept {
			
		// Otherwise
		#else
		
			// Create power usage thread
			thread powerUsageThread([]() __attribute__((always_inline)) noexcept {
		#endif
		
			// Check if using an Apple device
			#ifdef __APPLE__
			
				// Check if setting thread's scheduling priority to low was successful
				if(!pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0)) [[likely]] {
				
					// Check if getting the matching dictionary for the AppleSMC service was successful
					const CFDictionaryRef serviceMatchingDictionary = IOServiceMatching("AppleSMC");
					if(serviceMatchingDictionary) [[likely]] {
					
						// Check if getting the AppleSMC service was successful
						const io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, serviceMatchingDictionary);
						if(service) [[likely]] {
						
							// Check if opening connection to the AppleSMC service was successful
							io_connect_t serviceConnection;
							if(IOServiceOpen(service, mach_task_self_, 0, &serviceConnection) == KERN_SUCCESS) [[likely]] {
							
								// Create input parameters to get the total power in key's info
								SmcParameters inputParameters = {
								
									// Key
									.key = __builtin_bswap32(*reinterpret_cast<const decltype(inputParameters.key) *>("PDTR")),
									
									// Data 8
									.data8 = kSMCGetKeyInfo,
								};
								
								// Check if getting the total power in key's info was successful and the total power in key's info is valid
								SmcParameters outputParameters;
								size_t outputParametersSize = sizeof(outputParameters);
								
								if(IOConnectCallStructMethod(serviceConnection, kSMCHandleYPCEvent, &inputParameters, sizeof(inputParameters), &outputParameters, &outputParametersSize) == KERN_SUCCESS && !outputParameters.result && outputParameters.keyInfo.dataSize == sizeof(float) && outputParameters.keyInfo.dataType == __builtin_bswap32(*reinterpret_cast<const decltype(inputParameters.key) *>("flt "))) [[likely]] {
								
									// Set input parameters to read the total power in key's value
									inputParameters.data8 = kSMCReadKey;
									inputParameters.keyInfo.dataSize = outputParameters.keyInfo.dataSize;
									
									// Create power usage thread lock
									unique_lock powerUsageThreadLock(powerUsageThreadMutex, defer_lock);
									
									// Loop while not closing
									while(!closing) [[likely]] {
									
										// Check if reading the total power in key's value was successful
										if(IOConnectCallStructMethod(serviceConnection, kSMCHandleYPCEvent, &inputParameters, sizeof(inputParameters), &outputParameters, &outputParametersSize) == KERN_SUCCESS && !outputParameters.result) [[likely]] {
										
											// Lock power usage thread lock
											powerUsageThreadLock.lock();
											
											// Update total power used to include the total power in key's value
											totalPowerUsed += *reinterpret_cast<const float *>(&outputParameters.bytes);
											
											// Increment total power samples
											++totalPowerSamples;
											
											// Unlock power usage thread lock
											powerUsageThreadLock.unlock();
										}
										
										// Wait before reading the total power in key's value again
										usleep(SMC_POLL_RATE_MICROSECONDS);
									}
								}
								
								// Close service connection
								IOServiceClose(serviceConnection);
							}
							
							// Free service
							IOObjectRelease(service);
						}
					}
				}
			#endif
		});
	#endif
	
	// Check if using signal handler and not using Windows
	#if USE_SIGNAL_HANDLER && !defined _WIN32
	
		// Initialize signal action to restart syscalls when interrupted
		const struct sigaction signalAction = {
		
			// Handler
			.sa_handler = [](const int signal) __attribute__((always_inline)) noexcept {
			
				// Check if interrupt or terminate signal occurred
				if(signal == SIGINT || signal == SIGTERM) [[likely]] {
				
					// Set closing to true
					closing = true;
				}
			},
			
			// Flags
			.sa_flags = SA_RESTART
		};
	#endif
	
	// Create break loop
	int returnStatus = EXIT_FAILURE;
	do [[unlikely]] {
	
		// Check if CPU threads didn't initialize successfully
		if(!cpuThreadsInitializedSuccessfully) [[unlikely]] {
		
			// Display message
			cout << "Allocating memory failed" << endl;
			
			// Break
			break;
		}
		
		// Ensure memory is fully allocated
		setBufferGuaranteed(remainingEdges.get(), 0, MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * NUMBER_OF_EDGE_COMPONENTS * sizeof(uint32_t));
		setBufferGuaranteed(cpuTrimmingThreadsCoarseBucketsTwo.get(), 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_SHRUNK_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_BEFORE_SHRINKING_COARSE_BUCKETS);
		setBufferGuaranteed(cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketTwo.get(), 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t));
		setBufferGuaranteed(cpuTrimmingThreadsCompressedLookupTableFirstPartition.get(), 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) * CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_FIRST_PARTITION);
		setBufferGuaranteed(cpuTrimmingThreadsCompressedLookupTableSecondPartition.get(), 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) * CPU_NUMBER_OF_ITEMS_PER_COMPRESSED_LOOKUP_TABLE_SECOND_PARTITION);
		
		// Check if using signal handler
		#if USE_SIGNAL_HANDLER
		
			// Check if using Windows
			#ifdef _WIN32
			
				// Check if setting interrupt signal handler failed
				if(signal(SIGINT, [](const int signal) __attribute__((always_inline)) noexcept {
				
					// Check if interrupt signal occurred
					if(signal == SIGINT) [[likely]] {
					
						// Set closing to true
						closing = true;
					}
					
				}) == SIG_ERR) [[unlikely]] {
				
					// Display message
					cout << "Setting interrupt signal handler failed" << endl;
					
					// Break
					break;
				}
				
			// Otherwise
			#else
			
				// Check if setting interrupt signal handler failed
				if(sigaction(SIGINT, &signalAction, nullptr) || sigaction(SIGTERM, &signalAction, nullptr)) [[unlikely]] {
				
					// Display message
					cout << "Setting interrupt signal handler failed" << endl;
					
					// Break
					break;
				}
			#endif
		#endif
		
		// Check if setting this CPU thread's priority and affinity failed
		if(!setThreadPriorityAndAffinity(numberOfHighPerformanceCpuCores - 1)) [[unlikely]] {
		
			// Display message
			cout << "Setting thread's priority and affinity failed" << endl;
			
			// Break
			break;
		}
		
		// Check if preventing sleep failed
		const PreventSleep preventSleep;
		if(!preventSleep) [[unlikely]] {
		
			// Display message
			cout << "Preventing sleep failed" << endl;
			
			// Break
			break;
		}
		
		// Check if mining to a stratum server
		#if MINE_TO_A_STRATUM_SERVER
		
			// Create random number generator
			mt19937_64 randomNumberGenerator((random_device())());
			
			// Check if using Windows
			#ifdef _WIN32
			
				// Check if creating Windows socket failed
				const WindowsSocket windowsSocket;
				if(!windowsSocket) [[unlikely]] {
				
					// Display message
					cout << "Initializing Windows socket failed" << endl;
					
					// Break
					break;
				}
			#endif
		#endif
		
		// Check if displaying power usage
		#if DISPLAY_POWER_USAGE
		
			// Check if creating energy consumption failed
			const EnergyConsumption energyConsumption;
			if(!energyConsumption) [[unlikely]] {
			
				// Display message
				cout << "Monitoring energy consumption failed" << endl;
				
				// Break
				break;
			}
		#endif
		
		// Display message
		cout << "Finished creating CPU threads and allocating CPU memory" << endl;
		
		// Display message
		cout << "Acquiring GPU" << endl;
		
		// Check if using an Apple device
		#ifdef __APPLE__
		
			// Check if creating autorelease pool failed
			unique_ptr<NS::AutoreleasePool, void(*)(NS::AutoreleasePool *)> autoreleasePool(NS::AutoreleasePool::alloc()->init(), [](NS::AutoreleasePool *autoreleasePool) __attribute__((always_inline)) noexcept {
			
				// Free autorelease pool
				__builtin_assume_dereferenceable(autoreleasePool, sizeof(*autoreleasePool));
				autoreleasePool->release();
			});
			if(!autoreleasePool) [[unlikely]] {
			
				// Display message
				cout << "Creating autorelease pool failed" << endl;
				
				// Break
				break;
			}
			
			// Create block
			unique_ptr<MTL::Device, void(*)(MTL::Device *)> gpu(nullptr, [](MTL::Device *gpu) __attribute__((always_inline)) noexcept {
			
				// Free GPU
				__builtin_assume_dereferenceable(gpu, sizeof(*gpu));
				gpu->release();
			});
			unsigned long long gpuMemorySize;
			unique_ptr<decltype(gpuMemorySize), void(*)(decltype(gpuMemorySize) *)> gpuMemorySizeUniquePointer(nullptr, [](decltype(gpuMemorySize) *gpuMemorySize) __attribute__((always_inline)) noexcept {
			
				// Reset the GPU's memory size
				sysctlbyname("iogpu.wired_limit_mb", nullptr, 0, gpuMemorySize, sizeof(*gpuMemorySize));
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> coarseBucketSortEdgesPipeline(nullptr, [](MTL::ComputePipelineState *coarseBucketSortEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free coarse bucket sort edges pipeline
				__builtin_assume_dereferenceable(coarseBucketSortEdgesPipeline, sizeof(*coarseBucketSortEdgesPipeline));
				coarseBucketSortEdgesPipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> updateLargestInitialCoarseBucketSizePipeline(nullptr, [](MTL::ComputePipelineState *updateLargestInitialCoarseBucketSizePipeline) __attribute__((always_inline)) noexcept {
			
				// Free update largest initial coarse bucket size pipeline
				__builtin_assume_dereferenceable(updateLargestInitialCoarseBucketSizePipeline, sizeof(*updateLargestInitialCoarseBucketSizePipeline));
				updateLargestInitialCoarseBucketSizePipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> fineBucketSortInitialEdgesPipeline(nullptr, [](MTL::ComputePipelineState *fineBucketSortInitialEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free fine bucket sort initial edges pipeline
				__builtin_assume_dereferenceable(fineBucketSortInitialEdgesPipeline, sizeof(*fineBucketSortInitialEdgesPipeline));
				fineBucketSortInitialEdgesPipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> trimInitialEdgesPipeline(nullptr, [](MTL::ComputePipelineState *trimInitialEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free trim initial edges pipeline
				__builtin_assume_dereferenceable(trimInitialEdgesPipeline, sizeof(*trimInitialEdgesPipeline));
				trimInitialEdgesPipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> updateLargestIntermediateCoarseBucketSizePipeline(nullptr, [](MTL::ComputePipelineState *updateLargestIntermediateCoarseBucketSizePipeline) __attribute__((always_inline)) noexcept {
			
				// Free update largest intermediate coarse bucket size pipeline
				__builtin_assume_dereferenceable(updateLargestIntermediateCoarseBucketSizePipeline, sizeof(*updateLargestIntermediateCoarseBucketSizePipeline));
				updateLargestIntermediateCoarseBucketSizePipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> fineBucketSortIntermediateEdgesPipeline(nullptr, [](MTL::ComputePipelineState *fineBucketSortIntermediateEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free fine bucket sort intermediate edges pipeline
				__builtin_assume_dereferenceable(fineBucketSortIntermediateEdgesPipeline, sizeof(*fineBucketSortIntermediateEdgesPipeline));
				fineBucketSortIntermediateEdgesPipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> trimIntermediateEdgesPipeline(nullptr, [](MTL::ComputePipelineState *trimIntermediateEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free trim intermediate edges pipeline
				__builtin_assume_dereferenceable(trimIntermediateEdgesPipeline, sizeof(*trimIntermediateEdgesPipeline));
				trimIntermediateEdgesPipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> updateLargestFinalCoarseBucketSizePipeline(nullptr, [](MTL::ComputePipelineState *updateLargestFinalCoarseBucketSizePipeline) __attribute__((always_inline)) noexcept {
			
				// Free update largest final coarse bucket size pipeline
				__builtin_assume_dereferenceable(updateLargestFinalCoarseBucketSizePipeline, sizeof(*updateLargestFinalCoarseBucketSizePipeline));
				updateLargestFinalCoarseBucketSizePipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> fineBucketSortFinalEdgesPipeline(nullptr, [](MTL::ComputePipelineState *fineBucketSortFinalEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free fine bucket sort final edges pipeline
				__builtin_assume_dereferenceable(fineBucketSortFinalEdgesPipeline, sizeof(*fineBucketSortFinalEdgesPipeline));
				fineBucketSortFinalEdgesPipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> trimFinalEdgesPipeline(nullptr, [](MTL::ComputePipelineState *trimFinalEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free trim final edges pipeline
				__builtin_assume_dereferenceable(trimFinalEdgesPipeline, sizeof(*trimFinalEdgesPipeline));
				trimFinalEdgesPipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> trimFinalEdgesAndTransferEdgesPipeline(nullptr, [](MTL::ComputePipelineState *trimFinalEdgesAndTransferEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free trim final edges and transfer edges pipeline
				__builtin_assume_dereferenceable(trimFinalEdgesAndTransferEdgesPipeline, sizeof(*trimFinalEdgesAndTransferEdgesPipeline));
				trimFinalEdgesAndTransferEdgesPipeline->release();
			});
			unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)> recoverEdgesPipeline(nullptr, [](MTL::ComputePipelineState *recoverEdgesPipeline) __attribute__((always_inline)) noexcept {
			
				// Free recover edges pipeline
				__builtin_assume_dereferenceable(recoverEdgesPipeline, sizeof(*recoverEdgesPipeline));
				recoverEdgesPipeline->release();
			});
			{
			
				// Set total GPU memory allocated
				const size_t totalGpuMemoryAllocated = GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_COARSE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET + GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) + sizeof(TrimEdgesParameters) + sizeof(MTL::DispatchThreadgroupsIndirectArguments) + GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) + GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t) + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) + SOLUTION_SIZE * sizeof(uint32_t) + recoverEdgesParametersUsedSize;
				
				// Set max GPU work group memory size
				const size_t maxGpuWorkGroupMemorySize = max(max(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t) + max(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t) + GPU_BITMAP_SIZE, max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t) + max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t));
				
				// Display message
				cout << "Allocating " << (static_cast<double>(totalGpuMemoryAllocated) / BYTES_IN_A_GIGABYTE) << " GB of GPU memory and using at most " << (static_cast<double>(maxGpuWorkGroupMemorySize) / BYTES_IN_A_KILOBYTE) << " KB of GPU local memory" << endl;
				
				// Check if getting GPUs failed or no GPUs exist
				unique_ptr<NS::Array, void(*)(NS::Array *)> gpus(MTL::CopyAllDevices(), [](NS::Array *gpus) __attribute__((always_inline)) noexcept {
				
					// Free GPUs
					__builtin_assume_dereferenceable(gpus, sizeof(*gpus));
					gpus->release();
				});
				if(!gpus || !gpus->count()) [[unlikely]] {
				
					// Check if setting GPUs to include only the system's default GPU failed
					gpus = unique_ptr<NS::Array, void(*)(NS::Array *)>(NS::Array::alloc()->init((const NS::Object *[]){
					
						// System's default GPU
						unique_ptr<MTL::Device, void(*)(MTL::Device *)>(MTL::CreateSystemDefaultDevice(), [](MTL::Device *systemsDefaultGpu) __attribute__((always_inline)) noexcept {
						
							// Free system's default GPU
							__builtin_assume_dereferenceable(systemsDefaultGpu, sizeof(*systemsDefaultGpu));
							systemsDefaultGpu->release();
							
						}).get()
						
					}, 1), [](NS::Array *gpus) __attribute__((always_inline)) noexcept {
					
						// Free GPUs
						__builtin_assume_dereferenceable(gpus, sizeof(*gpus));
						gpus->release();
					});
					if(!gpus || !gpus->object(0)) [[unlikely]] {
					
						// Display message
						cout << "Getting GPUs failed" << endl;
						
						// Break
						break;
					}
				}
				
				// Go through all GPUs
				const NS::UInteger numberOfGpus = gpus->count();
				__builtin_assume(numberOfGpus > 0);
				for(NS::UInteger i = 0; i < numberOfGpus; ++i) [[likely]] {
				
					// Check if current GPU exists
					MTL::Device *currentGpu = gpus->object<MTL::Device>(i);
					if(currentGpu) [[likely]] {
					
						// Check if current GPU supports the Metal version, has enough work group memory, and has a name
						const NS::String *name = currentGpu->name();
						const char *nameAsUtf8String;
						if(currentGpu->supportsFamily(MTL::GPUFamilyMetal4) && currentGpu->maxThreadgroupMemoryLength() >= maxGpuWorkGroupMemorySize && name && (nameAsUtf8String = name->utf8String())) [[likely]] {
						
							// Check if GPU is built in
							int setGpuMemorySizeResult = -1;
							if(gpu->location() == MTL::DeviceLocationBuiltIn) [[likely]] {
							
								// Check if getting the GPU's memory size was successful
								gpuMemorySize = 0;
								size_t gpuMemorySizeSize = sizeof(gpuMemorySize);
								if(!sysctlbyname("iogpu.wired_limit_mb", &gpuMemorySize, &gpuMemorySizeSize, nullptr, 0)) [[likely]] {
								
									// Get new GPU memory size
									decltype(gpuMemorySize) newGpuMemorySize = (totalGpuMemoryAllocated + BYTES_IN_A_GIGABYTE - 1) / BYTES_IN_A_GIGABYTE * MEGABYTES_IN_A_GIGABYTE;
									
									// Check if new GPU memory size including additional space will overflow
									if(GPU_SET_MEMORY_SIZE_ADDITIONAL_SPACE_MEGABYTES > numeric_limits<decltype(newGpuMemorySize)>::max() - newGpuMemorySize) [[unlikely]] {
									
										// Set new GPU memory size to its max value
										newGpuMemorySize = numeric_limits<decltype(newGpuMemorySize)>::max();
									}
									
									// Otherwise
									else [[likely]] {
									
										// Add additional space to new GPU memory size
										newGpuMemorySize += GPU_SET_MEMORY_SIZE_ADDITIONAL_SPACE_MEGABYTES;
									}
									
									// Check if setting the GPU's memory size failed
									setGpuMemorySizeResult = sysctlbyname("iogpu.wired_limit_mb", nullptr, 0, &newGpuMemorySize, sizeof(newGpuMemorySize));
									if(setGpuMemorySizeResult) [[unlikely]] {
									
										// Display message
										cout << "Setting the " << nameAsUtf8String << " GPU's memory size to " << newGpuMemorySize << "MB failed" << endl;
										
										// Check if failed due to lacking privileges
										if(errno == EPERM) [[likely]] {
										
											// Display message
											cout << "If this program fails to run then run this program as root so that it can set the GPU's memory size by running the following command in a terminal: sudo \"" << (__builtin_expect(argv != nullptr, true) ? argv[0] : "") << '"' << endl;
										}
									}
								}
							}
							
							// Check if GPU has enough memory or its memory size was successfully set
							if(currentGpu->recommendedMaxWorkingSetSize() >= totalGpuMemoryAllocated || !setGpuMemorySizeResult) [[likely]] {
							
								// Set GPU to the current GPU and don't free it when GPUs is freed
								gpu = unique_ptr<MTL::Device, void(*)(MTL::Device *)>(currentGpu->retain(), [](MTL::Device *gpu) __attribute__((always_inline)) noexcept {
								
									// Free GPU
									__builtin_assume_dereferenceable(gpu, sizeof(*gpu));
									gpu->release();
								});
								
								// Automatically reset the GPU's memory size when done if it successfully set it
								gpuMemorySizeUniquePointer = unique_ptr<decltype(gpuMemorySize), void(*)(decltype(gpuMemorySize) *)>(__builtin_expect(setGpuMemorySizeResult, false) ? nullptr : &gpuMemorySize, [](decltype(gpuMemorySize) *gpuMemorySize) __attribute__((always_inline)) noexcept {
								
									// Reset the GPU's memory size
									sysctlbyname("iogpu.wired_limit_mb", nullptr, 0, gpuMemorySize, sizeof(*gpuMemorySize));
								});
								
								// Display message
								cout << "Using the " << nameAsUtf8String << " GPU" << endl;
								
								// Break
								break;
							}
							
							// Otherwise check if GPU's memory size was successfully set
							else if(!setGpuMemorySizeResult) [[unlikely]] {
								
								// Check if resetting the GPU's memory size failed
								if(sysctlbyname("iogpu.wired_limit_mb", nullptr, 0, &gpuMemorySize, sizeof(gpuMemorySize))) [[unlikely]] {
								
									// Display message
									cout << "Resetting the " << nameAsUtf8String << " GPU's memory size failed" << endl;
								}
							}
						}
					}
				}
				
				// Check if no applicable GPUs exist
				if(!gpu) [[unlikely]] {
				
					// Display message
					cout << "No applicable GPUs exist" << endl;
					
					// Break
					break;
				}
				
				// Check if creating preprocessor macros for the kernels failed
				const unique_ptr<NS::Dictionary, void(*)(NS::Dictionary *)> preprocessorMacros(NS::Dictionary::alloc()->init((const NS::Object *[]){
				
					// Edge bits value
					MTLSTR(TO_STRING(EDGE_BITS)),
					
					// Solution size value
					MTLSTR(TO_STRING(SOLUTION_SIZE)),
					
					// GPU trimming use max RAM value
					MTLSTR(TO_STRING(GPU_TRIMMING_USE_MAX_RAM)),
					
					// GPU trimming use more RAM value
					MTLSTR(TO_STRING(GPU_TRIMMING_USE_MORE_RAM)),
					
					// GPU trimming use less RAM value
					MTLSTR(TO_STRING(GPU_TRIMMING_USE_LESS_RAM)),
					
					// GPU trimming use min RAM value
					MTLSTR(TO_STRING(GPU_TRIMMING_USE_MIN_RAM)),
					
					// GPU number of most significant bits used for coarse bucket sorting value
					MTLSTR(TO_STRING(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)),
					
					// GPU number of most significant bits used for initial fine bucket sorting value
					MTLSTR(TO_STRING(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING)),
					
					// GPU number of most significant bits used for fine bucket sorting value
					MTLSTR(TO_STRING(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING)),
					
					// GPU fine bucket sort initial edges kernel number of work items per work group value
					MTLSTR(TO_STRING(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)),
					
					// GPU trim initial edges kernel number of work items per work group value
					MTLSTR(TO_STRING(GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)),
					
					// GPU fine bucket sort intermediate edges kernel number of work items per work group value
					MTLSTR(TO_STRING(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)),
					
					// GPU trim intermediate edges kernel number of work items per work group value
					MTLSTR(TO_STRING(GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)),
					
					// GPU fine bucket sort final edges kernel number of work items per work group value
					MTLSTR(TO_STRING(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)),
					
					// GPU trim final edges kernel number of work items per work group value
					MTLSTR(TO_STRING(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)),
					
					// CPU number of most significant bits used for coarse bucket sorting value
					MTLSTR(TO_STRING(CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)),
					
					// GPU trim final edges and transfer edges kernel number of work items per work group value
					MTLSTR(TO_STRING(GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP)),
					
					// GPU recover edges kernel number of edges per work item value
					MTLSTR(TO_STRING(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM)),
					
					// GPU recover edges kernel number of recovered edge candidates per work item value
					MTLSTR(TO_STRING(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM)),
					
					// GPU max number of edges per coarse bucket value
					unique_ptr<NS::String, void(*)(NS::String *)>(NS::String::alloc()->init(toString<GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET>::cString, NS::UTF8StringEncoding), [](NS::String *gpuMaxNumberOfEdgesPerCoarseBucketValue) __attribute__((always_inline)) noexcept {
					
						// Free GPU max number of edges per coarse bucket value
						__builtin_assume_dereferenceable(gpuMaxNumberOfEdgesPerCoarseBucketValue, sizeof(*gpuMaxNumberOfEdgesPerCoarseBucketValue));
						gpuMaxNumberOfEdgesPerCoarseBucketValue->release();
						
					}).get(),
					
					// GPU max number of edges per initial fine bucket value
					unique_ptr<NS::String, void(*)(NS::String *)>(NS::String::alloc()->init(toString<GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET>::cString, NS::UTF8StringEncoding), [](NS::String *gpuMaxNumberOfEdgesPerInitialFineBucketValue) __attribute__((always_inline)) noexcept {
					
						// Free GPU max number of edges per initial fine bucket value
						__builtin_assume_dereferenceable(gpuMaxNumberOfEdgesPerInitialFineBucketValue, sizeof(*gpuMaxNumberOfEdgesPerInitialFineBucketValue));
						gpuMaxNumberOfEdgesPerInitialFineBucketValue->release();
						
					}).get(),
					
					// GPU max number of edges per fine bucket value
					unique_ptr<NS::String, void(*)(NS::String *)>(NS::String::alloc()->init(toString<GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET>::cString, NS::UTF8StringEncoding), [](NS::String *gpuMaxNumberOfEdgesPerFineBucketValue) __attribute__((always_inline)) noexcept {
					
						// Free GPU max number of edges per fine bucket value
						__builtin_assume_dereferenceable(gpuMaxNumberOfEdgesPerFineBucketValue, sizeof(*gpuMaxNumberOfEdgesPerFineBucketValue));
						gpuMaxNumberOfEdgesPerFineBucketValue->release();
						
					}).get(),
					
					// CPU max number of edges per coarse bucket value
					unique_ptr<NS::String, void(*)(NS::String *)>(NS::String::alloc()->init(toString<CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET>::cString, NS::UTF8StringEncoding), [](NS::String *cpuMaxNumberOfEdgesPerCoarseBucketValue) __attribute__((always_inline)) noexcept {
					
						// Free CPU max number of edges per coarse bucket value
						__builtin_assume_dereferenceable(cpuMaxNumberOfEdgesPerCoarseBucketValue, sizeof(*cpuMaxNumberOfEdgesPerCoarseBucketValue));
						cpuMaxNumberOfEdgesPerCoarseBucketValue->release();
						
					}).get(),
					
					// GPU number of recovering edges value
					unique_ptr<NS::String, void(*)(NS::String *)>(NS::String::alloc()->init(toString<GPU_NUMBER_OF_RECOVERING_EDGES>::cString, NS::UTF8StringEncoding), [](NS::String *gpuNumberOfRecoveringEdgesValue) __attribute__((always_inline)) noexcept {
					
						// Free GPU number of recovering edges value
						__builtin_assume_dereferenceable(gpuNumberOfRecoveringEdgesValue, sizeof(*gpuNumberOfRecoveringEdgesValue));
						gpuNumberOfRecoveringEdgesValue->release();
						
					}).get()
					
				}, (const NS::Object *[]){
				
					// Edge bits key
					MTLSTR("EDGE_BITS"),
					
					// Solution size key
					MTLSTR("SOLUTION_SIZE"),
					
					// GPU trimming use max RAM key
					MTLSTR("GPU_TRIMMING_USE_MAX_RAM"),
					
					// GPU trimming use more RAM key
					MTLSTR("GPU_TRIMMING_USE_MORE_RAM"),
					
					// GPU trimming use less RAM key
					MTLSTR("GPU_TRIMMING_USE_LESS_RAM"),
					
					// GPU trimming use min RAM key
					MTLSTR("GPU_TRIMMING_USE_MIN_RAM"),
					
					// GPU number of most significant bits used for coarse bucket sorting key
					MTLSTR("GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING"),
					
					// GPU number of most significant bits used for initial fine bucket sorting key
					MTLSTR("GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING"),
					
					// GPU number of most significant bits used for fine bucket sorting key
					MTLSTR("GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING"),
					
					// GPU fine bucket sort initial edges kernel number of work items per work group key
					MTLSTR("GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP"),
					
					// GPU trim initial edges kernel number of work items per work group key
					MTLSTR("GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP"),
					
					// GPU fine bucket sort intermediate edges kernel number of work items per work group key
					MTLSTR("GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP"),
					
					// GPU trim intermediate edges kernel number of work items per work group key
					MTLSTR("GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP"),
					
					// GPU fine bucket sort final edges kernel number of work items per work group key
					MTLSTR("GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP"),
					
					// GPU trim final edges kernel number of work items per work group key
					MTLSTR("GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP"),
					
					// CPU number of most significant bits used for coarse bucket sorting key
					MTLSTR("CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING"),
					
					// GPU trim final edges and transfer edges kernel number of work items per work group key
					MTLSTR("GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP"),
					
					// GPU recover edges kernel number of edges per work item key
					MTLSTR("GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM"),
					
					// GPU recover edges kernel number of recovered edge candidates per work item key
					MTLSTR("GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM"),
					
					// GPU max number of edges per coarse bucket key
					MTLSTR("GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET"),
					
					// GPU max number of edges per initial fine bucket key
					MTLSTR("GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET"),
					
					// GPU max number of edges per fine bucket key
					MTLSTR("GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET"),
					
					// CPU max number of edges per coarse bucket key
					MTLSTR("CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET"),
					
					// GPU number of recovering edges key
					MTLSTR("GPU_NUMBER_OF_RECOVERING_EDGES")
					
				}, 24), [](NS::Dictionary *preprocessorMacros) __attribute__((always_inline)) noexcept {
				
					// Free preprocessor macros
					__builtin_assume_dereferenceable(preprocessorMacros, sizeof(*preprocessorMacros));
					preprocessorMacros->release();
				});
				if(!preprocessorMacros || !preprocessorMacros->object(MTLSTR("GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET")) || !preprocessorMacros->object(MTLSTR("GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET")) || !preprocessorMacros->object(MTLSTR("GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET")) || !preprocessorMacros->object(MTLSTR("CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET")) || !preprocessorMacros->object(MTLSTR("GPU_NUMBER_OF_RECOVERING_EDGES"))) [[unlikely]] {
				
					// Display message
					cout << "Creating preprocessor macros for the kernels failed" << endl;
					
					// Break
					break;
				}
				
				// Check if creating compile options for the kernels failed
				const unique_ptr<MTL::CompileOptions, void(*)(MTL::CompileOptions *)> compileOptions(MTL::CompileOptions::alloc()->init(), [](MTL::CompileOptions *compileOptions) __attribute__((always_inline)) noexcept {
				
					// Free compile options
					__builtin_assume_dereferenceable(compileOptions, sizeof(*compileOptions));
					compileOptions->release();
				});
				if(!compileOptions) [[unlikely]] {
				
					// Display message
					cout << "Creating compile options for the kernels failed" << endl;
					
					// Break
					break;
				}
				
				// Configure compile options
				compileOptions->setPreprocessorMacros(preprocessorMacros.get());
				compileOptions->setLanguageVersion(METAL_TARGET_VERSION);
				
				// Check if embedding GPU code
				#if EMBED_GPU_CODE
				
					// Get source
					const NS::String *source = (
						#include "gpu.metal"
					);
					
				// Otherwise
				#else
				
					// Check if opening source file failed
					const unique_ptr<FILE, decltype(&fclose)> sourceFile(fopen("gpu.metal", "rb"), fclose);
					if(!sourceFile) [[unlikely]] {
					
						// Display message
						cout << "Opening gpu.metal failed" << endl;
						
						// Break
						break;
					}
					
					// Check if getting source file's size failed
					long sourceFileSize;
					if(fseek(sourceFile.get(), 0, SEEK_END) || (sourceFileSize = ftell(sourceFile.get())) == -1 || fseek(sourceFile.get(), 0, SEEK_SET) || static_cast<unsigned long>(sourceFileSize) > numeric_limits<NS::UInteger>::max()) [[unlikely]] {
					
						// Display message
						cout << "Getting gpu.metal's size failed" << endl;
						
						// Break
						break;
					}
					
					// Check if reading source file failed
					char sourceFileContents[sourceFileSize];
					if(fread(sourceFileContents, sizeof(char), sourceFileSize, sourceFile.get()) != sizeof(sourceFileContents)) [[unlikely]] {
					
						// Display message
						cout << "Reading gpu.metal failed" << endl;
						
						// Break
						break;
					}
					
					// Check if creating source failed
					const unique_ptr<NS::String, void(*)(NS::String *)> source(NS::String::alloc()->init(&sourceFileContents[sizeof("MTLSTR(R\"(") - sizeof('\0')], sourceFileSize - (sizeof("MTLSTR(R\"()\")\n") - sizeof('\0')), NS::UTF8StringEncoding, false), [](NS::String *source) __attribute__((always_inline)) noexcept {
					
						// Free source
						__builtin_assume_dereferenceable(source, sizeof(*source));
						source->release();
					});
					if(!source) [[unlikely]] {
					
						// Display message
						cout << "Creating source failed" << endl;
						
						// Break
						break;
					}
				#endif
				
				// Check if creating library for the GPU failed
				NS::Error *createLibraryError;
				const unique_ptr<MTL::Library, void(*)(MTL::Library *)> library(gpu->newLibrary(&*source, compileOptions.get(), &createLibraryError), [](MTL::Library *library) __attribute__((always_inline)) noexcept {
				
					// Free library
					__builtin_assume_dereferenceable(library, sizeof(*library));
					library->release();
				});
				if(!library) [[unlikely]] {
				
					// Display message
					cout << "Creating library for the GPU failed" << endl;
					
					// Check if an error exists
					if(createLibraryError) [[likely]] {
					
						// Check if error's localized description exists
						const NS::String *localizedDescription = createLibraryError->localizedDescription();
						const char *localizedDescriptionAsUtf8String;
						if(localizedDescription && (localizedDescriptionAsUtf8String = localizedDescription->utf8String())) [[likely]] {
						
							// Display message
							cout << localizedDescriptionAsUtf8String << endl;
						}
					}
					
					// Break
					break;
				}
				
				// Check if getting kernels from the library failed
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> coarseBucketSortEdgesKernel(library->newFunction(MTLSTR("coarseBucketSortEdges")), [](MTL::Function *coarseBucketSortEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free coarse bucket sort edges kernel
					__builtin_assume_dereferenceable(coarseBucketSortEdgesKernel, sizeof(*coarseBucketSortEdgesKernel));
					coarseBucketSortEdgesKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> updateLargestInitialCoarseBucketSizeKernel(library->newFunction(MTLSTR("updateLargestInitialCoarseBucketSize")), [](MTL::Function *updateLargestInitialCoarseBucketSizeKernel) __attribute__((always_inline)) noexcept {
				
					// Free update largest initial coarse bucket size kernel
					__builtin_assume_dereferenceable(updateLargestInitialCoarseBucketSizeKernel, sizeof(*updateLargestInitialCoarseBucketSizeKernel));
					updateLargestInitialCoarseBucketSizeKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> fineBucketSortInitialEdgesKernel(library->newFunction(MTLSTR("fineBucketSortInitialEdges")), [](MTL::Function *fineBucketSortInitialEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free fine bucket sort initial edges kernel
					__builtin_assume_dereferenceable(fineBucketSortInitialEdgesKernel, sizeof(*fineBucketSortInitialEdgesKernel));
					fineBucketSortInitialEdgesKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> trimInitialEdgesKernel(library->newFunction(MTLSTR("trimInitialEdges")), [](MTL::Function *trimInitialEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free trim initial edges kernel
					__builtin_assume_dereferenceable(trimInitialEdgesKernel, sizeof(*trimInitialEdgesKernel));
					trimInitialEdgesKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> updateLargestIntermediateCoarseBucketSizeKernel(library->newFunction(MTLSTR("updateLargestIntermediateCoarseBucketSize")), [](MTL::Function *updateLargestIntermediateCoarseBucketSizeKernel) __attribute__((always_inline)) noexcept {
				
					// Free update largest intermediate coarse bucket size kernel
					__builtin_assume_dereferenceable(updateLargestIntermediateCoarseBucketSizeKernel, sizeof(*updateLargestIntermediateCoarseBucketSizeKernel));
					updateLargestIntermediateCoarseBucketSizeKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> fineBucketSortIntermediateEdgesKernel(library->newFunction(MTLSTR("fineBucketSortIntermediateEdges")), [](MTL::Function *fineBucketSortIntermediateEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free fine bucket sort intermediate edges kernel
					__builtin_assume_dereferenceable(fineBucketSortIntermediateEdgesKernel, sizeof(*fineBucketSortIntermediateEdgesKernel));
					fineBucketSortIntermediateEdgesKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> trimIntermediateEdgesKernel(library->newFunction(MTLSTR("trimIntermediateEdges")), [](MTL::Function *trimIntermediateEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free trim intermediate edges kernel
					__builtin_assume_dereferenceable(trimIntermediateEdgesKernel, sizeof(*trimIntermediateEdgesKernel));
					trimIntermediateEdgesKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> updateLargestFinalCoarseBucketSizeKernel(library->newFunction(MTLSTR("updateLargestFinalCoarseBucketSize")), [](MTL::Function *updateLargestFinalCoarseBucketSizeKernel) __attribute__((always_inline)) noexcept {
				
					// Free update largest final coarse bucket size kernel
					__builtin_assume_dereferenceable(updateLargestFinalCoarseBucketSizeKernel, sizeof(*updateLargestFinalCoarseBucketSizeKernel));
					updateLargestFinalCoarseBucketSizeKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> fineBucketSortFinalEdgesKernel(library->newFunction(MTLSTR("fineBucketSortFinalEdges")), [](MTL::Function *fineBucketSortFinalEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free fine bucket sort final edges kernel
					__builtin_assume_dereferenceable(fineBucketSortFinalEdgesKernel, sizeof(*fineBucketSortFinalEdgesKernel));
					fineBucketSortFinalEdgesKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> trimFinalEdgesKernel(library->newFunction(MTLSTR("trimFinalEdges")), [](MTL::Function *trimFinalEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free trim final edges kernel
					__builtin_assume_dereferenceable(trimFinalEdgesKernel, sizeof(*trimFinalEdgesKernel));
					trimFinalEdgesKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> trimFinalEdgesAndTransferEdgesKernel(library->newFunction(MTLSTR("trimFinalEdgesAndTransferEdges")), [](MTL::Function *trimFinalEdgesAndTransferEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free trim final edges and transfer edges kernel
					__builtin_assume_dereferenceable(trimFinalEdgesAndTransferEdgesKernel, sizeof(*trimFinalEdgesAndTransferEdgesKernel));
					trimFinalEdgesAndTransferEdgesKernel->release();
				});
				const unique_ptr<MTL::Function, void(*)(MTL::Function *)> recoverEdgesKernel(library->newFunction(MTLSTR("recoverEdges")), [](MTL::Function *recoverEdgesKernel) __attribute__((always_inline)) noexcept {
				
					// Free recover edges kernel
					__builtin_assume_dereferenceable(recoverEdgesKernel, sizeof(*recoverEdgesKernel));
					recoverEdgesKernel->release();
				});
				if(!coarseBucketSortEdgesKernel || !updateLargestInitialCoarseBucketSizeKernel || !fineBucketSortInitialEdgesKernel || !trimInitialEdgesKernel || !updateLargestIntermediateCoarseBucketSizeKernel || !fineBucketSortIntermediateEdgesKernel || !trimIntermediateEdgesKernel || !updateLargestFinalCoarseBucketSizeKernel || !fineBucketSortFinalEdgesKernel || !trimFinalEdgesKernel || !trimFinalEdgesAndTransferEdgesKernel || !recoverEdgesKernel) [[unlikely]] {
				
					// Display message
					cout << "Getting kernels from the library failed" << endl;
					
					// Break
					break;
				}
				
				// Check if creating pipelines for the GPU failed
				NS::Error *createPipelineError;
				coarseBucketSortEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(coarseBucketSortEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *coarseBucketSortEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free coarse bucket sort edges pipeline
					__builtin_assume_dereferenceable(coarseBucketSortEdgesPipeline, sizeof(*coarseBucketSortEdgesPipeline));
					coarseBucketSortEdgesPipeline->release();
				});
				updateLargestInitialCoarseBucketSizePipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(updateLargestInitialCoarseBucketSizeKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *updateLargestInitialCoarseBucketSizePipeline) __attribute__((always_inline)) noexcept {
				
					// Free update largest initial coarse bucket size pipeline
					__builtin_assume_dereferenceable(updateLargestInitialCoarseBucketSizePipeline, sizeof(*updateLargestInitialCoarseBucketSizePipeline));
					updateLargestInitialCoarseBucketSizePipeline->release();
				});
				fineBucketSortInitialEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(fineBucketSortInitialEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *fineBucketSortInitialEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free fine bucket sort initial edges pipeline
					__builtin_assume_dereferenceable(fineBucketSortInitialEdgesPipeline, sizeof(*fineBucketSortInitialEdgesPipeline));
					fineBucketSortInitialEdgesPipeline->release();
				});
				trimInitialEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(trimInitialEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *trimInitialEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free trim initial edges pipeline
					__builtin_assume_dereferenceable(trimInitialEdgesPipeline, sizeof(*trimInitialEdgesPipeline));
					trimInitialEdgesPipeline->release();
				});
				updateLargestIntermediateCoarseBucketSizePipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(updateLargestIntermediateCoarseBucketSizeKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *updateLargestIntermediateCoarseBucketSizePipeline) __attribute__((always_inline)) noexcept {
				
					// Free update largest intermediate coarse bucket size pipeline
					__builtin_assume_dereferenceable(updateLargestIntermediateCoarseBucketSizePipeline, sizeof(*updateLargestIntermediateCoarseBucketSizePipeline));
					updateLargestIntermediateCoarseBucketSizePipeline->release();
				});
				fineBucketSortIntermediateEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(fineBucketSortIntermediateEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *fineBucketSortIntermediateEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free fine bucket sort intermediate edges pipeline
					__builtin_assume_dereferenceable(fineBucketSortIntermediateEdgesPipeline, sizeof(*fineBucketSortIntermediateEdgesPipeline));
					fineBucketSortIntermediateEdgesPipeline->release();
				});
				trimIntermediateEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(trimIntermediateEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *trimIntermediateEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free trim intermediate edges pipeline
					__builtin_assume_dereferenceable(trimIntermediateEdgesPipeline, sizeof(*trimIntermediateEdgesPipeline));
					trimIntermediateEdgesPipeline->release();
				});
				updateLargestFinalCoarseBucketSizePipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(updateLargestFinalCoarseBucketSizeKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *updateLargestFinalCoarseBucketSizePipeline) __attribute__((always_inline)) noexcept {
				
					// Free update largest final coarse bucket size pipeline
					__builtin_assume_dereferenceable(updateLargestFinalCoarseBucketSizePipeline, sizeof(*updateLargestFinalCoarseBucketSizePipeline));
					updateLargestFinalCoarseBucketSizePipeline->release();
				});
				fineBucketSortFinalEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(fineBucketSortFinalEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *fineBucketSortFinalEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free fine bucket sort final edges pipeline
					__builtin_assume_dereferenceable(fineBucketSortFinalEdgesPipeline, sizeof(*fineBucketSortFinalEdgesPipeline));
					fineBucketSortFinalEdgesPipeline->release();
				});
				trimFinalEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(trimFinalEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *trimFinalEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free trim final edges pipeline
					__builtin_assume_dereferenceable(trimFinalEdgesPipeline, sizeof(*trimFinalEdgesPipeline));
					trimFinalEdgesPipeline->release();
				});
				trimFinalEdgesAndTransferEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(trimFinalEdgesAndTransferEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *trimFinalEdgesAndTransferEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free trim final edges and transfer edges pipeline
					__builtin_assume_dereferenceable(trimFinalEdgesAndTransferEdgesPipeline, sizeof(*trimFinalEdgesAndTransferEdgesPipeline));
					trimFinalEdgesAndTransferEdgesPipeline->release();
				});
				recoverEdgesPipeline = unique_ptr<MTL::ComputePipelineState, void(*)(MTL::ComputePipelineState *)>(gpu->newComputePipelineState(recoverEdgesKernel.get(), &createPipelineError), [](MTL::ComputePipelineState *recoverEdgesPipeline) __attribute__((always_inline)) noexcept {
				
					// Free recover edges pipeline
					__builtin_assume_dereferenceable(recoverEdgesPipeline, sizeof(*recoverEdgesPipeline));
					recoverEdgesPipeline->release();
				});
				if(!coarseBucketSortEdgesPipeline || !updateLargestInitialCoarseBucketSizePipeline || !fineBucketSortInitialEdgesPipeline || !trimInitialEdgesPipeline || !updateLargestIntermediateCoarseBucketSizePipeline || !fineBucketSortIntermediateEdgesPipeline || !trimIntermediateEdgesPipeline || !updateLargestFinalCoarseBucketSizePipeline || !fineBucketSortFinalEdgesPipeline || !trimFinalEdgesPipeline || !trimFinalEdgesAndTransferEdgesPipeline || !recoverEdgesPipeline) [[unlikely]] {
				
					// Display message
					cout << "Creating pipelines for the GPU failed" << endl;
					
					// Break
					break;
				}
			}
			
			// Throw error if GPU data type's sizes or alignments are invalid
			static_assert(sizeof(TrimEdgesParameters) == sizeof(uint64_t[4]) && sizeof(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS)))) == sizeof(uint64_t[4]) && alignof(TrimEdgesParameters) == alignof(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS)))), "GPU data type's sizes or alignments are invalid");
			
			// Check if allocating memory on the GPU failed
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> coarseBucketsBuffer(gpu->newBuffer(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_COARSE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, MTL::ResourceStorageModePrivate), [](MTL::Buffer *coarseBucketsBuffer) __attribute__((always_inline)) noexcept {
			
				// Free coarse bucket buffer
				__builtin_assume_dereferenceable(coarseBucketsBuffer, sizeof(*coarseBucketsBuffer));
				coarseBucketsBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> numberOfEdgesPerCoarseBucketBuffer(gpu->newBuffer(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), MTL::ResourceStorageModePrivate), [](MTL::Buffer *numberOfEdgesPerCoarseBucketBuffer) __attribute__((always_inline)) noexcept {
			
				// Free number of edges per coarse bucket buffer
				__builtin_assume_dereferenceable(numberOfEdgesPerCoarseBucketBuffer, sizeof(*numberOfEdgesPerCoarseBucketBuffer));
				numberOfEdgesPerCoarseBucketBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> trimEdgesParametersBuffer(gpu->newBuffer(sizeof(TrimEdgesParameters), MTL::ResourceStorageModeShared), [](MTL::Buffer *trimEdgesParametersBuffer) __attribute__((always_inline)) noexcept {
			
				// Free trim edges parameters buffer
				__builtin_assume_dereferenceable(trimEdgesParametersBuffer, sizeof(*trimEdgesParametersBuffer));
				trimEdgesParametersBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> largestCoarseBucketSizeBuffer(gpu->newBuffer((const MTL::DispatchThreadgroupsIndirectArguments []){{0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, 1}}, sizeof(MTL::DispatchThreadgroupsIndirectArguments), MTL::ResourceStorageModePrivate), [](MTL::Buffer *largestCoarseBucketSizeBuffer) __attribute__((always_inline)) noexcept {
			
				// Free largest coarse bucket size buffer
				__builtin_assume_dereferenceable(largestCoarseBucketSizeBuffer, sizeof(*largestCoarseBucketSizeBuffer));
				largestCoarseBucketSizeBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> fineBucketsBuffer(gpu->newBuffer(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET), MTL::ResourceStorageModePrivate), [](MTL::Buffer *fineBucketsBuffer) __attribute__((always_inline)) noexcept {
			
				// Free fine bucket buffer
				__builtin_assume_dereferenceable(fineBucketsBuffer, sizeof(*fineBucketsBuffer));
				fineBucketsBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> numberOfEdgesPerFineBucketBuffer(gpu->newBuffer(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t), MTL::ResourceStorageModePrivate), [](MTL::Buffer *numberOfEdgesPerFineBucketBuffer) __attribute__((always_inline)) noexcept {
			
				// Free number of edges per fine bucket buffer
				__builtin_assume_dereferenceable(numberOfEdgesPerFineBucketBuffer, sizeof(*numberOfEdgesPerFineBucketBuffer));
				numberOfEdgesPerFineBucketBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> cpuBucketsBuffer(gpu->newBuffer(CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, MTL::ResourceStorageModeShared), [](MTL::Buffer *cpuBucketsBuffer) __attribute__((always_inline)) noexcept {
			
				// Free CPU bucket buffer
				__builtin_assume_dereferenceable(cpuBucketsBuffer, sizeof(*cpuBucketsBuffer));
				cpuBucketsBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> numberOfEdgesPerCpuBucketBuffer(gpu->newBuffer(CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), MTL::ResourceStorageModeShared), [](MTL::Buffer *numberOfEdgesPerCpuBucketBuffer) __attribute__((always_inline)) noexcept {
			
				// Free number of edges per CPU bucket buffer
				__builtin_assume_dereferenceable(numberOfEdgesPerCpuBucketBuffer, sizeof(*numberOfEdgesPerCpuBucketBuffer));
				numberOfEdgesPerCpuBucketBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> solutionEdgesBuffer(gpu->newBuffer(SOLUTION_SIZE * sizeof(uint32_t), MTL::ResourceStorageModeShared), [](MTL::Buffer *solutionEdgesBuffer) __attribute__((always_inline)) noexcept {
			
				// Free solution edges buffer
				__builtin_assume_dereferenceable(solutionEdgesBuffer, sizeof(*solutionEdgesBuffer));
				solutionEdgesBuffer->release();
			});
			const unique_ptr<MTL::Buffer, void(*)(MTL::Buffer *)> recoverEdgesParametersBuffer(gpu->newBuffer(recoverEdgesParametersUsedSize, MTL::ResourceStorageModeShared), [](MTL::Buffer *recoverEdgesParametersBuffer) __attribute__((always_inline)) noexcept {
			
				// Free recover edges parameters buffer
				__builtin_assume_dereferenceable(recoverEdgesParametersBuffer, sizeof(*recoverEdgesParametersBuffer));
				recoverEdgesParametersBuffer->release();
			});
			if(!coarseBucketsBuffer || !numberOfEdgesPerCoarseBucketBuffer || !trimEdgesParametersBuffer || !largestCoarseBucketSizeBuffer || !fineBucketsBuffer || !numberOfEdgesPerFineBucketBuffer || !cpuBucketsBuffer || !numberOfEdgesPerCpuBucketBuffer || !solutionEdgesBuffer || !recoverEdgesParametersBuffer) [[unlikely]] {
			
				// Display message
				cout << "Allocating memory on the GPU failed" << endl;
				
				// Break
				break;
			}
			
			// Create block
			unique_ptr<MTL::ResidencySet, void(*)(MTL::ResidencySet *)> residencySet(nullptr, [](MTL::ResidencySet *residencySet) __attribute__((always_inline)) noexcept {
			
				// Free residency set
				__builtin_assume_dereferenceable(residencySet, sizeof(*residencySet));
				residencySet->release();
			});
			unique_ptr<MTL4::ArgumentTable, void(*)(MTL4::ArgumentTable *)> trimEdgesArgumentTable(nullptr, [](MTL4::ArgumentTable *trimEdgesArgumentTable) __attribute__((always_inline)) noexcept {
			
				// Free trim edges argument table
				__builtin_assume_dereferenceable(trimEdgesArgumentTable, sizeof(*trimEdgesArgumentTable));
				trimEdgesArgumentTable->release();
			});
			unique_ptr<MTL4::ArgumentTable, void(*)(MTL4::ArgumentTable *)> transferEdgesArgumentTable(nullptr, [](MTL4::ArgumentTable *transferEdgesArgumentTable) __attribute__((always_inline)) noexcept {
			
				// Free transfer edges argument table
				__builtin_assume_dereferenceable(transferEdgesArgumentTable, sizeof(*transferEdgesArgumentTable));
				transferEdgesArgumentTable->release();
			});
			unique_ptr<MTL4::ArgumentTable, void(*)(MTL4::ArgumentTable *)> recoverEdgesArgumentTable(nullptr, [](MTL4::ArgumentTable *recoverEdgesArgumentTable) __attribute__((always_inline)) noexcept {
			
				// Free recover edges argument table
				__builtin_assume_dereferenceable(recoverEdgesArgumentTable, sizeof(*recoverEdgesArgumentTable));
				recoverEdgesArgumentTable->release();
			});
			{
			
				// Check if creating residency set descriptor for the residency set failed
				const unique_ptr<MTL::ResidencySetDescriptor, void(*)(MTL::ResidencySetDescriptor *)> residencySetDescriptor(MTL::ResidencySetDescriptor::alloc()->init(), [](MTL::ResidencySetDescriptor *residencySetDescriptor) __attribute__((always_inline)) noexcept {
				
					// Free residency set descriptor
					__builtin_assume_dereferenceable(residencySetDescriptor, sizeof(*residencySetDescriptor));
					residencySetDescriptor->release();
				});
				if(!residencySetDescriptor) [[unlikely]] {
				
					// Display message
					cout << "Creating residency set descriptor for the residency set failed" << endl;
					
					// Break
					break;
				}
				
				// Configure residency set descriptor
				residencySetDescriptor->setInitialCapacity(10);
				
				// Check if creating residency set for the GPU failed
				NS::Error *createResidencySetError;
				residencySet = unique_ptr<MTL::ResidencySet, void(*)(MTL::ResidencySet *)>(gpu->newResidencySet(residencySetDescriptor.get(), &createResidencySetError), [](MTL::ResidencySet *residencySet) __attribute__((always_inline)) noexcept {
				
					// Free residency set
					__builtin_assume_dereferenceable(residencySet, sizeof(*residencySet));
					residencySet->release();
				});
				if(!residencySet) [[unlikely]] {
				
					// Display message
					cout << "Creating residency set for the GPU failed" << endl;
					
					// Break
					break;
				}
				
				// Configure residency set
				residencySet->addAllocation(coarseBucketsBuffer.get());
				residencySet->addAllocation(numberOfEdgesPerCoarseBucketBuffer.get());
				residencySet->addAllocation(trimEdgesParametersBuffer.get());
				residencySet->addAllocation(largestCoarseBucketSizeBuffer.get());
				residencySet->addAllocation(fineBucketsBuffer.get());
				residencySet->addAllocation(numberOfEdgesPerFineBucketBuffer.get());
				residencySet->addAllocation(cpuBucketsBuffer.get());
				residencySet->addAllocation(numberOfEdgesPerCpuBucketBuffer.get());
				residencySet->addAllocation(solutionEdgesBuffer.get());
				residencySet->addAllocation(recoverEdgesParametersBuffer.get());
				residencySet->commit();
				residencySet->requestResidency();
				
				// Check if creating argument table descriptor for the arguments table failed
				const unique_ptr<MTL4::ArgumentTableDescriptor, void(*)(MTL4::ArgumentTableDescriptor *)> argumentTableDescriptor(MTL4::ArgumentTableDescriptor::alloc()->init(), [](MTL4::ArgumentTableDescriptor *argumentTableDescriptor) __attribute__((always_inline)) noexcept {
				
					// Free argument table descriptor
					__builtin_assume_dereferenceable(argumentTableDescriptor, sizeof(*argumentTableDescriptor));
					argumentTableDescriptor->release();
				});
				if(!argumentTableDescriptor) [[unlikely]] {
				
					// Display message
					cout << "Creating argument table descriptor for the argument tables failed" << endl;
					
					// Break
					break;
				}
				
				// Check if creating argument tables for the GPU failed
				NS::Error *createArgumentTableError;
				argumentTableDescriptor->setMaxBufferBindCount(6);
				trimEdgesArgumentTable = unique_ptr<MTL4::ArgumentTable, void(*)(MTL4::ArgumentTable *)>(gpu->newArgumentTable(argumentTableDescriptor.get(), &createArgumentTableError), [](MTL4::ArgumentTable *trimEdgesArgumentTable) __attribute__((always_inline)) noexcept {
				
					// Free trim edges argument table
					__builtin_assume_dereferenceable(trimEdgesArgumentTable, sizeof(*trimEdgesArgumentTable));
					trimEdgesArgumentTable->release();
				});
				
				argumentTableDescriptor->setMaxBufferBindCount(4);
				transferEdgesArgumentTable = unique_ptr<MTL4::ArgumentTable, void(*)(MTL4::ArgumentTable *)>(gpu->newArgumentTable(argumentTableDescriptor.get(), &createArgumentTableError), [](MTL4::ArgumentTable *transferEdgesArgumentTable) __attribute__((always_inline)) noexcept {
				
					// Free transfer edges argument table
					__builtin_assume_dereferenceable(transferEdgesArgumentTable, sizeof(*transferEdgesArgumentTable));
					transferEdgesArgumentTable->release();
				});
				
				argumentTableDescriptor->setMaxBufferBindCount(2);
				recoverEdgesArgumentTable = unique_ptr<MTL4::ArgumentTable, void(*)(MTL4::ArgumentTable *)>(gpu->newArgumentTable(argumentTableDescriptor.get(), &createArgumentTableError), [](MTL4::ArgumentTable *recoverEdgesArgumentTable) __attribute__((always_inline)) noexcept {
				
					// Free recover edges argument table
					__builtin_assume_dereferenceable(recoverEdgesArgumentTable, sizeof(*recoverEdgesArgumentTable));
					recoverEdgesArgumentTable->release();
				});
				if(!trimEdgesArgumentTable || !transferEdgesArgumentTable || !recoverEdgesArgumentTable) [[unlikely]] {
				
					// Display message
					cout << "Creating argument tables for the GPU failed" << endl;
					
					// Break
					break;
				}
				
				// Configure argument tables
				trimEdgesArgumentTable->setAddress(coarseBucketsBuffer->gpuAddress(), 0);
				trimEdgesArgumentTable->setAddress(numberOfEdgesPerCoarseBucketBuffer->gpuAddress(), 1);
				trimEdgesArgumentTable->setAddress(trimEdgesParametersBuffer->gpuAddress(), 2);
				trimEdgesArgumentTable->setAddress(largestCoarseBucketSizeBuffer->gpuAddress(), 3);
				trimEdgesArgumentTable->setAddress(fineBucketsBuffer->gpuAddress(), 4);
				trimEdgesArgumentTable->setAddress(numberOfEdgesPerFineBucketBuffer->gpuAddress(), 5);
				
				transferEdgesArgumentTable->setAddress(cpuBucketsBuffer->gpuAddress(), 0);
				transferEdgesArgumentTable->setAddress(numberOfEdgesPerCpuBucketBuffer->gpuAddress(), 1);
				transferEdgesArgumentTable->setAddress(fineBucketsBuffer->gpuAddress(), 2);
				transferEdgesArgumentTable->setAddress(numberOfEdgesPerFineBucketBuffer->gpuAddress(), 3);
				
				recoverEdgesArgumentTable->setAddress(solutionEdgesBuffer->gpuAddress(), 0);
				recoverEdgesArgumentTable->setAddress(recoverEdgesParametersBuffer->gpuAddress(), 1);
			}
			
			// Check if creating command queue for the GPU failed
			const unique_ptr<MTL4::CommandQueue, void(*)(MTL4::CommandQueue *)> commandQueue(gpu->newMTL4CommandQueue(), [](MTL4::CommandQueue *commandQueue) __attribute__((always_inline)) noexcept {
			
				// Free command queue
				__builtin_assume_dereferenceable(commandQueue, sizeof(*commandQueue));
				commandQueue->release();
			});
			if(!commandQueue) [[unlikely]] {
			
				// Display message
				cout << "Creating command queue for the GPU failed" << endl;
				
				// Break
				break;
			}
			
			// Configure command queue
			commandQueue->addResidencySet(residencySet.get());
			
			// Check if creating command allocators for the GPU failed
			const unique_ptr<MTL4::CommandAllocator, void(*)(MTL4::CommandAllocator *)> trimEdgesCommandAllocator(gpu->newCommandAllocator(), [](MTL4::CommandAllocator *trimEdgesCommandAllocator) __attribute__((always_inline)) noexcept {
			
				// Free trim edges command allocator
				__builtin_assume_dereferenceable(trimEdgesCommandAllocator, sizeof(*trimEdgesCommandAllocator));
				trimEdgesCommandAllocator->release();
			});
			const unique_ptr<MTL4::CommandAllocator, void(*)(MTL4::CommandAllocator *)> transferEdgesCommandAllocator[] = {
			
				unique_ptr<MTL4::CommandAllocator, void(*)(MTL4::CommandAllocator *)>(gpu->newCommandAllocator(), [](MTL4::CommandAllocator *transferEdgesCommandAllocator) __attribute__((always_inline)) noexcept {
				
					// Free transfer edges command allocator
					__builtin_assume_dereferenceable(transferEdgesCommandAllocator, sizeof(*transferEdgesCommandAllocator));
					transferEdgesCommandAllocator->release();
				}),
				
				unique_ptr<MTL4::CommandAllocator, void(*)(MTL4::CommandAllocator *)>(gpu->newCommandAllocator(), [](MTL4::CommandAllocator *transferEdgesCommandAllocator) __attribute__((always_inline)) noexcept {
				
					// Free transfer edges command allocator
					__builtin_assume_dereferenceable(transferEdgesCommandAllocator, sizeof(*transferEdgesCommandAllocator));
					transferEdgesCommandAllocator->release();
				})
			};
			const unique_ptr<MTL4::CommandAllocator, void(*)(MTL4::CommandAllocator *)> recoverEdgesCommandAllocator(gpu->newCommandAllocator(), [](MTL4::CommandAllocator *recoverEdgesCommandAllocator) __attribute__((always_inline)) noexcept {
			
				// Free recover edges command allocator
				__builtin_assume_dereferenceable(recoverEdgesCommandAllocator, sizeof(*recoverEdgesCommandAllocator));
				recoverEdgesCommandAllocator->release();
			});
			if(!trimEdgesCommandAllocator || !transferEdgesCommandAllocator[0] || !transferEdgesCommandAllocator[1] || !recoverEdgesCommandAllocator) [[unlikely]] {
			
				// Display message
				cout << "Creating command allocators for the GPU failed" << endl;
				
				// Break
				break;
			}
			
			// Check if creating command buffers for the GPU failed
			const unique_ptr<MTL4::CommandBuffer, void(*)(MTL4::CommandBuffer *)> trimEdgesCommandBuffer(gpu->newCommandBuffer(), [](MTL4::CommandBuffer *trimEdgesCommandBuffer) __attribute__((always_inline)) noexcept {
			
				// Free trim edges command buffer
				__builtin_assume_dereferenceable(trimEdgesCommandBuffer, sizeof(*trimEdgesCommandBuffer));
				trimEdgesCommandBuffer->release();
			});
			const unique_ptr<MTL4::CommandBuffer, void(*)(MTL4::CommandBuffer *)> transferEdgesCommandBuffer(gpu->newCommandBuffer(), [](MTL4::CommandBuffer *transferEdgesCommandBuffer) __attribute__((always_inline)) noexcept {
			
				// Free transfer edges command buffer
				__builtin_assume_dereferenceable(transferEdgesCommandBuffer, sizeof(*transferEdgesCommandBuffer));
				transferEdgesCommandBuffer->release();
			});
			const unique_ptr<MTL4::CommandBuffer, void(*)(MTL4::CommandBuffer *)> recoverEdgesCommandBuffer(gpu->newCommandBuffer(), [](MTL4::CommandBuffer *recoverEdgesCommandBuffer) __attribute__((always_inline)) noexcept {
			
				// Free recover edges command buffer
				__builtin_assume_dereferenceable(recoverEdgesCommandBuffer, sizeof(*recoverEdgesCommandBuffer));
				recoverEdgesCommandBuffer->release();
			});
			if(!trimEdgesCommandBuffer || !transferEdgesCommandBuffer || !recoverEdgesCommandBuffer) [[unlikely]] {
			
				// Display message
				cout << "Creating command buffers for the GPU failed" << endl;
				
				// Break
				break;
			}
			
			// Create commit options feedback handler
			CFTimeInterval gpuStartTime;
			CFTimeInterval gpuEndTime;
			bool gpuError;
			binary_semaphore commandQueueFinishedSemaphore(0);
			
			const auto commitOptionsFeedbackHandler = [&gpuStartTime, &gpuEndTime, &gpuError, &commandQueueFinishedSemaphore](const MTL4::CommitFeedback *commitFeedback) __attribute__((always_inline)) noexcept {
			
				// Update GPU start time, end time, and error
				gpuStartTime = commitFeedback->GPUStartTime();
				gpuEndTime = commitFeedback->GPUEndTime();
				gpuError = commitFeedback->error();
				
				// Set that the command queue has finished
				commandQueueFinishedSemaphore.release();
			};
			
			// Create block
			{
			
				// Start encoding commands into the command buffer
				trimEdgesCommandBuffer->beginCommandBuffer(trimEdgesCommandAllocator.get());
				
				// Check if creating command encoder for the command buffer failed
				MTL4::ComputeCommandEncoder *commandEncoder = trimEdgesCommandBuffer->computeCommandEncoder();
				if(!commandEncoder) [[unlikely]] {
				
					// Display message
					cout << "Creating command encoder for the command buffer failed" << endl;
					
					// Break
					break;
				}
				
				// Encode clearing the all buffers
				commandEncoder->fillBuffer(coarseBucketsBuffer.get(), NS::Range(0, coarseBucketsBuffer->length()), 0);
				commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
				commandEncoder->fillBuffer(trimEdgesParametersBuffer.get(), NS::Range(0, trimEdgesParametersBuffer->length()), 0);
				commandEncoder->fillBuffer(largestCoarseBucketSizeBuffer.get(), NS::Range(0, sizeof(MTL::DispatchThreadgroupsIndirectArguments().threadgroupsPerGrid[0])), 0);
				commandEncoder->fillBuffer(fineBucketsBuffer.get(), NS::Range(0, fineBucketsBuffer->length()), 0);
				commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, numberOfEdgesPerFineBucketBuffer->length()), 0);
				commandEncoder->fillBuffer(cpuBucketsBuffer.get(), NS::Range(0, cpuBucketsBuffer->length()), 0);
				commandEncoder->fillBuffer(numberOfEdgesPerCpuBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCpuBucketBuffer->length()), 0);
				commandEncoder->fillBuffer(solutionEdgesBuffer.get(), NS::Range(0, solutionEdgesBuffer->length()), 0);
				commandEncoder->fillBuffer(recoverEdgesParametersBuffer.get(), NS::Range(0, recoverEdgesParametersBuffer->length()), 0);
				
				// Finish encoding commands into the command buffer
				commandEncoder->endEncoding();
				trimEdgesCommandBuffer->endCommandBuffer();
				
				// Check if creating commit options failed
				MTL4::CommitOptions *commitOptions = MTL4::CommitOptions::alloc()->init();
				if(!commitOptions) [[unlikely]] {
				
					// Display message
					cout << "Creating commit options failed" << endl;
					
					// Break
					break;
				}
				
				// Add commit options to autorelease pool
				commitOptions->autorelease();
				
				// Configure commit options
				commitOptions->addFeedbackHandler(commitOptionsFeedbackHandler);
				
				// Run the command buffer on the GPU
				commandQueue->commit((const MTL4::CommandBuffer *[]){trimEdgesCommandBuffer.get()}, 1, commitOptions);
				
				// Check if ensuring memory is fully allocated failed
				commandQueueFinishedSemaphore.acquire();
				if(gpuError) [[unlikely]] {
				
					// Display message
					cout << "Ensuring memory is fully allocated failed" << endl;
					
					// Break
					break;
				}
			}
			
		// Otherwise
		#else
		
			// Create block
			unique_ptr<remove_pointer_t<cl_context>, decltype(&clReleaseContext)> gpuContext(nullptr, clReleaseContext);
			cl_device_id gpu;
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> coarseBucketSortEdgesKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> updateLargestInitialCoarseBucketSizeKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> fineBucketSortInitialEdgesKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> trimInitialEdgesKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> updateLargestIntermediateCoarseBucketSizeKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> fineBucketSortIntermediateEdgesKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> trimIntermediateEdgesKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> updateLargestFinalCoarseBucketSizeKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> fineBucketSortFinalEdgesKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> trimFinalEdgesKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> trimFinalEdgesAndTransferEdgesKernel(nullptr, clReleaseKernel);
			unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)> recoverEdgesKernel(nullptr, clReleaseKernel);
			{
			
				// Set total GPU memory allocated
				const size_t totalGpuMemoryAllocated = GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_COARSE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET + GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) + sizeof(TrimEdgesParameters) + sizeof(uint32_t) + GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) + GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t) + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET + CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t) + SOLUTION_SIZE * sizeof(uint32_t) + recoverEdgesParametersUsedSize;
				
				// Set max GPU work group memory size
				const size_t maxGpuWorkGroupMemorySize = max(max(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t) + max(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t) + GPU_BITMAP_SIZE, max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t) + max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t));
				
				// Display message
				cout << "Allocating " << (static_cast<double>(totalGpuMemoryAllocated) / BYTES_IN_A_GIGABYTE) << " GB of GPU memory and using at most " << (static_cast<double>(maxGpuWorkGroupMemorySize) / BYTES_IN_A_KILOBYTE) << " KB of GPU local memory" << endl;
				
				// Check if getting number of OpenCL platforms failed
				cl_uint numberOfOpenClPlatforms;
				if(clGetPlatformIDs(0, nullptr, &numberOfOpenClPlatforms) != CL_SUCCESS) [[unlikely]] {
				
					// Display message
					cout << "Getting number of OpenCL platforms failed" << endl;
					
					// Break
					break;
				}
				
				// Check if no OpenCL platforms exist
				if(!numberOfOpenClPlatforms) [[unlikely]] {
				
					// Display message
					cout << "No OpenCL platforms exist" << endl;
					
					// Break
					break;
				}
				
				// Check if getting OpenCL platforms failed
				cl_platform_id openClPlatforms[numberOfOpenClPlatforms];
				if(clGetPlatformIDs(numberOfOpenClPlatforms, openClPlatforms, nullptr) != CL_SUCCESS) [[unlikely]] {
				
					// Display message
					cout << "Getting OpenCL platforms failed" << endl;
					
					// Break
					break;
				}
				
				// Go through all OpenCL platforms while a GPU context hasn't been created
				bool applicableGpuExists = false;
				__builtin_assume(numberOfOpenClPlatforms > 0);
				for(cl_uint i = 0; i < numberOfOpenClPlatforms && !gpuContext; ++i) [[likely]] {
				
					// Check if getting OpenCL platform's number of GPUs was successful and GPUs exist
					cl_uint numberOfGpus;
					if(clGetDeviceIDs(openClPlatforms[i], CL_DEVICE_TYPE_GPU, 0, nullptr, &numberOfGpus) == CL_SUCCESS && numberOfGpus) [[likely]] {
					
						// Check if getting OpenCL platform's GPUs was successful
						cl_device_id gpus[numberOfGpus];
						if(clGetDeviceIDs(openClPlatforms[i], CL_DEVICE_TYPE_GPU, numberOfGpus, gpus, nullptr) == CL_SUCCESS) [[likely]] {
						
							// Go through all of the GPUs
							__builtin_assume(numberOfGpus > 0);
							for(cl_uint j = 0; j < numberOfGpus; ++j) [[likely]] {
							
								// Check if current GPU is available, is little endian, has enough memory, has enough work group memory, and has a profile, OpenCL version, and name
								cl_bool isAvailable;
								cl_bool isLittleEndian;
								cl_ulong memorySize;
								cl_ulong workGroupMemorySize;
								size_t profileSize;
								size_t openClVersionSize;
								size_t nameSize;
								if(clGetDeviceInfo(gpus[j], CL_DEVICE_AVAILABLE, sizeof(isAvailable), &isAvailable, nullptr) == CL_SUCCESS && isAvailable == CL_TRUE && clGetDeviceInfo(gpus[j], CL_DEVICE_ENDIAN_LITTLE, sizeof(isLittleEndian), &isLittleEndian, nullptr) == CL_SUCCESS && isLittleEndian == CL_TRUE && clGetDeviceInfo(gpus[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(memorySize), &memorySize, nullptr) == CL_SUCCESS && memorySize >= totalGpuMemoryAllocated && clGetDeviceInfo(gpus[j], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(workGroupMemorySize), &workGroupMemorySize, nullptr) == CL_SUCCESS && workGroupMemorySize >= maxGpuWorkGroupMemorySize && clGetDeviceInfo(gpus[j], CL_DEVICE_PROFILE, 0, nullptr, &profileSize) == CL_SUCCESS && profileSize && clGetDeviceInfo(gpus[j], CL_DEVICE_OPENCL_C_VERSION, 0, nullptr, &openClVersionSize) == CL_SUCCESS && openClVersionSize && clGetDeviceInfo(gpus[j], CL_DEVICE_NAME, 0, nullptr, &nameSize) == CL_SUCCESS && nameSize) [[likely]] {
								
									// Check if current GPU supports full profile, its OpenCL version is compatible, and getting its name was successful
									char profile[profileSize];
									char openClVersion[openClVersionSize];
									char name[nameSize];
									if(clGetDeviceInfo(gpus[j], CL_DEVICE_PROFILE, profileSize, profile, nullptr) == CL_SUCCESS && !__builtin_strcmp(profile, "FULL_PROFILE") && clGetDeviceInfo(gpus[j], CL_DEVICE_OPENCL_C_VERSION, openClVersionSize, openClVersion, nullptr) == CL_SUCCESS && !__builtin_strncmp(openClVersion, "OpenCL C ", sizeof("OpenCL C ") - sizeof('\0')) && strtod(&openClVersion[sizeof("OpenCL C ") - sizeof('\0')], nullptr) >= 1.2 && clGetDeviceInfo(gpus[j], CL_DEVICE_NAME, nameSize, name, nullptr) == CL_SUCCESS) [[likely]] {
									
										// Set applicable GPU exists to true
										applicableGpuExists = true;
										
										// Check if creating a context for the current GPU was successful
										gpuContext = unique_ptr<remove_pointer_t<cl_context>, decltype(&clReleaseContext)>(clCreateContext((const cl_context_properties[]){CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(openClPlatforms[i]), 0}, 1, &gpus[j], nullptr, nullptr, nullptr), clReleaseContext);
										if(gpuContext) [[likely]] {
										
											// Set GPU to the current GPU
											gpu = gpus[j];
											
											// Display message
											cout << "Using the " << name << " GPU" << endl;
											
											// Break
											break;
										}
									}
								}
							}
						}
					}
				}
				
				// Check if no applicable GPUs exist
				if(!applicableGpuExists) [[unlikely]] {
				
					// Display message
					cout << "No applicable GPUs exist" << endl;
					
					// Break
					break;
				}
				
				// Check if creating a context for an OpenCL platform's GPU failed
				if(!gpuContext) [[unlikely]] {
				
					// Display message
					cout << "Creating a context for an OpenCL platform's GPU failed" << endl;
					
					// Break
					break;
				}
				
				// Check if embedding GPU code
				#if EMBED_GPU_CODE
				
					// Get source
					constexpr const char *source = (
						#include "gpu.cl"
					);
					
					// Get source size
					const size_t sourceSize = __builtin_strlen(source);
					
				// Otherwise
				#else
				
					// Check if opening source file failed
					const unique_ptr<FILE, decltype(&fclose)> sourceFile(fopen("gpu.cl", "rb"), fclose);
					if(!sourceFile) [[unlikely]] {
					
						// Display message
						cout << "Opening gpu.cl failed" << endl;
						
						// Break
						break;
					}
					
					// Check if getting source file's size failed
					long sourceFileSize;
					if(fseek(sourceFile.get(), 0, SEEK_END) || (sourceFileSize = ftell(sourceFile.get())) == -1 || fseek(sourceFile.get(), 0, SEEK_SET) || static_cast<unsigned long>(sourceFileSize) > SIZE_MAX) [[unlikely]] {
					
						// Display message
						cout << "Getting gpu.cl's size failed" << endl;
						
						// Break
						break;
					}
					
					// Check if reading source file failed
					char sourceFileContents[sourceFileSize];
					if(fread(sourceFileContents, sizeof(char), sourceFileSize, sourceFile.get()) != sizeof(sourceFileContents)) [[unlikely]] {
					
						// Display message
						cout << "Reading gpu.cl failed" << endl;
						
						// Break
						break;
					}
					
					// Set source
					const char *source = &sourceFileContents[sizeof("R\"(") - sizeof('\0')];
					
					// Set source size
					const size_t sourceSize = sourceFileSize - (sizeof("R\"()\"\n") - sizeof('\0'));
				#endif
				
				// Check if creating program for the GPU failed
				const unique_ptr<remove_pointer_t<cl_program>, decltype(&clReleaseProgram)> program(clCreateProgramWithSource(gpuContext.get(), 1, const_cast<const char **>(&source), &sourceSize, nullptr), clReleaseProgram);
				if(!program) [[unlikely]] {
				
					// Display message
					cout << "Creating program for the GPU failed" << endl;
					
					// Break
					break;
				}
				
				// Check if building program for the GPU failed
				static constexpr const string_view programParameters[] = {"-cl-std=CL1.2 -w "
				
					// Edge bits
					"-D EDGE_BITS=" TO_STRING(EDGE_BITS) " "
					
					// Solution size
					"-D SOLUTION_SIZE=" TO_STRING(SOLUTION_SIZE) " "
					
					// GPU trimming use max RAM
					"-D GPU_TRIMMING_USE_MAX_RAM=" TO_STRING(GPU_TRIMMING_USE_MAX_RAM) " "
					
					// GPU trimming use more RAM
					"-D GPU_TRIMMING_USE_MORE_RAM=" TO_STRING(GPU_TRIMMING_USE_MORE_RAM) " "
					
					// GPU trimming use less RAM
					"-D GPU_TRIMMING_USE_LESS_RAM=" TO_STRING(GPU_TRIMMING_USE_LESS_RAM) " "
					
					// GPU trimming use min RAM
					"-D GPU_TRIMMING_USE_MIN_RAM=" TO_STRING(GPU_TRIMMING_USE_MIN_RAM) " "
					
					// GPU number of most significant bits used for coarse bucket sorting
					"-D GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING=" TO_STRING(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING) " "
					
					// GPU number of most significant bits used for initial fine bucket sorting
					"-D GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING=" TO_STRING(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_INITIAL_FINE_BUCKET_SORTING) " "
					
					// GPU number of most significant bits used for fine bucket sorting
					"-D GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING=" TO_STRING(GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING) " "
					
					// GPU fine bucket sort initial edges kernel number of work items per work group
					"-D GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=" TO_STRING(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) " "
					
					// GPU trim initial edges kernel number of work items per work group
					"-D GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=" TO_STRING(GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) " "
					
					// GPU fine bucket sort intermediate edges kernel number of work items per work group
					"-D GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=" TO_STRING(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) " "
					
					// GPU trim intermediate edges kernel number of work items per work group
					"-D GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=" TO_STRING(GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) " "
					
					// GPU fine bucket sort final edges kernel number of work items per work group
					"-D GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=" TO_STRING(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) " "
					
					// GPU trim final edges kernel number of work items per work group
					"-D GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=" TO_STRING(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) " "
					
					// CPU number of most significant bits used for coarse bucket sorting
					"-D CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING=" TO_STRING(CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING) " "
					
					// GPU trim final edges and transfer edges kernel number of work items per work group
					"-D GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP=" TO_STRING(GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) " "
					
					// GPU recover edges kernel number of edges per work item
					"-D GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM=" TO_STRING(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM) " "
					
					// GPU recover edges kernel number of recovered edge candidates per work item
					"-D GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM=" TO_STRING(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM) " "
					
					// GPU max number of edges per coarse bucket
					"-D GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET=", toString<GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET>::value, " "
					
					// GPU max number of edges per initial fine bucket
					"-D GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET=", toString<GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET>::value, " "
					
					// GPU max number of edges per fine bucket
					"-D GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET=", toString<GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET>::value, " "
					
					// CPU max number of edges per coarse bucket
					"-D CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET=", toString<CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET>::value, " "
					
					// GPU number of recovering edges
					"-D GPU_NUMBER_OF_RECOVERING_EDGES=", toString<GPU_NUMBER_OF_RECOVERING_EDGES>::value, " "
				};
				
				if(clBuildProgram(program.get(), 1, &gpu, concatenateStrings<programParameters, sizeof(programParameters) / sizeof(programParameters[0])>::value, nullptr, nullptr) != CL_SUCCESS) [[unlikely]] {
				
					// Display message
					cout << "Building program for the GPU failed" << endl;
					
					// Check if getting log size for building the program was successful and log exists
					size_t logSize;
					if(clGetProgramBuildInfo(program.get(), gpu, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize) == CL_SUCCESS && logSize) [[likely]] {
					
						// Check if getting log for building the program was successful
						char log[logSize];
						if(clGetProgramBuildInfo(program.get(), gpu, CL_PROGRAM_BUILD_LOG, logSize, log, nullptr) == CL_SUCCESS) [[likely]] {
						
							// Display log
							cout << log << endl;
						}
					}
					
					// Break
					break;
				}
				
				// Check if creating kernels for the GPU failed
				coarseBucketSortEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "coarseBucketSortEdges", nullptr), clReleaseKernel);
				updateLargestInitialCoarseBucketSizeKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "updateLargestInitialCoarseBucketSize", nullptr), clReleaseKernel);
				fineBucketSortInitialEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "fineBucketSortInitialEdges", nullptr), clReleaseKernel);
				trimInitialEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "trimInitialEdges", nullptr), clReleaseKernel);
				updateLargestIntermediateCoarseBucketSizeKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "updateLargestIntermediateCoarseBucketSize", nullptr), clReleaseKernel);
				fineBucketSortIntermediateEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "fineBucketSortIntermediateEdges", nullptr), clReleaseKernel);
				trimIntermediateEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "trimIntermediateEdges", nullptr), clReleaseKernel);
				updateLargestFinalCoarseBucketSizeKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "updateLargestFinalCoarseBucketSize", nullptr), clReleaseKernel);
				fineBucketSortFinalEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "fineBucketSortFinalEdges", nullptr), clReleaseKernel);
				trimFinalEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "trimFinalEdges", nullptr), clReleaseKernel);
				trimFinalEdgesAndTransferEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "trimFinalEdgesAndTransferEdges", nullptr), clReleaseKernel);
				recoverEdgesKernel = unique_ptr<remove_pointer_t<cl_kernel>, decltype(&clReleaseKernel)>(clCreateKernel(program.get(), "recoverEdges", nullptr), clReleaseKernel);
				if(!coarseBucketSortEdgesKernel || !updateLargestInitialCoarseBucketSizeKernel || !fineBucketSortInitialEdgesKernel || !trimInitialEdgesKernel || !updateLargestIntermediateCoarseBucketSizeKernel || !fineBucketSortIntermediateEdgesKernel || !trimIntermediateEdgesKernel || !updateLargestFinalCoarseBucketSizeKernel || !fineBucketSortFinalEdgesKernel || !trimFinalEdgesKernel || !trimFinalEdgesAndTransferEdgesKernel || !recoverEdgesKernel) [[unlikely]] {
				
					// Display message
					cout << "Creating kernels for the GPU failed" << endl;
					
					// Break
					break;
				}
			}
			
			// Throw error if GPU data type's sizes or alignments are invalid
			static_assert(sizeof(cl_uint) == sizeof(uint32_t) && alignof(cl_uint) == alignof(uint32_t) && sizeof(cl_ulong) == sizeof(uint64_t) && alignof(cl_ulong) == alignof(uint64_t) && sizeof(TrimEdgesParameters) == sizeof(cl_ulong4) && sizeof(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS)))) == sizeof(cl_ulong4) && alignof(TrimEdgesParameters) == alignof(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS)))) && alignof(uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS)))) == alignof(cl_ulong4) && sizeof(cl_uint2) == sizeof(uint32_t[2]) && alignof(cl_uint2) == alignof(uint64_t) && sizeof(cl_uint2) == sizeof(cl_ulong) && alignof(cl_uint2) == alignof(cl_ulong), "GPU data type's sizes or alignments are invalid");
			
			// Check if allocating memory on the GPU failed
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> coarseBucketsBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_COARSE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> numberOfEdgesPerCoarseBucketBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> trimEdgesParametersBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(TrimEdgesParameters), nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> largestCoarseBucketSizeBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, sizeof(uint32_t), nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> fineBucketsBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET), nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> numberOfEdgesPerFineBucketBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t), nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> cpuBucketsBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> numberOfEdgesPerCpuBucketBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> solutionEdgesBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, SOLUTION_SIZE * sizeof(uint32_t), nullptr, nullptr), clReleaseMemObject);
			const unique_ptr<remove_pointer_t<cl_mem>, decltype(&clReleaseMemObject)> recoverEdgesParametersBuffer(clCreateBuffer(gpuContext.get(), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, recoverEdgesParametersUsedSize, nullptr, nullptr), clReleaseMemObject);
			if(!coarseBucketsBuffer || !numberOfEdgesPerCoarseBucketBuffer || !trimEdgesParametersBuffer || !largestCoarseBucketSizeBuffer || !fineBucketsBuffer || !numberOfEdgesPerFineBucketBuffer || !cpuBucketsBuffer || !numberOfEdgesPerCpuBucketBuffer || !solutionEdgesBuffer || !recoverEdgesParametersBuffer) [[unlikely]] {
			
				// Display message
				cout << "Allocating memory on the GPU failed" << endl;
				
				// Break
				break;
			}
			
			// Check if setting GPU program's arguments failed
			if(
				clSetKernelArg(coarseBucketSortEdgesKernel.get(), 0, sizeof(coarseBucketsBuffer.get()), &static_cast<const cl_mem &>(coarseBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(coarseBucketSortEdgesKernel.get(), 1, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(coarseBucketSortEdgesKernel.get(), 2, sizeof(trimEdgesParametersBuffer.get()), &static_cast<const cl_mem &>(trimEdgesParametersBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(updateLargestInitialCoarseBucketSizeKernel.get(), 0, sizeof(largestCoarseBucketSizeBuffer.get()), &static_cast<const cl_mem &>(largestCoarseBucketSizeBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(updateLargestInitialCoarseBucketSizeKernel.get(), 1, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(fineBucketSortInitialEdgesKernel.get(), 0, sizeof(fineBucketsBuffer.get()), &static_cast<const cl_mem &>(fineBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortInitialEdgesKernel.get(), 1, sizeof(numberOfEdgesPerFineBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerFineBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortInitialEdgesKernel.get(), 2, sizeof(coarseBucketsBuffer.get()), &static_cast<const cl_mem &>(coarseBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortInitialEdgesKernel.get(), 3, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortInitialEdgesKernel.get(), 4, sizeof(largestCoarseBucketSizeBuffer.get()), &static_cast<const cl_mem &>(largestCoarseBucketSizeBuffer.get())) != CL_SUCCESS ||
				#if !GPU_TRIMMING_USE_MAX_RAM
					clSetKernelArg(fineBucketSortInitialEdgesKernel.get(), 5, sizeof(trimEdgesParametersBuffer.get()), &static_cast<const cl_mem &>(trimEdgesParametersBuffer.get())) != CL_SUCCESS ||
				#endif
				
				clSetKernelArg(trimInitialEdgesKernel.get(), 0, sizeof(coarseBucketsBuffer.get()), &static_cast<const cl_mem &>(coarseBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimInitialEdgesKernel.get(), 1, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimInitialEdgesKernel.get(), 2, sizeof(fineBucketsBuffer.get()), &static_cast<const cl_mem &>(fineBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimInitialEdgesKernel.get(), 3, sizeof(numberOfEdgesPerFineBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerFineBucketBuffer.get())) != CL_SUCCESS ||
				#if !GPU_TRIMMING_USE_MAX_RAM && !GPU_TRIMMING_USE_MORE_RAM
					clSetKernelArg(trimInitialEdgesKernel.get(), 4, sizeof(trimEdgesParametersBuffer.get()), &static_cast<const cl_mem &>(trimEdgesParametersBuffer.get())) != CL_SUCCESS ||
				#endif
				
				clSetKernelArg(updateLargestIntermediateCoarseBucketSizeKernel.get(), 0, sizeof(largestCoarseBucketSizeBuffer.get()), &static_cast<const cl_mem &>(largestCoarseBucketSizeBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(updateLargestIntermediateCoarseBucketSizeKernel.get(), 1, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(fineBucketSortIntermediateEdgesKernel.get(), 0, sizeof(fineBucketsBuffer.get()), &static_cast<const cl_mem &>(fineBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortIntermediateEdgesKernel.get(), 1, sizeof(numberOfEdgesPerFineBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerFineBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortIntermediateEdgesKernel.get(), 2, sizeof(coarseBucketsBuffer.get()), &static_cast<const cl_mem &>(coarseBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortIntermediateEdgesKernel.get(), 3, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortIntermediateEdgesKernel.get(), 4, sizeof(trimEdgesParametersBuffer.get()), &static_cast<const cl_mem &>(trimEdgesParametersBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortIntermediateEdgesKernel.get(), 5, sizeof(largestCoarseBucketSizeBuffer.get()), &static_cast<const cl_mem &>(largestCoarseBucketSizeBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(trimIntermediateEdgesKernel.get(), 0, sizeof(coarseBucketsBuffer.get()), &static_cast<const cl_mem &>(coarseBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimIntermediateEdgesKernel.get(), 1, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimIntermediateEdgesKernel.get(), 2, sizeof(fineBucketsBuffer.get()), &static_cast<const cl_mem &>(fineBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimIntermediateEdgesKernel.get(), 3, sizeof(numberOfEdgesPerFineBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerFineBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimIntermediateEdgesKernel.get(), 4, sizeof(trimEdgesParametersBuffer.get()), &static_cast<const cl_mem &>(trimEdgesParametersBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(updateLargestFinalCoarseBucketSizeKernel.get(), 0, sizeof(largestCoarseBucketSizeBuffer.get()), &static_cast<const cl_mem &>(largestCoarseBucketSizeBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(updateLargestFinalCoarseBucketSizeKernel.get(), 1, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(fineBucketSortFinalEdgesKernel.get(), 0, sizeof(fineBucketsBuffer.get()), &static_cast<const cl_mem &>(fineBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortFinalEdgesKernel.get(), 1, sizeof(numberOfEdgesPerFineBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerFineBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortFinalEdgesKernel.get(), 2, sizeof(coarseBucketsBuffer.get()), &static_cast<const cl_mem &>(coarseBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortFinalEdgesKernel.get(), 3, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(fineBucketSortFinalEdgesKernel.get(), 4, sizeof(largestCoarseBucketSizeBuffer.get()), &static_cast<const cl_mem &>(largestCoarseBucketSizeBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(trimFinalEdgesKernel.get(), 0, sizeof(coarseBucketsBuffer.get()), &static_cast<const cl_mem &>(coarseBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimFinalEdgesKernel.get(), 1, sizeof(numberOfEdgesPerCoarseBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCoarseBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimFinalEdgesKernel.get(), 2, sizeof(fineBucketsBuffer.get()), &static_cast<const cl_mem &>(fineBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimFinalEdgesKernel.get(), 3, sizeof(numberOfEdgesPerFineBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerFineBucketBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(trimFinalEdgesAndTransferEdgesKernel.get(), 0, sizeof(cpuBucketsBuffer.get()), &static_cast<const cl_mem &>(cpuBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimFinalEdgesAndTransferEdgesKernel.get(), 1, sizeof(numberOfEdgesPerCpuBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerCpuBucketBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimFinalEdgesAndTransferEdgesKernel.get(), 2, sizeof(fineBucketsBuffer.get()), &static_cast<const cl_mem &>(fineBucketsBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(trimFinalEdgesAndTransferEdgesKernel.get(), 3, sizeof(numberOfEdgesPerFineBucketBuffer.get()), &static_cast<const cl_mem &>(numberOfEdgesPerFineBucketBuffer.get())) != CL_SUCCESS ||
				
				clSetKernelArg(recoverEdgesKernel.get(), 0, sizeof(solutionEdgesBuffer.get()), &static_cast<const cl_mem &>(solutionEdgesBuffer.get())) != CL_SUCCESS ||
				clSetKernelArg(recoverEdgesKernel.get(), 1, sizeof(recoverEdgesParametersBuffer.get()), &static_cast<const cl_mem &>(recoverEdgesParametersBuffer.get())) != CL_SUCCESS
				
			) [[unlikely]] {
			
				// Display message
				cout << "Setting GPU program's arguments failed" << endl;
				
				// Break
				break;
			}
			
			// Check if creating command queue for the GPU failed
			const unique_ptr<remove_pointer_t<cl_command_queue>, decltype(&clReleaseCommandQueue)> commandQueue(clCreateCommandQueue(gpuContext.get(), gpu, CL_QUEUE_PROFILING_ENABLE, nullptr), clReleaseCommandQueue);
			if(!commandQueue) [[unlikely]] {
			
				// Display message
				cout << "Creating command queue for the GPU failed" << endl;
				
				// Break
				break;
			}
			
			// Check if clearing the all buffers failed
			if(clEnqueueFillBuffer(commandQueue.get(), coarseBucketsBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_COARSE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), trimEdgesParametersBuffer.get(), (const uint64_t[]){0}, sizeof(uint64_t), 0, sizeof(TrimEdgesParameters), 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), largestCoarseBucketSizeBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), fineBucketsBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_INITIAL_FINE_BUCKET, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * GPU_FINE_BUCKET_ITEM_SIZE * GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET), 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * max(GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), cpuBucketsBuffer.get(), (const uint64_t[]){0}, sizeof(uint64_t), 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCpuBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), solutionEdgesBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, SOLUTION_SIZE * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
				clEnqueueFillBuffer(commandQueue.get(), recoverEdgesParametersBuffer.get(), (const uint8_t[]){0}, sizeof(uint8_t), 0, recoverEdgesParametersUsedSize, 0, nullptr, nullptr) != CL_SUCCESS ||
				clFlush(commandQueue.get()) != CL_SUCCESS ||
				clFinish(commandQueue.get()) != CL_SUCCESS
			) [[unlikely]] {
			
				// Display message
				cout << "Ensuring memory is fully allocated failed" << endl;
				
				// Break
				break;
			}
		#endif
		
		// Display message
		cout << "Finished acquiring GPU and allocating GPU memory" << endl;
		
		// Loop while not closing and an error hasn't occurred
		do [[likely]] {
		
			// Check if using an Apple device
			#ifdef __APPLE__
			
				// Free objects in autorelease pool
				autoreleasePool.reset();
			#endif
			
			// Create job values
			uint8_t jobHeader[HEADER_SIZE_EXCLUDING_NONCE] = {};
			NonceType jobNonce[2] = {STARTING_NONCE, STARTING_NONCE + 1};
			
			// Check if starting header is set
			#if STARTING_HEADER_SIZE != 0
			
				// Set job header to the starting header
				__builtin_memcpy(jobHeader, TO_STRING(STARTING_HEADER), STARTING_HEADER_SIZE);
			#endif
			
			// Check if mining to a stratum server
			#if MINE_TO_A_STRATUM_SERVER
			
				// Create job values
				uint64_t jobHeight[2];
				uint64_t jobId[2];
				
				// Check if using Windows
				#ifdef _WIN32
				
					// Create socket descriptor
					SOCKET socketDescriptor = INVALID_SOCKET;
					
				// Otherwise
				#else
				
					// Create socket descriptor
					int socketDescriptor = -1;
				#endif
				
				// Create block
				{
				
					// Display message
					cout << "Connecting to the stratum server" << endl;
					
					// Check if getting address info for the stratum server failed
					const addrinfo addressInfoHints = {
					
						// Port provided
						.ai_flags = AI_NUMERICSERV,
						
						// IPv4 or IPv6
						.ai_family = AF_UNSPEC,
						
						// TCP
						.ai_socktype = SOCK_STREAM,
					};
					
					addrinfo *addressInfo;
					if(getaddrinfo(stratumServerAddress, stratumServerPort, &addressInfoHints, &addressInfo)) [[unlikely]] {
					
						// Display message
						cout << "Getting address info for the stratum server failed" << endl;
						
						// Check if previously connected to the stratum server
						if(returnStatus == EXIT_SUCCESS) [[likely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Wait before trying again
								Sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND);
								
							// Otherwise
							#else
							
								// Wait before trying again
								sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS);
							#endif
						}
						
						// Continue
						continue;
					}
					
					// Automatically free address info when done
					const unique_ptr<addrinfo, decltype(&freeaddrinfo)> addressInfoUniquePointer(addressInfo, freeaddrinfo);
					
					// Go through all addresses for the stratum server
					for(const addrinfo *address = addressInfo; address; address = address->ai_next) [[likely]] {
					
						// Create socket descriptor
						socketDescriptor = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
						
						// Check if using Windows
						#ifdef _WIN32
						
							// Check if creating socket descriptor was successful
							if(socketDescriptor != INVALID_SOCKET) [[likely]] {
							
								// Set read timeout
								const DWORD readTimeout = STRATUM_SERVER_READ_TIMEOUT_SECONDS * MILLISECONDS_IN_A_SECOND;
								
								// Set write timeout
								const DWORD writeTimeout = STRATUM_SERVER_WRITE_TIMEOUT_SECONDS * MILLISECONDS_IN_A_SECOND;
								
								// Set enable TCP no delay
								const DWORD enableTcpNoDelay = 1;
								
						// Otherwise
						#else
						
							// Check if creating socket descriptor was successful
							if(socketDescriptor != -1) [[likely]] {
							
								// Set read timeout
								const timeval readTimeout = {
								
									// Seconds
									.tv_sec = STRATUM_SERVER_READ_TIMEOUT_SECONDS
								};
								
								// Set write timeout
								const timeval writeTimeout = {
								
									// Seconds
									.tv_sec = STRATUM_SERVER_WRITE_TIMEOUT_SECONDS
								};
								
								// Set enable TCP no delay
								const int enableTcpNoDelay = 1;
						#endif
						
							// Check if configuring the socket descriptor and connecting to the address was successful
							if(!setsockopt(socketDescriptor, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&readTimeout), sizeof(readTimeout)) && !setsockopt(socketDescriptor, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&writeTimeout), sizeof(writeTimeout)) && !setsockopt(socketDescriptor, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&enableTcpNoDelay), sizeof(enableTcpNoDelay)) && !connect(socketDescriptor, address->ai_addr, address->ai_addrlen)) [[likely]] {
							
								// Break
								break;
							}
							
							// Otherwise
							else [[unlikely]] {
							
								// Check if using Windows
								#ifdef _WIN32
								
									// Close socket descriptor
									closesocket(socketDescriptor);
									
									// Reset socket descriptor
									socketDescriptor = INVALID_SOCKET;
									
								// Otherwise
								#else
								
									// Close socket descriptor
									close(socketDescriptor);
									
									// Reset socket descriptor
									socketDescriptor = -1;
								#endif
							}
						}
					}
					
					// Check if using Windows
					#ifdef _WIN32
					
						// Check if connecting to the stratum server failed
						if(socketDescriptor == INVALID_SOCKET) [[unlikely]] {
						
					// Otherwise
					#else
					
						// Check if connecting to the stratum server failed
						if(socketDescriptor == -1) [[unlikely]] {
					#endif
					
						// Display message
						cout << "Connecting to the stratum server failed" << endl;
						
						// Check if previously connected to the stratum server
						if(returnStatus == EXIT_SUCCESS) [[likely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Wait before trying again
								Sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND);
								
							// Otherwise
							#else
							
								// Wait before trying again
								sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS);
							#endif
						}
						
						// Continue
						continue;
					}
					
					// Display message
					cout << "Connected to the stratum server" << endl;
				}
				
				// Automatically free socket descriptor when done
				const unique_ptr<decltype(socketDescriptor), void(*)(const decltype(socketDescriptor) *)> socketDescriptorUniquePointer(&socketDescriptor, [](const decltype(socketDescriptor) *socketDescriptorPointer) __attribute__((always_inline)) noexcept {
				
					// Check if using Windows
					#ifdef _WIN32
					
						// Shutdown socket descriptor receive and send
						__builtin_assume_dereferenceable(socketDescriptorPointer, sizeof(*socketDescriptorPointer));
						shutdown(*socketDescriptorPointer, SD_BOTH);
						
						// Close socket descriptor
						__builtin_assume_dereferenceable(socketDescriptorPointer, sizeof(*socketDescriptorPointer));
						closesocket(*socketDescriptorPointer);
						
					// Otherwise
					#else
					
						// Shutdown socket descriptor receive and send
						__builtin_assume_dereferenceable(socketDescriptorPointer, sizeof(*socketDescriptorPointer));
						shutdown(*socketDescriptorPointer, SHUT_RDWR);
						
						// Close socket descriptor
						__builtin_assume_dereferenceable(socketDescriptorPointer, sizeof(*socketDescriptorPointer));
						close(*socketDescriptorPointer);
					#endif
					
					// Display message
					cout << "Disconnected from the stratum server" << endl;
				});
				
				// Create block
				char receiveBuffer[STRATUM_SERVER_RECEIVE_BUFFER_SIZE_KILOBYTES * BYTES_IN_A_KILOBYTE];
				size_t totalBytesReceived = 0;
				{
				
					// Display message
					cout << "Logging into the stratum server" << endl;
					
					// Check if creating login request failed
					char loginRequest[sizeof("{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"login\",\"params\":{\"login\":\"") - sizeof('\0') + (__builtin_expect(stratumServerUsername != nullptr, true) ? __builtin_strlen(stratumServerUsername) : 0) + sizeof("\",\"pass\":\"\",\"agent\":\"" TO_STRING(NAME) "/v" TO_STRING(VERSION) "\"}}\n") - sizeof('\0')];
					
					// Append start of username parameter to login request
					__builtin_memcpy_inline(loginRequest, "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"login\",\"params\":{\"login\":\"", sizeof("{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"login\",\"params\":{\"login\":\"") - sizeof('\0'));
					
					// Check if stratum server username exists
					if(stratumServerUsername) [[likely]] {
					
						// Append username to login request
						__builtin_memcpy(&loginRequest[sizeof("{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"login\",\"params\":{\"login\":\"") - sizeof('\0')], stratumServerUsername, __builtin_strlen(stratumServerUsername));
					}
					
					// Append password and agent to login request
					__builtin_memcpy_inline(&loginRequest[sizeof("{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"login\",\"params\":{\"login\":\"") - sizeof('\0') + (__builtin_expect(stratumServerUsername != nullptr, true) ? __builtin_strlen(stratumServerUsername) : 0)], "\",\"pass\":\"\",\"agent\":\"" TO_STRING(NAME) "/v" TO_STRING(VERSION) "\"}}\n", sizeof("\",\"pass\":\"\",\"agent\":\"" TO_STRING(NAME) "/v" TO_STRING(VERSION) "\"}}\n") - sizeof('\0'));
					
					// Loop until full message is sent
					size_t totalBytesSent = 0;
					do [[unlikely]] {
					
						// Check if using Windows
						#ifdef _WIN32
						
							// Send data to the stratum server
							const int bytesSent = send(socketDescriptor, &loginRequest[totalBytesSent], sizeof(loginRequest) - totalBytesSent, 0);
							
						// Otherwise
						#else
						
							// Send data to the stratum server
							const ssize_t bytesSent = send(socketDescriptor, &loginRequest[totalBytesSent], sizeof(loginRequest) - totalBytesSent, MSG_NOSIGNAL);
						#endif
						
						// Check if sending data to the stratum server failed
						if(bytesSent <= 0) [[unlikely]] {
						
							// Break
							break;
						}
						
						// Update total bytes sent
						totalBytesSent += bytesSent;
						
					} while(totalBytesSent != sizeof(loginRequest));
					
					// Check if sending data to the stratum server failed
					if(totalBytesSent != sizeof(loginRequest)) [[unlikely]] {
					
						// Display message
						cout << "Sending data to the stratum server failed" << endl;
						
						// Check if previously connected to the stratum server
						if(returnStatus == EXIT_SUCCESS) [[likely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Wait before trying again
								Sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND);
								
							// Otherwise
							#else
							
								// Wait before trying again
								sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS);
							#endif
						}
						
						// Continue
						continue;
					}
					
					// Check if displaying stratum server messages
					#if DISPLAY_STRATUM_SERVER_MESSAGES
					
						// Display message
						cout << "Sent: " << string_view(loginRequest, sizeof(loginRequest));
					#endif
					
					// Loop until full message is received
					for(bool fullMessageReceived = false; !fullMessageReceived;) [[unlikely]] {
					
						// Check if receiving data from the stratum server failed
						const decltype(function(recv))::result_type bytesReceived = recv(socketDescriptor, &receiveBuffer[totalBytesReceived], sizeof(receiveBuffer) - totalBytesReceived, 0);
						if(bytesReceived <= 0) [[unlikely]] {
						
							// Set that receiving data from the stratum server failed
							totalBytesReceived = 0;
							
							// Break
							break;
						}
						
						// Set full message received to if the received data contains the end of a message
						fullMessageReceived = __builtin_memchr(&receiveBuffer[totalBytesReceived], '\n', bytesReceived);
						
						// Update total bytes received
						totalBytesReceived += bytesReceived;
						
						// Check if receive buffer is full and full message hasn't been received
						if(totalBytesReceived == sizeof(receiveBuffer) && !fullMessageReceived) [[unlikely]] {
						
							// Set that receiving data from the stratum server failed
							totalBytesReceived = 0;
							
							// Break
							break;
						}
					};
					
					// Check if receiving data from the stratum server failed
					if(!totalBytesReceived) [[unlikely]] {
					
						// Display message
						cout << "Receiving data from the stratum server failed" << endl;
						
						// Check if previously connected to the stratum server
						if(returnStatus == EXIT_SUCCESS) [[likely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Wait before trying again
								Sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND);
								
							// Otherwise
							#else
							
								// Wait before trying again
								sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS);
							#endif
						}
						
						// Continue
						continue;
					}
					
					// Go through all received messages
					bool loggedIn = false;
					char *currentMessageStart = receiveBuffer;
					char *currentMessageEnd = reinterpret_cast<char *>(__builtin_memchr(currentMessageStart, '\n', totalBytesReceived));
					
					do [[unlikely]] {
					
						// Make current message a string
						*currentMessageEnd = '\0';
						
						// Check if displaying stratum server messages
						#if DISPLAY_STRATUM_SERVER_MESSAGES
						
							// Display message
							cout << "Received: " << currentMessageStart << endl;
						#endif
						
						// Check if current message is a response to the login request
						if(__builtin_strstr(currentMessageStart, "\"method\":\"login\"") || __builtin_strstr(currentMessageStart, "\"method\": \"login\"")) [[likely]] {
						
							// Set logged in to if the current message is that logging in was successful
							loggedIn = (!__builtin_strstr(currentMessageStart, "\"error\":") || __builtin_strstr(currentMessageStart, "\"error\":null") || __builtin_strstr(currentMessageStart, "\"error\": null")) && !__builtin_strstr(currentMessageStart, "\"result\":null") && !__builtin_strstr(currentMessageStart, "\"result\": null");
							
							// Check if current message is that logging in failed
							if(!loggedIn) [[unlikely]] {
							
								// Check if current message contains a message
								const char *message = __builtin_strstr(currentMessageStart, "\"message\":");
								if(message) [[likely]] {
								
									// Check if message contains a value
									message = __builtin_strchr(&message[sizeof("\"message\":") - sizeof('\0')], '"');
									if(message) [[likely]] {
									
										// Display message
										cout << "Message from the stratum server: ";
										
										// Go through all characters in the message
										for(++message; *message && *message != '"'; ++message) [[likely]] {
										
											// Check if character is an escaped double quote or backslash
											if(*message == '\\' && (message[sizeof('\\')] == '"' || message[sizeof('\\')] == '\\')) [[unlikely]] {
											
												// Go to next character
												++message;
											}
											
											// Check if character is printable
											if(isprint(*message)) [[likely]] {
											
												// Display character
												cout << *message;
											}
										}
										
										// Display new line
										cout << endl;
									}
								}
							}
						}
						
						// Get the start and end of the next message
						totalBytesReceived -= currentMessageEnd + sizeof('\n') - currentMessageStart;
						currentMessageStart = currentMessageEnd + sizeof('\n');
						currentMessageEnd = reinterpret_cast<char *>(__builtin_memchr(currentMessageStart, '\n', totalBytesReceived));
						
					} while(currentMessageEnd);
					
					// Remove received messages that were processed
					__builtin_memmove(receiveBuffer, currentMessageStart, totalBytesReceived);
					
					// Check if logging into the stratum server failed
					if(!loggedIn) [[unlikely]] {
					
						// Display message
						cout << "Logging into the stratum server failed" << endl;
						
						// Check if previously connected to the stratum server
						if(returnStatus == EXIT_SUCCESS) [[likely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Wait before trying again
								Sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND);
								
							// Otherwise
							#else
							
								// Wait before trying again
								sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS);
							#endif
						}
						
						// Continue
						continue;
					}
					
					// Display message
					cout << "Logged into the stratum server" << endl;
					
					// Display message
					cout << "Getting job from the stratum server" << endl;
					
					// Create get job template request
					const char getJobTemplateRequest[] = "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"getjobtemplate\",\"params\":null}\n";
					
					// Loop until full message is sent
					totalBytesSent = 0;
					do [[unlikely]] {
					
						// Check if using Windows
						#ifdef _WIN32
						
							// Send data to the stratum server
							const int bytesSent = send(socketDescriptor, &getJobTemplateRequest[totalBytesSent], sizeof(getJobTemplateRequest) - sizeof('\0') - totalBytesSent, 0);
							
						// Otherwise
						#else
						
							// Send data to the stratum server
							const ssize_t bytesSent = send(socketDescriptor, &getJobTemplateRequest[totalBytesSent], sizeof(getJobTemplateRequest) - sizeof('\0') - totalBytesSent, MSG_NOSIGNAL);
						#endif
						
						// Check if sending data to the stratum server failed
						if(bytesSent <= 0) [[unlikely]] {
						
							// Break
							break;
						}
						
						// Update total bytes sent
						totalBytesSent += bytesSent;
						
					} while(totalBytesSent != sizeof(getJobTemplateRequest) - sizeof('\0'));
					
					// Check if sending data to the stratum server failed
					if(totalBytesSent != sizeof(getJobTemplateRequest) - sizeof('\0')) [[unlikely]] {
					
						// Display message
						cout << "Sending data to the stratum server failed" << endl;
						
						// Check if previously connected to the stratum server
						if(returnStatus == EXIT_SUCCESS) [[likely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Wait before trying again
								Sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND);
								
							// Otherwise
							#else
							
								// Wait before trying again
								sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS);
							#endif
						}
						
						// Continue
						continue;
					}
					
					// Check if displaying stratum server messages
					#if DISPLAY_STRATUM_SERVER_MESSAGES
					
						// Display message
						cout << "Sent: " << getJobTemplateRequest;
					#endif
					
					// Loop until full message is received
					for(bool fullMessageReceived = false; !fullMessageReceived;) [[unlikely]] {
					
						// Check if receiving data from the stratum server failed
						const decltype(function(recv))::result_type bytesReceived = recv(socketDescriptor, &receiveBuffer[totalBytesReceived], sizeof(receiveBuffer) - totalBytesReceived, 0);
						if(bytesReceived <= 0) [[unlikely]] {
						
							// Set that receiving data from the stratum server failed
							totalBytesReceived = 0;
							
							// Break
							break;
						}
						
						// Set full message received to if the received data contains the end of a message
						fullMessageReceived = __builtin_memchr(&receiveBuffer[totalBytesReceived], '\n', bytesReceived);
						
						// Update total bytes received
						totalBytesReceived += bytesReceived;
						
						// Check if receive buffer is full and full message hasn't been received
						if(totalBytesReceived == sizeof(receiveBuffer) && !fullMessageReceived) [[unlikely]] {
						
							// Set that receiving data from the stratum server failed
							totalBytesReceived = 0;
							
							// Break
							break;
						}
					};
					
					// Check if receiving data from the stratum server failed
					if(!totalBytesReceived) [[unlikely]] {
					
						// Display message
						cout << "Receiving data from the stratum server failed" << endl;
						
						// Check if previously connected to the stratum server
						if(returnStatus == EXIT_SUCCESS) [[likely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Wait before trying again
								Sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND);
								
							// Otherwise
							#else
							
								// Wait before trying again
								sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS);
							#endif
						}
						
						// Continue
						continue;
					}
					
					// Go through all received messages
					bool gotJob = false;
					currentMessageStart = receiveBuffer;
					currentMessageEnd = reinterpret_cast<char *>(__builtin_memchr(currentMessageStart, '\n', totalBytesReceived));
					
					do [[unlikely]] {
					
						// Make current message a string
						*currentMessageEnd = '\0';
						
						// Check if displaying stratum server messages
						#if DISPLAY_STRATUM_SERVER_MESSAGES
						
							// Display message
							cout << "Received: " << currentMessageStart << endl;
						#endif
						
						// Check if current message is a response to the get job template request
						if(__builtin_strstr(currentMessageStart, "\"method\":\"getjobtemplate\"") || __builtin_strstr(currentMessageStart, "\"method\": \"getjobtemplate\"")) [[likely]] {
						
							// Set got job to if the current message contains a job
							gotJob = (!__builtin_strstr(currentMessageStart, "\"error\":") || __builtin_strstr(currentMessageStart, "\"error\":null") || __builtin_strstr(currentMessageStart, "\"error\": null")) && !__builtin_strstr(currentMessageStart, "\"result\":null") && !__builtin_strstr(currentMessageStart, "\"result\": null");
							
							// Check if current message doesn't contain a job
							if(!gotJob) [[unlikely]] {
							
								// Check if current message contains a message
								const char *message = __builtin_strstr(currentMessageStart, "\"message\":");
								if(message) [[likely]] {
								
									// Check if message contains a value
									message = __builtin_strchr(&message[sizeof("\"message\":") - sizeof('\0')], '"');
									if(message) [[likely]] {
									
										// Display message
										cout << "Message from the stratum server: ";
										
										// Go through all characters in the message
										for(++message; *message && *message != '"'; ++message) [[likely]] {
										
											// Check if character is an escaped double quote or backslash
											if(*message == '\\' && (message[sizeof('\\')] == '"' || message[sizeof('\\')] == '\\')) [[unlikely]] {
											
												// Go to next character
												++message;
											}
											
											// Check if character is printable
											if(isprint(*message)) [[likely]] {
											
												// Display character
												cout << *message;
											}
										}
										
										// Display new line
										cout << endl;
									}
								}
							}
							
							// Otherwise
							else [[likely]] {
							
								// Check if reading the job message failed
								if(!readJobMessage(currentMessageStart, jobHeader, jobHeight[0], jobId[0])) [[unlikely]] {
								
									// Set got job to false
									gotJob = false;
									
									// Display message
									cout << "Received invalid job from the stratum server" << endl;
								}
								
								// Otherwise
								else [[likely]] {
								
									// Create random job nonce
									jobNonce[0] = randomNumberGenerator();
									
									// Set next job nonce, job height, and job ID
									jobNonce[1] = jobNonce[0] + 1;
									jobHeight[1] = jobHeight[0];
									jobId[1] = jobId[0];
								}
							}
						}
						
						// Get the start and end of the next message
						totalBytesReceived -= currentMessageEnd + sizeof('\n') - currentMessageStart;
						currentMessageStart = currentMessageEnd + sizeof('\n');
						currentMessageEnd = reinterpret_cast<char *>(__builtin_memchr(currentMessageStart, '\n', totalBytesReceived));
						
					} while(currentMessageEnd);
					
					// Remove received messages that were processed
					__builtin_memmove(receiveBuffer, currentMessageStart, totalBytesReceived);
					
					// Check if getting job from the stratum server failed
					if(!gotJob) [[unlikely]] {
					
						// Display message
						cout << "Getting job from the stratum server failed" << endl;
						
						// Check if previously connected to the stratum server
						if(returnStatus == EXIT_SUCCESS) [[likely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Wait before trying again
								Sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS * MILLISECONDS_IN_A_SECOND);
								
							// Otherwise
							#else
							
								// Wait before trying again
								sleep(STRATUM_SERVER_RECONNECT_AFTER_FAILURE_DELAY_SECONDS);
							#endif
						}
						
						// Continue
						continue;
					}
					
					// Display message
					cout << "Got job from the stratum server" << endl;
				}
				
				// Update last keep alive time
				chrono::steady_clock::time_point lastKeepAliveTime = chrono::steady_clock::now();
			#endif
			
			// Display message
			cout << "Priming pipeline" << endl;
			
			// Get SipHash keys from current job header and job nonce
			uint64_t __attribute__((vector_size(sizeof(uint64_t) * NUMBER_OF_SIPHASH_KEYS))) sipHashKeys[2];
			blake2b(sipHashKeys[0], jobHeader, jobNonce[0]);
			
			// Check if using an Apple device
			#ifdef __APPLE__
			
				// Check if creating autorelease pool failed
				autoreleasePool = unique_ptr<NS::AutoreleasePool, void(*)(NS::AutoreleasePool *)>(NS::AutoreleasePool::alloc()->init(), [](NS::AutoreleasePool *autoreleasePool) __attribute__((always_inline)) noexcept {
				
					// Free autorelease pool
					__builtin_assume_dereferenceable(autoreleasePool, sizeof(*autoreleasePool));
					autoreleasePool->release();
				});
				if(!autoreleasePool) [[unlikely]] {
				
					// Display message
					cout << "Creating autorelease pool failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Update trim edges parameters buffer
				__builtin_memcpy_inline(trimEdgesParametersBuffer->contents(), &sipHashKeys[0], sizeof(TrimEdgesParameters));
				
				// Start encoding commands into the trim edges command buffer
				trimEdgesCommandAllocator->reset();
				trimEdgesCommandBuffer->beginCommandBuffer(trimEdgesCommandAllocator.get());
				
				// Check if creating command encoder for the trim edges command buffer failed
				MTL4::ComputeCommandEncoder *commandEncoder = trimEdgesCommandBuffer->computeCommandEncoder();
				if(!commandEncoder) [[unlikely]] {
				
					// Display message
					cout << "Creating command encoder for the trim edges command buffer failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Set command encoder's argument table
				commandEncoder->setArgumentTable(trimEdgesArgumentTable.get());
				
				// Encode clearing number of edges per coarse bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
				
				// Encode clearing number of edges per fine bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running coarse bucket sort edges pipeline
				commandEncoder->setComputePipelineState(coarseBucketSortEdgesPipeline.get());
				commandEncoder->dispatchThreads(MTL::Size(NUMBER_OF_EDGES / 2, 1, 1), MTL::Size(GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running update largest initial coarse bucket size pipeline
				commandEncoder->setComputePipelineState(updateLargestInitialCoarseBucketSizePipeline.get());
				commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestInitialCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running fine bucket sort initial edges pipeline
				commandEncoder->setComputePipelineState(fineBucketSortInitialEdgesPipeline.get());
				commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
				
				// Encode clearing number of edges per coarse bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running trim initial edges pipeline
				commandEncoder->setComputePipelineState(trimInitialEdgesPipeline.get());
				commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Check if using max RAM for GPU trimming or using more RAM for GPU trimming
				#if GPU_TRIMMING_USE_MAX_RAM || GPU_TRIMMING_USE_MORE_RAM
				
					// Encode running update largest final coarse bucket size pipeline
					commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per fine bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running fine bucket sort final edges pipeline
					commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per coarse bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running trim final edges pipeline
					commandEncoder->setComputePipelineState(trimFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
				// Otherwise
				#else
				
					// Encode running update largest intermediate coarse bucket size pipeline
					commandEncoder->setComputePipelineState(updateLargestIntermediateCoarseBucketSizePipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestIntermediateCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per fine bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running fine bucket sort intermediate edges pipeline
					commandEncoder->setComputePipelineState(fineBucketSortIntermediateEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per coarse bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running trim intermediate edges pipeline
					commandEncoder->setComputePipelineState(trimIntermediateEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				#endif
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running update largest final coarse bucket size pipeline
				commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
				commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
				
				// Encode clearing number of edges per fine bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running fine bucket sort final edges pipeline
				commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
				commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Go through all remaining GPU trimming rounds except the last one
				for(int i = 2; i < GPU_TRIMMING_ROUNDS - 1; ++i) [[likely]] {
				
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per coarse bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running trim final edges pipeline
					commandEncoder->setComputePipelineState(trimFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running update largest final coarse bucket size pipeline
					commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per fine bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running fine bucket sort final edges pipeline
					commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				}
				
				// Finish encoding commands into the trim edges command buffer
				commandEncoder->endEncoding();
				trimEdgesCommandBuffer->endCommandBuffer();
				
				// Check if creating trim edges commit options failed
				MTL4::CommitOptions *trimEdgesCommitOptions = MTL4::CommitOptions::alloc()->init();
				if(!trimEdgesCommitOptions) [[unlikely]] {
				
					// Display message
					cout << "Creating trim edges commit options failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Add trim edges commit options to autorelease pool
				trimEdgesCommitOptions->autorelease();
				
				// Configure trim edges commit options
				trimEdgesCommitOptions->addFeedbackHandler(commitOptionsFeedbackHandler);
				
				// Start GPU trimming on the current job
				commandQueue->commit((const MTL4::CommandBuffer *[]){trimEdgesCommandBuffer.get()}, 1, trimEdgesCommitOptions);
				
				// Check if not displaying tuning times
				#if !DISPLAY_TUNING_TIMES
				
					// Check if CPU trimming threads haven't been primed
					if(returnStatus != EXIT_SUCCESS) [[unlikely]] {
					
						// Start CPU trimming on nothing
						cpuTrimmingThreadsCoarseBucketsOne = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET]>(cpuBucketsBuffer->contents());
						cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(numberOfEdgesPerCpuBucketBuffer->contents());
						cpuTrimmingThreadsFinished = false;
						startCpuTrimmingThreadsTriggerToggle = !startCpuTrimmingThreadsTriggerToggle;
						cpuTrimmingThreadsLock.unlock();
						startCpuTrimmingThreadsConditionalVariable.notify_all();
						
						// Wait for CPU trimming to finish
						cpuTrimmingThreadsLock.lock();
						cpuTrimmingThreadsFinishedConditionalVariable.wait(cpuTrimmingThreadsLock, [&cpuTrimmingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
						
							// Return if CPU trimming threads have finished
							return cpuTrimmingThreadsFinished;
						});
					}
				#endif
				
				// Check if waiting for GPU trimming to finish failed
				commandQueueFinishedSemaphore.acquire();
				if(gpuError) [[unlikely]] {
				
					// Display message
					cout << "Waiting for GPU trimming to finish failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Start encoding commands into the transfer edges command buffer
				transferEdgesCommandAllocator[0]->reset();
				transferEdgesCommandBuffer->beginCommandBuffer(transferEdgesCommandAllocator[0].get());
				
				// Check if creating command encoder for the transfer edges command buffer failed
				commandEncoder = transferEdgesCommandBuffer->computeCommandEncoder();
				if(!commandEncoder) [[unlikely]] {
				
					// Display message
					cout << "Creating command encoder for the transfer edges command buffer failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Set command encoder's argument table
				commandEncoder->setArgumentTable(transferEdgesArgumentTable.get());
				
				// Encode clearing number of edges per CPU bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerCpuBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCpuBucketBuffer->length()), 0);
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running trim final edges and transfer edges pipeline
				commandEncoder->setComputePipelineState(trimFinalEdgesAndTransferEdgesPipeline.get());
				commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Finish encoding commands into the transfer edges command buffer
				commandEncoder->endEncoding();
				transferEdgesCommandBuffer->endCommandBuffer();
				
				// Check if creating transfer edges commit options failed
				MTL4::CommitOptions *transferEdgesCommitOptions = MTL4::CommitOptions::alloc()->init();
				if(!transferEdgesCommitOptions) [[unlikely]] {
				
					// Display message
					cout << "Creating transfer edges commit options failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Add transfer edges commit options to autorelease pool
				transferEdgesCommitOptions->autorelease();
				
				// Configure transfer edges commit options
				transferEdgesCommitOptions->addFeedbackHandler(commitOptionsFeedbackHandler);
				
				// Start GPU transferring on the current job
				commandQueue->commit((const MTL4::CommandBuffer *[]){transferEdgesCommandBuffer.get()}, 1, transferEdgesCommitOptions);
				
				// Check if not displaying tuning times
				#if !DISPLAY_TUNING_TIMES
				
					// Check if CPU searching threads haven't been primed
					if(returnStatus != EXIT_SUCCESS) [[unlikely]] {
					
						// Start CPU searching on nothing
						recoverEdgesParameters.solutionNodes[3] = 0;
						cpuSearchingThreadsFinished = false;
						startCpuSearchingThreadsTriggerToggle = !startCpuSearchingThreadsTriggerToggle;
						cpuSearchingThreadsLock.unlock();
						startCpuSearchingThreadsConditionalVariable.notify_all();
						
						// Wait for CPU searching to finish
						cpuSearchingThreadsLock.lock();
						cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
						
							// Return if CPU searching threads have finished
							return cpuSearchingThreadsFinished;
						});
					}
				#endif
				
				// Check if waiting for GPU transferring to finish failed
				commandQueueFinishedSemaphore.acquire();
				if(gpuError) [[unlikely]] {
				
					// Display message
					cout << "Waiting for GPU transferring to finish failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Free objects in autorelease pool
				autoreleasePool.reset();
				
				// Check if creating autorelease pool failed
				autoreleasePool = unique_ptr<NS::AutoreleasePool, void(*)(NS::AutoreleasePool *)>(NS::AutoreleasePool::alloc()->init(), [](NS::AutoreleasePool *autoreleasePool) __attribute__((always_inline)) noexcept {
				
					// Free autorelease pool
					__builtin_assume_dereferenceable(autoreleasePool, sizeof(*autoreleasePool));
					autoreleasePool->release();
				});
				if(!autoreleasePool) [[unlikely]] {
				
					// Display message
					cout << "Creating autorelease pool failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
			// Otherwise
			#else
			
				// Create mapped buffers
				uint64_t *mappedCpuBucketsBuffer = nullptr;
				uint32_t *mappedNumberOfEdgesPerCpuBucketBuffer = nullptr;
				uint32_t *mappedSolutionEdgesBuffer = nullptr;
				
				// Automatically unmap mapped buffers when done
				const auto mappedBuffersUniquePointerDestructor = [commandQueue = commandQueue.get(), cpuBucketsBuffer = cpuBucketsBuffer.get(), numberOfEdgesPerCpuBucketBuffer = numberOfEdgesPerCpuBucketBuffer.get(), solutionEdgesBuffer = solutionEdgesBuffer.get(), &mappedCpuBucketsBuffer, &mappedNumberOfEdgesPerCpuBucketBuffer, &mappedSolutionEdgesBuffer](const cl_device_id *unused [[maybe_unused]]) __attribute__((always_inline)) noexcept {
				
					// Check if CPU buckets buffer is mapped
					if(mappedCpuBucketsBuffer) [[likely]] {
					
						// Enqueue unmapping CPU buckets buffer
						clEnqueueUnmapMemObject(commandQueue, cpuBucketsBuffer, reinterpret_cast<void *>(mappedCpuBucketsBuffer), 0, nullptr, nullptr);
					}
					
					// Check if number of edges per CPU bucket buffer is mapped
					if(mappedNumberOfEdgesPerCpuBucketBuffer) [[likely]] {
					
						// Enqueue unmapping number of edges per CPU bucket buffer
						clEnqueueUnmapMemObject(commandQueue, numberOfEdgesPerCpuBucketBuffer, reinterpret_cast<void *>(mappedNumberOfEdgesPerCpuBucketBuffer), 0, nullptr, nullptr);
					}
					
					// Check if solution edges buffer is mapped
					if(mappedSolutionEdgesBuffer) [[likely]] {
					
						// Enqueue unmapping solution edges buffer
						clEnqueueUnmapMemObject(commandQueue, solutionEdgesBuffer, reinterpret_cast<void *>(mappedSolutionEdgesBuffer), 0, nullptr, nullptr);
					}
					
					// Wait for commands to finish
					clFlush(commandQueue);
					clFinish(commandQueue);
				};
				
				const unique_ptr<cl_device_id, decltype(mappedBuffersUniquePointerDestructor)> mappedBuffersUniquePointer(&gpu, mappedBuffersUniquePointerDestructor);
				
				// Check if mapping buffers failed
				mappedCpuBucketsBuffer = reinterpret_cast<uint64_t *>(clEnqueueMapBuffer(commandQueue.get(), cpuBucketsBuffer.get(), CL_TRUE, CL_MAP_READ, 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, 0, nullptr, nullptr, nullptr));
				mappedNumberOfEdgesPerCpuBucketBuffer = reinterpret_cast<uint32_t *>(clEnqueueMapBuffer(commandQueue.get(), numberOfEdgesPerCpuBucketBuffer.get(), CL_TRUE, CL_MAP_READ, 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr, nullptr));
				mappedSolutionEdgesBuffer = reinterpret_cast<uint32_t *>(clEnqueueMapBuffer(commandQueue.get(), solutionEdgesBuffer.get(), CL_TRUE, CL_MAP_READ, 0, SOLUTION_SIZE * sizeof(uint32_t), 0, nullptr, nullptr, nullptr));
				if(!mappedCpuBucketsBuffer || !mappedNumberOfEdgesPerCpuBucketBuffer || !mappedSolutionEdgesBuffer) [[unlikely]] {
				
					// Display message
					cout << "Mapping buffers failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Create GPU events
				cl_event gpuFirstEvent = nullptr;
				cl_event gpuLastEvent = nullptr;
				
				// Automatically free GPU events when done
				const auto gpuEventsUniquePointerDestructor = [&gpuFirstEvent, &gpuLastEvent](const cl_device_id *unused [[maybe_unused]]) __attribute__((always_inline)) noexcept {
				
					// Check if GPU first event exists
					if(gpuFirstEvent) [[unlikely]] {
					
						// Wait for GPU first events to finish
						clWaitForEvents(1, &gpuFirstEvent);
						
						// Free GPU first event
						clReleaseEvent(gpuFirstEvent);
					}
					
					// Check if GPU last event exists
					if(gpuLastEvent) [[unlikely]] {
					
						// Wait for GPU last event to finish
						clWaitForEvents(1, &gpuLastEvent);
						
						// Free GPU last events
						clReleaseEvent(gpuLastEvent);
					}
				};
				
				const unique_ptr<cl_device_id, decltype(gpuEventsUniquePointerDestructor)> gpuEventsUniquePointer(&gpu, gpuEventsUniquePointerDestructor);
				
				// Check if starting GPU trimming on the current job failed
				if(
				
					// Enqueue updating trim edges parameters buffer
					clEnqueueWriteBuffer(commandQueue.get(), trimEdgesParametersBuffer.get(), CL_FALSE, 0, sizeof(TrimEdgesParameters), &sipHashKeys[0], 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue clearing number of edges per coarse bucket buffer
					clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue clearing number of edges per fine bucket buffer
					clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue running coarse bucket sort edges kernel
					clEnqueueNDRangeKernel(commandQueue.get(), coarseBucketSortEdgesKernel.get(), 1, nullptr, (const size_t[]){NUMBER_OF_EDGES / 2}, (const size_t[]){GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue running update largest initial coarse bucket size kernel
					clEnqueueNDRangeKernel(commandQueue.get(), updateLargestInitialCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue running fine bucket sort initial edges kernel
					clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortInitialEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue clearing number of edges per coarse bucket buffer
					clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue running trim initial edges kernel
					clEnqueueNDRangeKernel(commandQueue.get(), trimInitialEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Check if using max RAM for GPU trimming or using more RAM for GPU trimming
					#if GPU_TRIMMING_USE_MAX_RAM || GPU_TRIMMING_USE_MORE_RAM
					
						// Enqueue running update largest final coarse bucket size kernel
						clEnqueueNDRangeKernel(commandQueue.get(), updateLargestFinalCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per fine bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running fine bucket sort final edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortFinalEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[1] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per coarse bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running trim final edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), trimFinalEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
					// Otherwise
					#else
					
						// Enqueue running update largest intermediate coarse bucket size kernel
						clEnqueueNDRangeKernel(commandQueue.get(), updateLargestIntermediateCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per fine bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running fine bucket sort intermediate edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortIntermediateEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[1] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per coarse bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running trim intermediate edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), trimIntermediateEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
					#endif
					
					// Enqueue running update largest final coarse bucket size kernel
					clEnqueueNDRangeKernel(commandQueue.get(), updateLargestFinalCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue clearing number of edges per fine bucket buffer
					clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Check if there's three GPU trimming rounds
					#if GPU_TRIMMING_ROUNDS == 3
					
						// Enqueue running fine bucket sort final edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortFinalEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[2] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, &gpuLastEvent) != CL_SUCCESS
						
					// Otherwise
					#else
					
						// Enqueue running fine bucket sort final edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortFinalEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[2] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, nullptr) != CL_SUCCESS
					#endif
					
				) [[unlikely]] {
				
					// Display message
					cout << "Starting GPU trimming on the current job failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Go through all remaining GPU trimming rounds except the last one
				bool gpuError = false;
				for(int i = 2; i < GPU_TRIMMING_ROUNDS - 1; ++i) [[likely]] {
				
					// Check if continuing GPU trimming on the current job failed
					if(
					
						// Enqueue clearing number of edges per coarse bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running trim final edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), trimFinalEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running update largest final coarse bucket size kernel
						clEnqueueNDRangeKernel(commandQueue.get(), updateLargestFinalCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per fine bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running fine bucket sort final edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortFinalEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[i + 1] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, __builtin_expect(i == GPU_TRIMMING_ROUNDS - 2, false) ? &gpuLastEvent : nullptr) != CL_SUCCESS
						
					) [[unlikely]] {
					
						// Set GPU error to true
						gpuError = true;
						
						// Break
						break;
					}
				}
				
				// Check if continuing GPU trimming on the current job failed
				if(gpuError || clFlush(commandQueue.get()) != CL_SUCCESS) [[unlikely]] {
				
					// Display message
					cout << "Starting GPU trimming on the current job failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Check if not displaying tuning times
				#if !DISPLAY_TUNING_TIMES
				
					// Check if CPU trimming threads haven't been primed
					if(returnStatus != EXIT_SUCCESS) [[unlikely]] {
					
						// Start CPU trimming on nothing
						cpuTrimmingThreadsCoarseBucketsOne = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET]>(mappedCpuBucketsBuffer);
						cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(mappedNumberOfEdgesPerCpuBucketBuffer);
						cpuTrimmingThreadsFinished = false;
						startCpuTrimmingThreadsTriggerToggle = !startCpuTrimmingThreadsTriggerToggle;
						cpuTrimmingThreadsLock.unlock();
						startCpuTrimmingThreadsConditionalVariable.notify_all();
						
						// Wait for CPU trimming to finish
						cpuTrimmingThreadsLock.lock();
						cpuTrimmingThreadsFinishedConditionalVariable.wait(cpuTrimmingThreadsLock, [&cpuTrimmingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
						
							// Return if CPU trimming threads have finished
							return cpuTrimmingThreadsFinished;
						});
					}
				#endif
				
				// Check if waiting for GPU trimming to finish failed
				if(clWaitForEvents(1, &gpuLastEvent) != CL_SUCCESS || clReleaseEvent(gpuLastEvent) != CL_SUCCESS || (gpuLastEvent = nullptr)) [[unlikely]] {
				
					// Display message
					cout << "Waiting for GPU trimming to finish failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Check if starting GPU transferring on the current job failed
				if(
				
					// Enqueue unmapping CPU buckets buffer
					clEnqueueUnmapMemObject(commandQueue.get(), cpuBucketsBuffer.get(), reinterpret_cast<void *>(mappedCpuBucketsBuffer), 0, nullptr, nullptr) != CL_SUCCESS || (mappedCpuBucketsBuffer = nullptr) ||
					
					// Enqueue unmapping number of edges per CPU bucket buffer
					clEnqueueUnmapMemObject(commandQueue.get(), numberOfEdgesPerCpuBucketBuffer.get(), reinterpret_cast<void *>(mappedNumberOfEdgesPerCpuBucketBuffer), 0, nullptr, nullptr) != CL_SUCCESS || (mappedNumberOfEdgesPerCpuBucketBuffer = nullptr) ||
					
					// Enqueue clearing number of edges per CPU bucket buffer
					clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCpuBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue running trim final edges and transfer edges kernel
					clEnqueueNDRangeKernel(commandQueue.get(), trimFinalEdgesAndTransferEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
					
					// Enqueue mapping CPU buckets buffer
					!(mappedCpuBucketsBuffer = reinterpret_cast<uint64_t *>(clEnqueueMapBuffer(commandQueue.get(), cpuBucketsBuffer.get(), CL_FALSE, CL_MAP_READ, 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, 0, nullptr, nullptr, nullptr))) ||
					
					// Enqueue mapping number of edges per CPU bucket buffer
					!(mappedNumberOfEdgesPerCpuBucketBuffer = reinterpret_cast<uint32_t *>(clEnqueueMapBuffer(commandQueue.get(), numberOfEdgesPerCpuBucketBuffer.get(), CL_FALSE, CL_MAP_READ, 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, &gpuLastEvent, nullptr))) ||
					
					// Flush commands to queue
					clFlush(commandQueue.get()) != CL_SUCCESS
					
				) [[unlikely]] {
				
					// Display message
					cout << "Starting GPU transferring on the current job failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Check if not displaying tuning times
				#if !DISPLAY_TUNING_TIMES
				
					// Check if CPU searching threads haven't been primed
					if(returnStatus != EXIT_SUCCESS) [[unlikely]] {
						
						// Start CPU searching on nothing
						recoverEdgesParameters.solutionNodes[3] = 0;
						cpuSearchingThreadsFinished = false;
						startCpuSearchingThreadsTriggerToggle = !startCpuSearchingThreadsTriggerToggle;
						cpuSearchingThreadsLock.unlock();
						startCpuSearchingThreadsConditionalVariable.notify_all();
						
						// Wait for CPU searching to finish
						cpuSearchingThreadsLock.lock();
						cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
						
							// Return if CPU searching threads have finished
							return cpuSearchingThreadsFinished;
						});
					}
				#endif
				
				// Check if waiting for GPU transferring to finish failed
				if(clWaitForEvents(1, &gpuLastEvent) != CL_SUCCESS || clReleaseEvent(gpuLastEvent) != CL_SUCCESS || (gpuLastEvent = nullptr)) [[unlikely]] {
				
					// Display message
					cout << "Waiting for GPU transferring to finish failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
			#endif
			
			// Set return status to success
			returnStatus = EXIT_SUCCESS;
			
			// Go to next job
			int currentJobIndex = 1;
			uint64_t graphsProcessed = 0;
			
			// Get SipHash keys from current job header and job nonce
			blake2b(sipHashKeys[currentJobIndex], jobHeader, jobNonce[currentJobIndex]);
			
			// Check if using an Apple device
			#ifdef __APPLE__
			
				// Update trim edges parameters buffer
				__builtin_memcpy_inline(trimEdgesParametersBuffer->contents(), &sipHashKeys[currentJobIndex], sizeof(TrimEdgesParameters));
				
				// Start encoding commands into the trim edges command buffer
				trimEdgesCommandAllocator->reset();
				trimEdgesCommandBuffer->beginCommandBuffer(trimEdgesCommandAllocator.get());
				
				// Check if creating command encoder for the trim edges command buffer failed
				commandEncoder = trimEdgesCommandBuffer->computeCommandEncoder();
				if(!commandEncoder) [[unlikely]] {
				
					// Display message
					cout << "Creating command encoder for the trim edges command buffer failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Set command encoder's argument table
				commandEncoder->setArgumentTable(trimEdgesArgumentTable.get());
				
				// Encode clearing number of edges per coarse bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
				
				// Encode clearing number of edges per fine bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running coarse bucket sort edges pipeline
				commandEncoder->setComputePipelineState(coarseBucketSortEdgesPipeline.get());
				commandEncoder->dispatchThreads(MTL::Size(NUMBER_OF_EDGES / 2, 1, 1), MTL::Size(GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running update largest initial coarse bucket size pipeline
				commandEncoder->setComputePipelineState(updateLargestInitialCoarseBucketSizePipeline.get());
				commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestInitialCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running fine bucket sort initial edges pipeline
				commandEncoder->setComputePipelineState(fineBucketSortInitialEdgesPipeline.get());
				commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
				
				// Encode clearing number of edges per coarse bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running trim initial edges pipeline
				commandEncoder->setComputePipelineState(trimInitialEdgesPipeline.get());
				commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Check if using max RAM for GPU trimming or using more RAM for GPU trimming
				#if GPU_TRIMMING_USE_MAX_RAM || GPU_TRIMMING_USE_MORE_RAM
				
					// Encode running update largest final coarse bucket size pipeline
					commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per fine bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running fine bucket sort final edges pipeline
					commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per coarse bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running trim final edges pipeline
					commandEncoder->setComputePipelineState(trimFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
				// Otherwise
				#else
				
					// Encode running update largest intermediate coarse bucket size pipeline
					commandEncoder->setComputePipelineState(updateLargestIntermediateCoarseBucketSizePipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestIntermediateCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per fine bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running fine bucket sort intermediate edges pipeline
					commandEncoder->setComputePipelineState(fineBucketSortIntermediateEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per coarse bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running trim intermediate edges pipeline
					commandEncoder->setComputePipelineState(trimIntermediateEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				#endif
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running update largest final coarse bucket size pipeline
				commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
				commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
				
				// Encode clearing number of edges per fine bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running fine bucket sort final edges pipeline
				commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
				commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Go through all remaining GPU trimming rounds except the last one
				for(int i = 2; i < GPU_TRIMMING_ROUNDS - 1; ++i) [[likely]] {
				
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per coarse bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running trim final edges pipeline
					commandEncoder->setComputePipelineState(trimFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running update largest final coarse bucket size pipeline
					commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per fine bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running fine bucket sort final edges pipeline
					commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				}
				
				// Finish encoding commands into the trim edges command buffer
				commandEncoder->endEncoding();
				trimEdgesCommandBuffer->endCommandBuffer();
				
				// Check if creating trim edges commit options failed
				trimEdgesCommitOptions = MTL4::CommitOptions::alloc()->init();
				if(!trimEdgesCommitOptions) [[unlikely]] {
				
					// Display message
					cout << "Creating trim edges commit options failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Add trim edges commit options to autorelease pool
				trimEdgesCommitOptions->autorelease();
				
				// Configure trim edges commit options
				trimEdgesCommitOptions->addFeedbackHandler(commitOptionsFeedbackHandler);
			#endif
			
			// Prepare to start CPU trimming on the previous job
			cpuTrimmingThreadsFinished = false;
			
			// Check if using an Apple device
			#ifdef __APPLE__
			
				// Start encoding commands into the transfer edges command buffer
				transferEdgesCommandAllocator[currentJobIndex]->reset();
				transferEdgesCommandBuffer->beginCommandBuffer(transferEdgesCommandAllocator[currentJobIndex].get());
				
				// Check if creating command encoder for the transfer edges command buffer failed
				commandEncoder = transferEdgesCommandBuffer->computeCommandEncoder();
				if(!commandEncoder) [[unlikely]] {
				
					// Display message
					cout << "Creating command encoder for the transfer edges command buffer failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Set command encoder's argument table
				commandEncoder->setArgumentTable(transferEdgesArgumentTable.get());
				
				// Encode clearing number of edges per CPU bucket buffer
				commandEncoder->fillBuffer(numberOfEdgesPerCpuBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCpuBucketBuffer->length()), 0);
				
				// Encode a barrier
				commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
				
				// Encode running trim final edges and transfer edges pipeline
				commandEncoder->setComputePipelineState(trimFinalEdgesAndTransferEdgesPipeline.get());
				commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
				
				// Finish encoding commands into the transfer edges command buffer
				commandEncoder->endEncoding();
				transferEdgesCommandBuffer->endCommandBuffer();
				
				// Check if creating transfer edges commit options failed
				transferEdgesCommitOptions = MTL4::CommitOptions::alloc()->init();
				if(!transferEdgesCommitOptions) [[unlikely]] {
				
					// Display message
					cout << "Creating transfer edges commit options failed" << endl;
					
					// Set return status to failure
					returnStatus = EXIT_FAILURE;
					
					// Break
					break;
				}
				
				// Add transfer edges commit options to autorelease pool
				transferEdgesCommitOptions->autorelease();
				
				// Configure transfer edges commit options
				transferEdgesCommitOptions->addFeedbackHandler(commitOptionsFeedbackHandler);
			#endif
			
			// Check if displaying power usage
			#if DISPLAY_POWER_USAGE
			
				// Get energy consumption before
				pair energyConsumptionBefore = energyConsumption.getTotalEnergyConsumption();
				
				// Lock power usage thread lock
				powerUsageThreadLock.lock();
				
				// Reset total power used
				totalPowerUsed = 0;
				totalPowerSamples = 0;
				
				// Unlock power usage thread lock
				powerUsageThreadLock.unlock();
			#endif
			
			// Set solutions found to zero
			uint64_t solutionsFound = 0;
			
			// Display message
			cout << "Pipeline primed" << endl;
			
			// Display message
			cout << "Mining started" << endl << endl;
			
			// Get mining start time
			const chrono::steady_clock::time_point miningStartTime = chrono::steady_clock::now();
			
			// Get graph start time
			chrono::steady_clock::time_point graphStartTime = miningStartTime;
			
			// Get CPU start time
			chrono::steady_clock::time_point cpuStartTime = miningStartTime;
			
			// Check if stopping after a specified number of graphs
			#if STOP_AFTER_NUMBER_OF_GRAPHS != 0
			
				// Loop though all graphs to process
				while(graphsProcessed < STOP_AFTER_NUMBER_OF_GRAPHS) [[likely]] {
				
			// Otherwise
			#else
			
				// Loop forever
				while(true) {
			#endif
			
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Start GPU trimming on the current job
					commandQueue->commit((const MTL4::CommandBuffer *[]){trimEdgesCommandBuffer.get()}, 1, trimEdgesCommitOptions);
					
				// Otherwise
				#else
				
					// Check if starting GPU trimming on the current job failed
					if(
					
						// Enqueue updating trim edges parameters buffer
						clEnqueueWriteBuffer(commandQueue.get(), trimEdgesParametersBuffer.get(), CL_FALSE, 0, sizeof(TrimEdgesParameters), &sipHashKeys[currentJobIndex], 0, nullptr, &gpuFirstEvent) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per coarse bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per fine bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running coarse bucket sort edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), coarseBucketSortEdgesKernel.get(), 1, nullptr, (const size_t[]){NUMBER_OF_EDGES / 2}, (const size_t[]){GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running update largest initial coarse bucket size kernel
						clEnqueueNDRangeKernel(commandQueue.get(), updateLargestInitialCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running fine bucket sort initial edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortInitialEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per coarse bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running trim initial edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), trimInitialEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Check if using max RAM for GPU trimming or using more RAM for GPU trimming
						#if GPU_TRIMMING_USE_MAX_RAM || GPU_TRIMMING_USE_MORE_RAM
						
							// Enqueue running update largest final coarse bucket size kernel
							clEnqueueNDRangeKernel(commandQueue.get(), updateLargestFinalCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue clearing number of edges per fine bucket buffer
							clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue running fine bucket sort final edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortFinalEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[1] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue clearing number of edges per coarse bucket buffer
							clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue running trim final edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), trimFinalEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
							
						// Otherwise
						#else
						
							// Enqueue running update largest intermediate coarse bucket size kernel
							clEnqueueNDRangeKernel(commandQueue.get(), updateLargestIntermediateCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue clearing number of edges per fine bucket buffer
							clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue running fine bucket sort intermediate edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortIntermediateEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[1] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue clearing number of edges per coarse bucket buffer
							clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue running trim intermediate edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), trimIntermediateEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
						#endif
						
						// Enqueue running update largest final coarse bucket size kernel
						clEnqueueNDRangeKernel(commandQueue.get(), updateLargestFinalCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue clearing number of edges per fine bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Check if there's three GPU trimming rounds
						#if GPU_TRIMMING_ROUNDS == 3
						
							// Enqueue running fine bucket sort final edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortFinalEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[2] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, &gpuLastEvent) != CL_SUCCESS
							
						// Otherwise
						#else
						
							// Enqueue running fine bucket sort final edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortFinalEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[2] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, nullptr) != CL_SUCCESS
						#endif
						
					) [[unlikely]] {
					
						// Display message
						cout << "Starting GPU trimming on the current job failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Go through all remaining GPU trimming rounds except the last one
					for(int i = 2; i < GPU_TRIMMING_ROUNDS - 1; ++i) [[likely]] {
					
						// Check if continuing GPU trimming on the current job failed
						if(
						
							// Enqueue clearing number of edges per coarse bucket buffer
							clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCoarseBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue running trim final edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), trimFinalEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue running update largest final coarse bucket size kernel
							clEnqueueNDRangeKernel(commandQueue.get(), updateLargestFinalCoarseBucketSizeKernel.get(), 1, nullptr, (const size_t[]){1}, (const size_t[]){1}, 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue clearing number of edges per fine bucket buffer
							clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerFineBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue running fine bucket sort final edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), fineBucketSortFinalEdgesKernel.get(), 2, nullptr, (const size_t[]){static_cast<size_t>(((GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET_AFTER_TRIMMING_ROUND[i + 1] + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION}, (const size_t[]){GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1}, 0, nullptr, __builtin_expect(i == GPU_TRIMMING_ROUNDS - 2, false) ? &gpuLastEvent : nullptr) != CL_SUCCESS
							
						) [[unlikely]] {
						
							// Set GPU error to true
							gpuError = true;
							
							// Break
							break;
						}
					}
					
					// Check if continuing GPU trimming on the current job failed
					if(gpuError || clFlush(commandQueue.get()) != CL_SUCCESS) [[unlikely]] {
					
						// Display message
						cout << "Starting GPU trimming on the current job failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
				#endif
				
				// Check if displaying tuning times or not mining to a stratum server
				#if DISPLAY_TUNING_TIMES || !MINE_TO_A_STRATUM_SERVER
				
					// Check if not mining to a stratum server
					#if !MINE_TO_A_STRATUM_SERVER
					
						// Display message
						cout << "Header with nonce: 0x";
						
						// Go through all characters in the job header
						for(size_t i = 0; i < HEADER_SIZE_EXCLUDING_NONCE; ++i) [[likely]] {
						
							// Display character
							cout << hex << setw(sizeof(uint8_t) * HEXADECIMAL_CHARACTER_SIZE) << setfill('0') << static_cast<uint16_t>(jobHeader[i]) << setfill(' ') << setw(0) << dec;
						}
						
						// Check if nonce is big endian in the header
						#if NONCE_IN_HEADER_IS_BIG_ENDIAN
						
							// Display nonce for the previous job
							cout << hex << setw(sizeof(NonceType) * HEXADECIMAL_CHARACTER_SIZE) << setfill('0') << static_cast<uint64_t>(jobNonce[1 - currentJobIndex]) << setfill(' ') << setw(0) << dec;
							
						// Otherwise
						#else
						
							// Display nonce for the previous job in little endian format
							#if NONCE_SIZE == 1
								cout << hex << setw(sizeof(NonceType) * HEXADECIMAL_CHARACTER_SIZE) << setfill('0') << static_cast<uint64_t>(jobNonce[1 - currentJobIndex]) << setfill(' ') << setw(0) << dec;
							#elif NONCE_SIZE == 2
								cout << hex << setw(sizeof(NonceType) * HEXADECIMAL_CHARACTER_SIZE) << setfill('0') << static_cast<uint64_t>(__builtin_bswap16(jobNonce[1 - currentJobIndex])) << setfill(' ') << setw(0) << dec;
							#elif NONCE_SIZE == 4
								cout << hex << setw(sizeof(NonceType) * HEXADECIMAL_CHARACTER_SIZE) << setfill('0') << static_cast<uint64_t>(__builtin_bswap32(jobNonce[1 - currentJobIndex])) << setfill(' ') << setw(0) << dec;
							#else
								cout << hex << setw(sizeof(NonceType) * HEXADECIMAL_CHARACTER_SIZE) << setfill('0') << static_cast<uint64_t>(__builtin_bswap64(jobNonce[1 - currentJobIndex])) << setfill(' ') << setw(0) << dec;
							#endif
						#endif
						
						// Display new line
						cout << endl;
					#endif
					
					// Display message
					cout << "Nonce: " << static_cast<uint64_t>(jobNonce[1 - currentJobIndex]) << endl;
					
					// Display message
					cout << "SipHash keys: ";
					
					// Go through all SipHash keys
					for(int i = 0; i < NUMBER_OF_SIPHASH_KEYS; ++i) [[likely]] {
					
						// Display SipHash key for the previous job
						cout << "0x" << hex << setw(sizeof(uint64_t) * HEXADECIMAL_CHARACTER_SIZE) << setfill('0') << sipHashKeys[1 - currentJobIndex][i] << setfill(' ') << setw(0) << dec << ' ';
					}
					
					// Display new line
					cout << endl;
				#endif
				
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Set CPU trimming threads coarse buckets one and number of edges per coarse bucket one
					cpuTrimmingThreadsCoarseBucketsOne = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET]>(cpuBucketsBuffer->contents());
					cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(numberOfEdgesPerCpuBucketBuffer->contents());
					
				// Otherwise
				#else
				
					// Set CPU trimming threads coarse buckets one and number of edges per coarse bucket one
					cpuTrimmingThreadsCoarseBucketsOne = reinterpret_cast<uint64_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION][CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET]>(mappedCpuBucketsBuffer);
					cpuTrimmingThreadsNumberOfEdgesPerCoarseBucketOne = reinterpret_cast<uint32_t (*)[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION]>(mappedNumberOfEdgesPerCpuBucketBuffer);
				#endif
				
				// Start CPU trimming on the previous job
				startCpuTrimmingThreadsTriggerToggle = !startCpuTrimmingThreadsTriggerToggle;
				cpuTrimmingThreadsLock.unlock();
				startCpuTrimmingThreadsConditionalVariable.notify_all();
				
				// Wait for CPU trimming to finish
				cpuTrimmingThreadsLock.lock();
				cpuTrimmingThreadsFinishedConditionalVariable.wait(cpuTrimmingThreadsLock, [&cpuTrimmingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
				
					// Return if CPU trimming threads have finished
					return cpuTrimmingThreadsFinished;
				});
				
				// Check if performing CPU searching during GPU trimming
				#if CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
				
					// Start CPU searching on the previous job
					recoverEdgesParameters.solutionNodes[3] = 0;
					cpuSearchingThreadsFinished = false;
					startCpuSearchingThreadsTriggerToggle = !startCpuSearchingThreadsTriggerToggle;
					cpuSearchingThreadsLock.unlock();
					startCpuSearchingThreadsConditionalVariable.notify_all();
					
					// Wait for CPU searching to finish
					cpuSearchingThreadsLock.lock();
					cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
					
						// Return if CPU searching threads have finished
						return cpuSearchingThreadsFinished;
					});
					
					// Get CPU end time
					chrono::steady_clock::time_point cpuEndTime = chrono::steady_clock::now();
					
					// Display message
					cout << "CPU trimming and searching finished in " << static_cast<chrono::duration<double, milli>>(cpuEndTime - cpuStartTime) << endl;
					
				// Otherwise
				#else
				
					// Get CPU end time
					chrono::steady_clock::time_point cpuEndTime = chrono::steady_clock::now();
					
					// Display message
					cout << "CPU trimming finished in " << static_cast<chrono::duration<double, milli>>(cpuEndTime - cpuStartTime) << endl;
				#endif
				
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Check if waiting for GPU trimming to finish failed
					commandQueueFinishedSemaphore.acquire();
					if(gpuError) [[unlikely]] {
					
						// Display message
						cout << "Waiting for GPU trimming to finish failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Display message
					cout << "GPU trimming finished in " << static_cast<chrono::duration<double, milli>>(static_cast<chrono::nanoseconds>(static_cast<uint64_t>((gpuEndTime - gpuStartTime) * NANOSECONDS_IN_A_SECOND))) << endl;
					
				// Otherwise
				#else
				
					// Check if waiting for GPU trimming to finish failed
					cl_ulong gpuStartTime;
					cl_ulong gpuEndTime;
					if(clWaitForEvents(1, &gpuLastEvent) != CL_SUCCESS || clGetEventProfilingInfo(gpuFirstEvent, CL_PROFILING_COMMAND_QUEUED, sizeof(gpuStartTime), &gpuStartTime, nullptr) != CL_SUCCESS || clReleaseEvent(gpuFirstEvent) != CL_SUCCESS || (gpuFirstEvent = nullptr) || clGetEventProfilingInfo(gpuLastEvent, CL_PROFILING_COMMAND_END, sizeof(gpuEndTime), &gpuEndTime, nullptr) != CL_SUCCESS || clReleaseEvent(gpuLastEvent) != CL_SUCCESS || (gpuLastEvent = nullptr)) [[unlikely]] {
					
						// Display message
						cout << "Waiting for GPU trimming to finish failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Display message
					cout << "GPU trimming finished in " << static_cast<chrono::duration<double, milli>>(static_cast<chrono::nanoseconds>(gpuEndTime - gpuStartTime)) << endl;
				#endif
				
				// Get CPU start time
				cpuStartTime = chrono::steady_clock::now();
				
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Start GPU transferring on the current job
					commandQueue->commit((const MTL4::CommandBuffer *[]){transferEdgesCommandBuffer.get()}, 1, transferEdgesCommitOptions);
					
				// Otherwise
				#else
				
					// Check if starting GPU transferring on the current job failed
					if(
					
						// Enqueue unmapping CPU buckets buffer
						clEnqueueUnmapMemObject(commandQueue.get(), cpuBucketsBuffer.get(), reinterpret_cast<void *>(mappedCpuBucketsBuffer), 0, nullptr, &gpuFirstEvent) != CL_SUCCESS || (mappedCpuBucketsBuffer = nullptr) ||
						
						// Enqueue unmapping number of edges per CPU bucket buffer
						clEnqueueUnmapMemObject(commandQueue.get(), numberOfEdgesPerCpuBucketBuffer.get(), reinterpret_cast<void *>(mappedNumberOfEdgesPerCpuBucketBuffer), 0, nullptr, nullptr) != CL_SUCCESS || (mappedNumberOfEdgesPerCpuBucketBuffer = nullptr) ||
						
						// Enqueue clearing number of edges per CPU bucket buffer
						clEnqueueFillBuffer(commandQueue.get(), numberOfEdgesPerCpuBucketBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue running trim final edges and transfer edges kernel
						clEnqueueNDRangeKernel(commandQueue.get(), trimFinalEdgesAndTransferEdgesKernel.get(), 1, nullptr, (const size_t[]){static_cast<size_t>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, (const size_t[]){GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
						
						// Enqueue mapping CPU buckets buffer
						!(mappedCpuBucketsBuffer = reinterpret_cast<uint64_t *>(clEnqueueMapBuffer(commandQueue.get(), cpuBucketsBuffer.get(), CL_FALSE, CL_MAP_READ, 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_COARSE_BUCKET_ITEM_SIZE * CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, 0, nullptr, nullptr, nullptr))) ||
						
						// Enqueue mapping number of edges per CPU bucket buffer
						!(mappedNumberOfEdgesPerCpuBucketBuffer = reinterpret_cast<uint32_t *>(clEnqueueMapBuffer(commandQueue.get(), numberOfEdgesPerCpuBucketBuffer.get(), CL_FALSE, CL_MAP_READ, 0, CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * sizeof(uint32_t), 0, nullptr, &gpuLastEvent, nullptr))) ||
						
						// Flush commands to queue
						clFlush(commandQueue.get()) != CL_SUCCESS
						
					) [[unlikely]] {
					
						// Display message
						cout << "Starting GPU transferring on the current job failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
				#endif
				
				// Check if not performing CPU searching during GPU trimming
				#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
				
					// Start CPU searching on the previous job
					recoverEdgesParameters.solutionNodes[3] = 0;
					cpuSearchingThreadsFinished = false;
					startCpuSearchingThreadsTriggerToggle = !startCpuSearchingThreadsTriggerToggle;
					cpuSearchingThreadsLock.unlock();
					startCpuSearchingThreadsConditionalVariable.notify_all();
				#endif
				
				// Check if closing
				if(closing) [[unlikely]] {
				
					// Check if not performing CPU searching during GPU trimming
					#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
					
						// Wait for CPU searching to finish
						cpuSearchingThreadsLock.lock();
						cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
						
							// Return if CPU searching threads have finished
							return cpuSearchingThreadsFinished;
						});
					#endif
					
					// Check if using an Apple device
					#ifdef __APPLE__
					
						// Check if waiting for GPU transferring to finish failed
						commandQueueFinishedSemaphore.acquire();
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
						}
						
					// Otherwise
					#else
					
						// Check if waiting for GPU transferring to finish failed
						if(clWaitForEvents(1, &gpuLastEvent) != CL_SUCCESS || clReleaseEvent(gpuFirstEvent) != CL_SUCCESS || (gpuFirstEvent = nullptr) || clReleaseEvent(gpuLastEvent) != CL_SUCCESS || (gpuLastEvent = nullptr)) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
						}
					#endif
					
					// Break
					break;
				}
				
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Don't free transfer edges commit options when autorelease pool is freed
					transferEdgesCommitOptions->retain();
					
					// Free objects in autorelease pool
					autoreleasePool.reset();
					
					// Check if creating autorelease pool failed
					autoreleasePool = unique_ptr<NS::AutoreleasePool, void(*)(NS::AutoreleasePool *)>(NS::AutoreleasePool::alloc()->init(), [](NS::AutoreleasePool *autoreleasePool) __attribute__((always_inline)) noexcept {
					
						// Free autorelease pool
						__builtin_assume_dereferenceable(autoreleasePool, sizeof(*autoreleasePool));
						autoreleasePool->release();
					});
					if(!autoreleasePool) [[unlikely]] {
					
						// Check if not performing CPU searching during GPU trimming
						#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
						
							// Wait for CPU searching to finish
							cpuSearchingThreadsLock.lock();
							cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
							
								// Return if CPU searching threads have finished
								return cpuSearchingThreadsFinished;
							});
						#endif
						
						// Wait for GPU transferring to finish
						commandQueueFinishedSemaphore.acquire();
						
						// Free transfer edges commit options
						transferEdgesCommitOptions->release();
						
						// Check if waiting for GPU transferring to finish failed
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "Creating autorelease pool failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Add transfer edges commit options to autorelease pool
					transferEdgesCommitOptions->autorelease();
				#endif
				
				// Prepare to start GPU and CPU recovering on the previous job
				recoverEdgesParameters.solutionSipHashKeys = sipHashKeys[1 - currentJobIndex];
				cpuRecoveringThreadsBitmap.clear();
				__builtin_memset_inline(solutionEdges, 0, sizeof(solutionEdges));
				cpuRecoveringThreadsFinished = false;
				
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Start encoding commands into the recover edges command buffer
					recoverEdgesCommandAllocator->reset();
					recoverEdgesCommandBuffer->beginCommandBuffer(recoverEdgesCommandAllocator.get());
					
					// Check if creating command encoder for the recover edges command buffer failed
					commandEncoder = recoverEdgesCommandBuffer->computeCommandEncoder();
					if(!commandEncoder) [[unlikely]] {
					
						// Check if not performing CPU searching during GPU trimming
						#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
						
							// Wait for CPU searching to finish
							cpuSearchingThreadsLock.lock();
							cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
							
								// Return if CPU searching threads have finished
								return cpuSearchingThreadsFinished;
							});
						#endif
						
						// Check if waiting for GPU transferring to finish failed
						commandQueueFinishedSemaphore.acquire();
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "Creating command encoder for the recover edges command buffer failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Set command encoder's argument table
					commandEncoder->setArgumentTable(recoverEdgesArgumentTable.get());
					
					// Encode clearing solution edges buffer
					commandEncoder->fillBuffer(solutionEdgesBuffer.get(), NS::Range(0, solutionEdgesBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running recover edges pipeline
					commandEncoder->setComputePipelineState(recoverEdgesPipeline.get());
					commandEncoder->dispatchThreads(MTL::Size(GPU_NUMBER_OF_RECOVERING_EDGES / GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM, 1, 1), MTL::Size(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Finish encoding commands into the recover edges command buffer
					commandEncoder->endEncoding();
					recoverEdgesCommandBuffer->endCommandBuffer();
					
					// Check if creating recover edges commit options failed
					MTL4::CommitOptions *recoverEdgesCommitOptions = MTL4::CommitOptions::alloc()->init();
					if(!recoverEdgesCommitOptions) [[unlikely]] {
					
						// Check if not performing CPU searching during GPU trimming
						#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
						
							// Wait for CPU searching to finish
							cpuSearchingThreadsLock.lock();
							cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
							
								// Return if CPU searching threads have finished
								return cpuSearchingThreadsFinished;
							});
						#endif
						
						// Check if waiting for GPU transferring to finish failed
						commandQueueFinishedSemaphore.acquire();
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "Creating recover edges commit options failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Add recover edges commit options to autorelease pool
					recoverEdgesCommitOptions->autorelease();
					
					// Configure recover edges commit options
					recoverEdgesCommitOptions->addFeedbackHandler(commitOptionsFeedbackHandler);
				#endif
				
				// Check if mining to a stratum server
				#if MINE_TO_A_STRATUM_SERVER
				
					// Create submit request
					char submitRequest[sizeof("{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"submit\",\"params\":{\"edge_bits\":" TO_STRING(EDGE_BITS) ",\"height\":") - sizeof('\0') + MAX_UINT64_STRING_SIZE + sizeof(",\"job_id\":") - sizeof('\0') + MAX_UINT64_STRING_SIZE + sizeof(",\"nonce\":") - sizeof('\0') + MAX_UINT64_STRING_SIZE + sizeof(",\"pow\":[") - sizeof('\0') + (MAX_UINT32_STRING_SIZE + sizeof(',')) * SOLUTION_SIZE - sizeof(',') + sizeof("]}}\n") - sizeof('\0')] = "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"submit\",\"params\":{\"edge_bits\":" TO_STRING(EDGE_BITS) ",\"height\":";
					
					// Append previous job height to submit request
					to_chars_result appendResult = to_chars(&submitRequest[sizeof("{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"submit\",\"params\":{\"edge_bits\":" TO_STRING(EDGE_BITS) ",\"height\":") - sizeof('\0')], &submitRequest[sizeof("{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"submit\",\"params\":{\"edge_bits\":" TO_STRING(EDGE_BITS) ",\"height\":") - sizeof('\0')] + MAX_UINT64_STRING_SIZE, jobHeight[1 - currentJobIndex]);
					
					// Append start of job ID to submit request
					__builtin_memcpy(appendResult.ptr, ",\"job_id\":", sizeof(",\"job_id\":") - sizeof('\0'));
					
					// Append previous job ID to submit request
					appendResult = to_chars(appendResult.ptr + sizeof(",\"job_id\":") - sizeof('\0'), appendResult.ptr + sizeof(",\"job_id\":") - sizeof('\0') + MAX_UINT64_STRING_SIZE, jobId[1 - currentJobIndex]);
					
					// Append start of job nonce to submit request
					__builtin_memcpy(appendResult.ptr, ",\"nonce\":", sizeof(",\"nonce\":") - sizeof('\0'));
					
					// Append previous job nonce to submit request
					appendResult = to_chars(appendResult.ptr + sizeof(",\"nonce\":") - sizeof('\0'), appendResult.ptr + sizeof(",\"nonce\":") - sizeof('\0') + MAX_UINT64_STRING_SIZE, jobNonce[1 - currentJobIndex]);
					
					// Append start of job proof of work to submit request
					__builtin_memcpy(appendResult.ptr, ",\"pow\":[", sizeof(",\"pow\":[") - sizeof('\0'));
				#endif
				
				// Update next job nonce
				jobNonce[1 - currentJobIndex] = jobNonce[currentJobIndex] + 1;
				
				// Check if mining to a stratum server
				#if MINE_TO_A_STRATUM_SERVER
				
					// Check if its time to send a keep alive request
					if(chrono::steady_clock::now() - lastKeepAliveTime >= static_cast<chrono::seconds>(STRATUM_SERVER_SEND_KEEP_ALIVE_REQUEST_INTERVAL_SECONDS)) [[unlikely]] {
					
						// Create keep alive request
						const char keepAliveRequest[] = "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"keepalive\",\"params\":null}\n";
						
						// Loop until full message is sent
						size_t totalBytesSent = 0;
						do [[unlikely]] {
						
							// Check if using Windows
							#ifdef _WIN32
							
								// Send data to the stratum server
								const int bytesSent = send(socketDescriptor, &keepAliveRequest[totalBytesSent], sizeof(keepAliveRequest) - sizeof('\0') - totalBytesSent, 0);
								
							// Otherwise
							#else
							
								// Send data to the stratum server
								const ssize_t bytesSent = send(socketDescriptor, &keepAliveRequest[totalBytesSent], sizeof(keepAliveRequest) - sizeof('\0') - totalBytesSent, MSG_NOSIGNAL);
							#endif
							
							// Check if sending data to the stratum server failed
							if(bytesSent <= 0) [[unlikely]] {
							
								// Break
								break;
							}
							
							// Update total bytes sent
							totalBytesSent += bytesSent;
							
						} while(totalBytesSent != sizeof(keepAliveRequest) - sizeof('\0'));
						
						// Check if sending data to the stratum server failed
						if(totalBytesSent != sizeof(keepAliveRequest) - sizeof('\0')) [[unlikely]] {
						
							// Check if not performing CPU searching during GPU trimming
							#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
							
								// Wait for CPU searching to finish
								cpuSearchingThreadsLock.lock();
								cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
								
									// Return if CPU searching threads have finished
									return cpuSearchingThreadsFinished;
								});
							#endif
							
							// Check if using an Apple device
							#ifdef __APPLE__
							
								// Check if waiting for GPU transferring to finish
								commandQueueFinishedSemaphore.acquire();
								if(gpuError) [[unlikely]] {
								
									// Display message
									cout << "Waiting for GPU transferring to finish failed" << endl;
									
									// Set return status to failure
									returnStatus = EXIT_FAILURE;
									
									// Break
									break;
								}
								
							// Otherwise
							#else
							
								// Check if waiting for GPU transferring to finish
								if(clWaitForEvents(1, &gpuLastEvent) != CL_SUCCESS || clReleaseEvent(gpuFirstEvent) != CL_SUCCESS || (gpuFirstEvent = nullptr) || clReleaseEvent(gpuLastEvent) != CL_SUCCESS || (gpuLastEvent = nullptr)) [[unlikely]] {
								
									// Display message
									cout << "Waiting for GPU transferring to finish failed" << endl;
									
									// Set return status to failure
									returnStatus = EXIT_FAILURE;
									
									// Break
									break;
								}
							#endif
							
							// Display message
							cout << "Sending data to the stratum server failed" << endl;
							
							// Break
							break;
						}
						
						// Check if displaying stratum server messages
						#if DISPLAY_STRATUM_SERVER_MESSAGES
						
							// Check if performing CPU searching during GPU trimming
							#if CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
							
								// Display message
								cout << "Sent: " << keepAliveRequest;
								
							// Otherwise
							#else
							
								// Display message
								cpuSearchingThreadsLock.lock();
								cout << "Sent: " << keepAliveRequest;
								cpuSearchingThreadsLock.unlock();
							#endif
						#endif
						
						// Update last keep alive time
						lastKeepAliveTime = chrono::steady_clock::now();
					}
					
					// Update next job height and job ID
					jobHeight[1 - currentJobIndex] = jobHeight[currentJobIndex];
					jobId[1 - currentJobIndex] = jobId[currentJobIndex];
					
					// Loop while there's data from the stratum server to receive
					bool receiveBufferFull;
					do [[unlikely]] {
					
						// Check if using Windows
						#ifdef _WIN32
						
							// Check if getting if data from the stratum server is available failed
							pollfd pollInfo = {
								
								// Socket descriptor
								.fd = socketDescriptor,
								
								// Events
								.events = POLLIN
							};
							
							const int dataAvailable = WSAPoll(&pollInfo, 1, 0);
							if(dataAvailable <= 0) [[unlikely]] {
							
								// Set that receiving data from the stratum server failed if error isn't that no data is available
								receiveBufferFull = dataAvailable == SOCKET_ERROR;
								
								// Break
								break;
							}
							
							// Check if receiving data from the stratum server failed
							const int bytesReceived = recv(socketDescriptor, &receiveBuffer[totalBytesReceived], sizeof(receiveBuffer) - totalBytesReceived, 0);
							if(bytesReceived <= 0) [[unlikely]] {
							
								// Set that receiving data from the stratum server failed
								receiveBufferFull = true;
								
								// Break
								break;
							}
							
						// Otherwise
						#else
						
							// Check if receiving data from the stratum server failed
							const ssize_t bytesReceived = recv(socketDescriptor, &receiveBuffer[totalBytesReceived], sizeof(receiveBuffer) - totalBytesReceived, MSG_DONTWAIT);
							if(bytesReceived <= 0) [[unlikely]] {
							
								// Set that receiving data from the stratum server failed if error isn't that receiving would have blocked
								receiveBufferFull = bytesReceived == 0 || (errno != EAGAIN && errno != EWOULDBLOCK);
								
								// Break
								break;
							}
						#endif
						
						// Set full message received to if the received data contains the end of a message
						const bool fullMessageReceived = __builtin_memchr(&receiveBuffer[totalBytesReceived], '\n', bytesReceived);
						
						// Update total bytes received
						totalBytesReceived += bytesReceived;
						
						// Get if receive buffer is full
						receiveBufferFull = totalBytesReceived == sizeof(receiveBuffer);
						
						// Check if a full message was received
						if(fullMessageReceived) [[likely]] {
						
							// Go through all received messages
							char *currentMessageStart = receiveBuffer;
							char *currentMessageEnd = reinterpret_cast<char *>(__builtin_memchr(currentMessageStart, '\n', totalBytesReceived));
							
							do [[likely]] {
							
								// Make current message a string
								*currentMessageEnd = '\0';
								
								// Check if displaying stratum server messages
								#if DISPLAY_STRATUM_SERVER_MESSAGES
								
									// Check if performing CPU searching during GPU trimming
									#if CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
									
										// Display message
										cout << "Received: " << currentMessageStart << endl;
										
									// Otherwise
									#else
									
										// Display message
										cpuSearchingThreadsLock.lock();
										cout << "Received: " << currentMessageStart << endl;
										cpuSearchingThreadsLock.unlock();
									#endif
								#endif
								
								// Check if current message is a job
								if(__builtin_strstr(currentMessageStart, "\"method\":\"job\"") || __builtin_strstr(currentMessageStart, "\"method\": \"job\"")) [[likely]] {
								
									// Check if reading the job message failed
									if(!readJobMessage(currentMessageStart, jobHeader, jobHeight[1 - currentJobIndex], jobId[1 - currentJobIndex])) [[unlikely]] {
									
										// Check if performing CPU searching during GPU trimming
										#if CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
										
											// Display message
											cout << "Received invalid job from the stratum server" << endl;
											
										// Otherwise
										#else
										
											// Display message
											cpuSearchingThreadsLock.lock();
											cout << "Received invalid job from the stratum server" << endl;
											cpuSearchingThreadsLock.unlock();
										#endif
									}
									
									// Otherwise
									else [[likely]] {
									
										// Create random next job nonce
										jobNonce[1 - currentJobIndex] = randomNumberGenerator();
									}
								}
								
								// Get the start and end of the next message
								totalBytesReceived -= currentMessageEnd + sizeof('\n') - currentMessageStart;
								currentMessageStart = currentMessageEnd + sizeof('\n');
								currentMessageEnd = reinterpret_cast<char *>(__builtin_memchr(currentMessageStart, '\n', totalBytesReceived));
								
							} while(currentMessageEnd);
							
							// Remove received messages that were processed
							__builtin_memmove(receiveBuffer, currentMessageStart, totalBytesReceived);
						}
						
						// Otherwise check if receive buffer is full
						else if(receiveBufferFull) [[unlikely]] {
						
							// Break
							break;
						}
						
					} while(receiveBufferFull);
					
					// Check if receiving data from the stratum server failed
					if(receiveBufferFull) [[unlikely]] {
					
						// Check if not performing CPU searching during GPU trimming
						#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
						
							// Wait for CPU searching to finish
							cpuSearchingThreadsLock.lock();
							cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
							
								// Return if CPU searching threads have finished
								return cpuSearchingThreadsFinished;
							});
						#endif
						
						// Check if using an Apple device
						#ifdef __APPLE__
						
							// Check if waiting for GPU transferring to finish
							commandQueueFinishedSemaphore.acquire();
							if(gpuError) [[unlikely]] {
							
								// Display message
								cout << "Waiting for GPU transferring to finish failed" << endl;
								
								// Set return status to failure
								returnStatus = EXIT_FAILURE;
								
								// Break
								break;
							}
							
						// Otherwise
						#else
						
							// Check if waiting for GPU transferring to finish
							if(clWaitForEvents(1, &gpuLastEvent) != CL_SUCCESS || clReleaseEvent(gpuFirstEvent) != CL_SUCCESS || (gpuFirstEvent = nullptr) || clReleaseEvent(gpuLastEvent) != CL_SUCCESS || (gpuLastEvent = nullptr)) [[unlikely]] {
							
								// Display message
								cout << "Waiting for GPU transferring to finish failed" << endl;
								
								// Set return status to failure
								returnStatus = EXIT_FAILURE;
								
								// Break
								break;
							}
						#endif
						
						// Display message
						cout << "Receiving data from the stratum server failed" << endl;
						
						// Break
						break;
					}
				#endif
				
				// Go to next job
				currentJobIndex = graphsProcessed % 2;
				++graphsProcessed;
				
				// Get SipHash keys from current job header and job nonce
				blake2b(sipHashKeys[currentJobIndex], jobHeader, jobNonce[currentJobIndex]);
				
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Update trim edges parameters buffer
					__builtin_memcpy_inline(trimEdgesParametersBuffer->contents(), &sipHashKeys[currentJobIndex], sizeof(TrimEdgesParameters));
					
					// Start encoding commands into the trim edges command buffer
					trimEdgesCommandAllocator->reset();
					trimEdgesCommandBuffer->beginCommandBuffer(trimEdgesCommandAllocator.get());
					
					// Check if creating command encoder for the trim edges command buffer failed
					commandEncoder = trimEdgesCommandBuffer->computeCommandEncoder();
					if(!commandEncoder) [[unlikely]] {
					
						// Check if not performing CPU searching during GPU trimming
						#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
						
							// Wait for CPU searching to finish
							cpuSearchingThreadsLock.lock();
							cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
							
								// Return if CPU searching threads have finished
								return cpuSearchingThreadsFinished;
							});
						#endif
						
						// Check if waiting for GPU transferring to finish failed
						commandQueueFinishedSemaphore.acquire();
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "Creating command encoder for the trim edges command buffer failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Set command encoder's argument table
					commandEncoder->setArgumentTable(trimEdgesArgumentTable.get());
					
					// Encode clearing number of edges per coarse bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
					
					// Encode clearing number of edges per fine bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running coarse bucket sort edges pipeline
					commandEncoder->setComputePipelineState(coarseBucketSortEdgesPipeline.get());
					commandEncoder->dispatchThreads(MTL::Size(NUMBER_OF_EDGES / 2, 1, 1), MTL::Size(GPU_COARSE_BUCKET_SORT_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running update largest initial coarse bucket size pipeline
					commandEncoder->setComputePipelineState(updateLargestInitialCoarseBucketSizePipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestInitialCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running fine bucket sort initial edges pipeline
					commandEncoder->setComputePipelineState(fineBucketSortInitialEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per coarse bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running trim initial edges pipeline
					commandEncoder->setComputePipelineState(trimInitialEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_INITIAL_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Check if using max RAM for GPU trimming or using more RAM for GPU trimming
					#if GPU_TRIMMING_USE_MAX_RAM || GPU_TRIMMING_USE_MORE_RAM
					
						// Encode running update largest final coarse bucket size pipeline
						commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
						commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
						
						// Encode clearing number of edges per fine bucket buffer
						commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
						
						// Encode running fine bucket sort final edges pipeline
						commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
						commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
						
						// Encode clearing number of edges per coarse bucket buffer
						commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
						
						// Encode running trim final edges pipeline
						commandEncoder->setComputePipelineState(trimFinalEdgesPipeline.get());
						commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
						
					// Otherwise
					#else
					
						// Encode running update largest intermediate coarse bucket size pipeline
						commandEncoder->setComputePipelineState(updateLargestIntermediateCoarseBucketSizePipeline.get());
						commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestIntermediateCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
						
						// Encode clearing number of edges per fine bucket buffer
						commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
						
						// Encode running fine bucket sort intermediate edges pipeline
						commandEncoder->setComputePipelineState(fineBucketSortIntermediateEdgesPipeline.get());
						commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
						
						// Encode clearing number of edges per coarse bucket buffer
						commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
						
						// Encode running trim intermediate edges pipeline
						commandEncoder->setComputePipelineState(trimIntermediateEdgesPipeline.get());
						commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					#endif
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running update largest final coarse bucket size pipeline
					commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
					
					// Encode clearing number of edges per fine bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running fine bucket sort final edges pipeline
					commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Go through all remaining GPU trimming rounds except the last one
					for(int i = 2; i < GPU_TRIMMING_ROUNDS - 1; ++i) [[likely]] {
					
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
						
						// Encode clearing number of edges per coarse bucket buffer
						commandEncoder->fillBuffer(numberOfEdgesPerCoarseBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCoarseBucketBuffer->length()), 0);
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
						
						// Encode running trim final edges pipeline
						commandEncoder->setComputePipelineState(trimFinalEdgesPipeline.get());
						commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
						
						// Encode running update largest final coarse bucket size pipeline
						commandEncoder->setComputePipelineState(updateLargestFinalCoarseBucketSizePipeline.get());
						commandEncoder->dispatchThreadgroups(MTL::Size(1, 1, 1), MTL::Size(min(updateLargestFinalCoarseBucketSizePipeline->threadExecutionWidth(), static_cast<NS::UInteger>(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION)), 1, 1));
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageBlit, MTL4::VisibilityOptionDevice);
						
						// Encode clearing number of edges per fine bucket buffer
						commandEncoder->fillBuffer(numberOfEdgesPerFineBucketBuffer.get(), NS::Range(0, GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * sizeof(uint32_t)), 0);
						
						// Encode a barrier
						commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
						
						// Encode running fine bucket sort final edges pipeline
						commandEncoder->setComputePipelineState(fineBucketSortFinalEdgesPipeline.get());
						commandEncoder->dispatchThreadgroups(largestCoarseBucketSizeBuffer->gpuAddress(), MTL::Size(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					}
					
					// Finish encoding commands into the trim edges command buffer
					commandEncoder->endEncoding();
					trimEdgesCommandBuffer->endCommandBuffer();
					
					// Check if creating trim edges commit options failed
					trimEdgesCommitOptions = MTL4::CommitOptions::alloc()->init();
					if(!trimEdgesCommitOptions) [[unlikely]] {
					
						// Check if not performing CPU searching during GPU trimming
						#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
						
							// Wait for CPU searching to finish
							cpuSearchingThreadsLock.lock();
							cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
							
								// Return if CPU searching threads have finished
								return cpuSearchingThreadsFinished;
							});
						#endif
						
						// Check if waiting for GPU transferring to finish failed
						commandQueueFinishedSemaphore.acquire();
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "Creating trim edges commit options failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Add trim edges commit options to autorelease pool
					trimEdgesCommitOptions->autorelease();
					
					// Configure trim edges commit options
					trimEdgesCommitOptions->addFeedbackHandler(commitOptionsFeedbackHandler);
				#endif
				
				// Prepare to start CPU trimming on the previous job
				cpuTrimmingThreadsFinished = false;
				
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Start encoding commands into the transfer edges command buffer
					transferEdgesCommandAllocator[currentJobIndex]->reset();
					transferEdgesCommandBuffer->beginCommandBuffer(transferEdgesCommandAllocator[currentJobIndex].get());
					
					// Check if creating command encoder for the transfer edges command buffer failed
					commandEncoder = transferEdgesCommandBuffer->computeCommandEncoder();
					if(!commandEncoder) [[unlikely]] {
					
						// Check if not performing CPU searching during GPU trimming
						#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
						
							// Wait for CPU searching to finish
							cpuSearchingThreadsLock.lock();
							cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
							
								// Return if CPU searching threads have finished
								return cpuSearchingThreadsFinished;
							});
						#endif
						
						// Check if waiting for GPU transferring to finish failed
						commandQueueFinishedSemaphore.acquire();
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "Creating command encoder for the transfer edges command buffer failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Set command encoder's argument table
					commandEncoder->setArgumentTable(transferEdgesArgumentTable.get());
					
					// Encode clearing number of edges per CPU bucket buffer
					commandEncoder->fillBuffer(numberOfEdgesPerCpuBucketBuffer.get(), NS::Range(0, numberOfEdgesPerCpuBucketBuffer->length()), 0);
					
					// Encode a barrier
					commandEncoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageDispatch, MTL4::VisibilityOptionDevice);
					
					// Encode running trim final edges and transfer edges pipeline
					commandEncoder->setComputePipelineState(trimFinalEdgesAndTransferEdgesPipeline.get());
					commandEncoder->dispatchThreadgroups(MTL::Size(GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, 1, 1), MTL::Size(GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP, 1, 1));
					
					// Finish encoding commands into the transfer edges command buffer
					commandEncoder->endEncoding();
					transferEdgesCommandBuffer->endCommandBuffer();
					
					// Check if creating transfer edges commit options failed
					transferEdgesCommitOptions = MTL4::CommitOptions::alloc()->init();
					if(!transferEdgesCommitOptions) [[unlikely]] {
					
						// Check if not performing CPU searching during GPU trimming
						#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
						
							// Wait for CPU searching to finish
							cpuSearchingThreadsLock.lock();
							cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
							
								// Return if CPU searching threads have finished
								return cpuSearchingThreadsFinished;
							});
						#endif
						
						// Check if waiting for GPU transferring to finish failed
						commandQueueFinishedSemaphore.acquire();
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU transferring to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "Creating transfer edges commit options failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Add transfer edges commit options to autorelease pool
					transferEdgesCommitOptions->autorelease();
					
					// Configure transfer edges commit options
					transferEdgesCommitOptions->addFeedbackHandler(commitOptionsFeedbackHandler);
				#endif
				
				// Check if displaying power usage
				#if DISPLAY_POWER_USAGE
				
					// Get energy consumption after
					const pair energyConsumptionAfter = energyConsumption.getTotalEnergyConsumption();
					
					// Get GPU and CPU energy consumed
					const unsigned long long gpuEnergyConsumed = energyConsumptionAfter.first - energyConsumptionBefore.first;
					const unsigned long long cpuEnergyConsumed = energyConsumptionAfter.second - energyConsumptionBefore.second;
					
					// Update energy consumption before
					energyConsumptionBefore = energyConsumptionAfter;
					
					// Lock power usage thread lock
					powerUsageThreadLock.lock();
					
					// Check if total power used was monitored
					double powerUsed = 0;
					if(totalPowerSamples) [[likely]] {
					
						// Get power used
						powerUsed = totalPowerUsed / totalPowerSamples;
						
						// Reset total power used
						totalPowerUsed = 0;
						totalPowerSamples = 0;
					}
					
					// Unlock power usage thread lock
					powerUsageThreadLock.unlock();
				#endif
				
				// Check if not performing CPU searching during GPU trimming
				#if !CPU_PERFORM_SEARCHING_DURING_GPU_TRIMMING
				
					// Wait for CPU searching to finish
					cpuSearchingThreadsLock.lock();
					cpuSearchingThreadsFinishedConditionalVariable.wait(cpuSearchingThreadsLock, [&cpuSearchingThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
					
						// Return if CPU searching threads have finished
						return cpuSearchingThreadsFinished;
					});
					
					// Get CPU end time
					cpuEndTime = chrono::steady_clock::now();
					
					// Display message
					cout << "CPU searching and additional tasks finished in " << static_cast<chrono::duration<double, milli>>(cpuEndTime - cpuStartTime) << endl;
					
				// Otherwise
				#else
				
					// Get CPU end time
					cpuEndTime = chrono::steady_clock::now();
					
					// Display message
					cout << "CPU additional tasks finished in " << static_cast<chrono::duration<double, milli>>(cpuEndTime - cpuStartTime) << endl;
				#endif
				
				// Check if using an Apple device
				#ifdef __APPLE__
				
					// Check if wait for GPU transferring to finish failed
					commandQueueFinishedSemaphore.acquire();
					if(gpuError) [[unlikely]] {
					
						// Display message
						cout << "Waiting for GPU transferring to finish failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Display message
					cout << "GPU transferring finished in " << static_cast<chrono::duration<double, milli>>(static_cast<chrono::nanoseconds>(static_cast<uint64_t>((gpuEndTime - gpuStartTime) * NANOSECONDS_IN_A_SECOND))) << endl;
					
				// Otherwise
				#else
				
					// Check if wait for GPU transferring to finish failed
					if(clWaitForEvents(1, &gpuLastEvent) != CL_SUCCESS || clGetEventProfilingInfo(gpuFirstEvent, CL_PROFILING_COMMAND_QUEUED, sizeof(gpuStartTime), &gpuStartTime, nullptr) != CL_SUCCESS || clReleaseEvent(gpuFirstEvent) != CL_SUCCESS || (gpuFirstEvent = nullptr) || clGetEventProfilingInfo(gpuLastEvent, CL_PROFILING_COMMAND_END, sizeof(gpuEndTime), &gpuEndTime, nullptr) != CL_SUCCESS || clReleaseEvent(gpuLastEvent) != CL_SUCCESS || (gpuLastEvent = nullptr)) [[unlikely]] {
					
						// Display message
						cout << "Waiting for GPU transferring to finish failed" << endl;
						
						// Set return status to failure
						returnStatus = EXIT_FAILURE;
						
						// Break
						break;
					}
					
					// Display message
					cout << "GPU transferring finished in " << static_cast<chrono::duration<double, milli>>(static_cast<chrono::nanoseconds>(gpuEndTime - gpuStartTime)) << endl;
				#endif
				
				// Get CPU start time
				cpuStartTime = chrono::steady_clock::now();
				
				// Check if not recovering edges for every graph
				#if !RECOVER_EDGES_FOR_EVERY_GRAPH
				
					// Check if a solution was found
					if(recoverEdgesParameters.solutionNodes[3]) [[unlikely]] {
				#endif
				
					// Go through all solution node pairs
					for(int i = 0; i < SOLUTION_SIZE / 2; ++i) [[likely]] {
					
						// Get solution's node in the first partition
						const uint32_t &node = recoverEdgesParameters.solutionNodes[i * NUMBER_OF_EDGE_COMPONENTS * 2 + 1];
						
						// Add the node's pair to the list of solution node pairs first partition
						recoverEdgesParameters.solutionNodePairsFirstPartition[i] = node >> 1;
						
						// Set node's group in the recovering bitmap
						cpuRecoveringThreadsBitmap.setBit(node >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_IN_RECOVERING_BITMAP);
					}
					
					// Check if using an Apple device
					#ifdef __APPLE__
					
						// Update recover edges parameters buffer
						__builtin_memcpy_inline(recoverEdgesParametersBuffer->contents(), &recoverEdgesParameters, recoverEdgesParametersUsedSize);
						
						// Start GPU recovering on the previous job
						commandQueue->commit((const MTL4::CommandBuffer *[]){recoverEdgesCommandBuffer.get()}, 1, recoverEdgesCommitOptions);
						
					// Otherwise
					#else
					
						// Check if starting GPU recovering on the previous job failed
						if(
						
							// Enqueue updating recover edges parameters buffer
							clEnqueueWriteBuffer(commandQueue.get(), recoverEdgesParametersBuffer.get(), CL_FALSE, 0, recoverEdgesParametersUsedSize, &recoverEdgesParameters, 0, nullptr, &gpuFirstEvent) != CL_SUCCESS ||
							
							// Enqueue unmapping solution edges buffer
							clEnqueueUnmapMemObject(commandQueue.get(), solutionEdgesBuffer.get(), reinterpret_cast<void *>(mappedSolutionEdgesBuffer), 0, nullptr, nullptr) != CL_SUCCESS || (mappedSolutionEdgesBuffer = nullptr) ||
							
							// Enqueue clearing solution edges buffer
							clEnqueueFillBuffer(commandQueue.get(), solutionEdgesBuffer.get(), (const uint32_t[]){0}, sizeof(uint32_t), 0, SOLUTION_SIZE * sizeof(uint32_t), 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue running recover edges kernel
							clEnqueueNDRangeKernel(commandQueue.get(), recoverEdgesKernel.get(), 1, nullptr, (const size_t[]){GPU_NUMBER_OF_RECOVERING_EDGES / GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM}, (const size_t[]){GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP}, 0, nullptr, nullptr) != CL_SUCCESS ||
							
							// Enqueue mapping solution edges buffer
							!(mappedSolutionEdgesBuffer = reinterpret_cast<uint32_t *>(clEnqueueMapBuffer(commandQueue.get(), solutionEdgesBuffer.get(), CL_FALSE, CL_MAP_READ, 0, SOLUTION_SIZE * sizeof(uint32_t), 0, nullptr, &gpuLastEvent, nullptr))) ||
							
							// Flush commands to queue
							clFlush(commandQueue.get()) != CL_SUCCESS
							
						) [[unlikely]] {
						
							// Display message
							cout << "Starting GPU recovering on the previous job failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
					#endif
					
					// Start CPU recovering on the previous job
					startCpuRecoveringThreadsTriggerToggle = !startCpuRecoveringThreadsTriggerToggle;
					cpuRecoveringThreadsLock.unlock();
					startCpuRecoveringThreadsConditionalVariable.notify_all();
					
					// Wait for CPU recovering to finish
					cpuRecoveringThreadsLock.lock();
					cpuRecoveringThreadsFinishedConditionalVariable.wait(cpuRecoveringThreadsLock, [&cpuRecoveringThreadsFinished]() __attribute__((always_inline)) noexcept -> bool {
					
						// Return if CPU recovering threads have finished
						return cpuRecoveringThreadsFinished;
					});
					
					// Get CPU end time
					cpuEndTime = chrono::steady_clock::now();
					
					// Display message
					cout << "CPU recovering finished in " << static_cast<chrono::duration<double, milli>>(cpuEndTime - cpuStartTime) << endl;
					
					// Check if using an Apple device
					#ifdef __APPLE__
					
						// Check if waiting for GPU recovering to finish failed
						commandQueueFinishedSemaphore.acquire();
						if(gpuError) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU recovering to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "GPU recovering finished in " << static_cast<chrono::duration<double, milli>>(static_cast<chrono::nanoseconds>(static_cast<uint64_t>((gpuEndTime - gpuStartTime) * NANOSECONDS_IN_A_SECOND))) << endl;
						
						// Go through all solution edges
						for(int i = 0; i < SOLUTION_SIZE; ++i) [[likely]] {
						
							// Merge GPU's solution edges with the CPU's solution edges
							__builtin_assume_dereferenceable(solutionEdgesBuffer->contents(), SOLUTION_SIZE * sizeof(uint32_t));
							solutionEdges[i] |= reinterpret_cast<const uint32_t *>(solutionEdgesBuffer->contents())[i];
						}
						
					// Otherwise
					#else
					
						// Check if waiting for GPU recovering to finish failed
						if(clWaitForEvents(1, &gpuLastEvent) != CL_SUCCESS || clGetEventProfilingInfo(gpuFirstEvent, CL_PROFILING_COMMAND_QUEUED, sizeof(gpuStartTime), &gpuStartTime, nullptr) != CL_SUCCESS || clReleaseEvent(gpuFirstEvent) != CL_SUCCESS || (gpuFirstEvent = nullptr) || clGetEventProfilingInfo(gpuLastEvent, CL_PROFILING_COMMAND_END, sizeof(gpuEndTime), &gpuEndTime, nullptr) != CL_SUCCESS || clReleaseEvent(gpuLastEvent) != CL_SUCCESS || (gpuLastEvent = nullptr)) [[unlikely]] {
						
							// Display message
							cout << "Waiting for GPU recovering to finish failed" << endl;
							
							// Set return status to failure
							returnStatus = EXIT_FAILURE;
							
							// Break
							break;
						}
						
						// Display message
						cout << "GPU recovering finished in " << static_cast<chrono::duration<double, milli>>(static_cast<chrono::nanoseconds>(gpuEndTime - gpuStartTime)) << endl;
						
						// Go through all solution edges
						for(int i = 0; i < SOLUTION_SIZE; ++i) [[likely]] {
						
							// Merge GPU's solution edges with the CPU's solution edges
							__builtin_assume_dereferenceable(mappedSolutionEdgesBuffer, SOLUTION_SIZE * sizeof(uint32_t));
							solutionEdges[i] |= mappedSolutionEdgesBuffer[i];
						}
					#endif
					
					// Sort solution edges in ascending order
					sort(execution::unseq, solutionEdges, solutionEdges + SOLUTION_SIZE);
					
					// Check if all solution edges after the first one were recovered
					if(solutionEdges[1]) [[likely]] {
					
						// Check if the first solution edge might not have been recovered
						bool solutionIsValid = true;
						if(!solutionEdges[0]) [[unlikely]] {
						
							// Get first solution edge's nodes
							const uint64_t bothNodes = (static_cast<uint64_t>(sipHash24(recoverEdgesParameters.solutionSipHashKeys, 0) & NODE_MASK) << (sizeof(uint32_t) * BITS_IN_A_BYTE)) | (sipHash24(recoverEdgesParameters.solutionSipHashKeys, 1) & NODE_MASK);
							
							// Set that solution is valid if those nodes are part of the solution nodes
							solutionIsValid = find(execution::unseq, reinterpret_cast<const uint64_t *>(recoverEdgesParameters.solutionNodes), reinterpret_cast<const uint64_t *>(recoverEdgesParameters.solutionNodes) + SOLUTION_SIZE, bothNodes) != reinterpret_cast<const uint64_t *>(recoverEdgesParameters.solutionNodes) + SOLUTION_SIZE;
						}
						
						// Check if solution is valid
						if(solutionIsValid) [[likely]] {
						
							// Increment solutions found
							++solutionsFound;
							
							// Check if mining to a stratum server
							#if MINE_TO_A_STRATUM_SERVER
							
								// Append first solution edge to submit request
								appendResult = to_chars(appendResult.ptr + sizeof(",\"pow\":[") - sizeof('\0'), appendResult.ptr + sizeof(",\"pow\":[") - sizeof('\0') + MAX_UINT32_STRING_SIZE, solutionEdges[0]);
								
								// Go through all remaining solution edges
								for(int i = 1; i < SOLUTION_SIZE; ++i) [[likely]] {
								
									// Append comma to submit request
									*appendResult.ptr = ',';
									
									// Append solution edge to submit request
									appendResult = to_chars(appendResult.ptr + sizeof(','), appendResult.ptr + sizeof(',') + MAX_UINT32_STRING_SIZE, solutionEdges[i]);
								}
								
								// Append ending to submit request
								__builtin_memcpy(appendResult.ptr, "]}}\n", sizeof("]}}\n") - sizeof('\0'));
								
								// Get submit request's size
								const size_t submitRequestSize = appendResult.ptr + sizeof("]}}\n") - sizeof('\0') - submitRequest;
								
								// Loop until full message is sent
								size_t totalBytesSent = 0;
								do [[unlikely]] {
								
									// Check if using Windows
									#ifdef _WIN32
									
										// Send data to the stratum server
										const int bytesSent = send(socketDescriptor, &submitRequest[totalBytesSent], submitRequestSize - totalBytesSent, 0);
										
									// Otherwise
									#else
									
										// Send data to the stratum server
										const ssize_t bytesSent = send(socketDescriptor, &submitRequest[totalBytesSent], submitRequestSize - totalBytesSent, MSG_NOSIGNAL);
									#endif
									
									// Check if sending data to the stratum server failed
									if(bytesSent <= 0) [[unlikely]] {
									
										// Break
										break;
									}
									
									// Update total bytes sent
									totalBytesSent += bytesSent;
									
								} while(totalBytesSent != submitRequestSize);
								
								// Check if sending data to the stratum server failed
								if(totalBytesSent != submitRequestSize) [[unlikely]] {
								
									// Display message
									cout << "Sending data to the stratum server failed" << endl;
									
									// Break
									break;
								}
								
								// Check if displaying stratum server messages
								#if DISPLAY_STRATUM_SERVER_MESSAGES
								
									// Display message
									cout << "Sent: " << string_view(submitRequest, submitRequestSize);
								#endif
								
								// Update last keep alive time
								lastKeepAliveTime = chrono::steady_clock::now();
							#endif
						}
					}
					
					// Get CPU start time
					cpuStartTime = chrono::steady_clock::now();
					
				// Check if not recovering edges for every graph
				#if !RECOVER_EDGES_FOR_EVERY_GRAPH
				
					}
				#endif
				
				// Get graph end time
				const chrono::steady_clock::time_point graphEndTime = chrono::steady_clock::now();
				
				// Get time elapsed
				const auto timeElapsed = static_cast<chrono::duration<double>>(graphEndTime - graphStartTime).count();
				
				// Check if displaying power usage
				#if DISPLAY_POWER_USAGE
				
					// Check if CPU energy used was monitored
					if(energyConsumptionBefore.second) [[likely]] {
					
						// Display message
						cout << "CPU used " << ((cpuEnergyConsumed / timeElapsed) / NANOWATTS_IN_A_WATT) << "W of power" << endl;
					}
					
					// Check if GPU energy used was monitored
					if(energyConsumptionBefore.first) [[likely]] {
					
						// Display message
						cout << "GPU used " << ((gpuEnergyConsumed / timeElapsed) / NANOWATTS_IN_A_WATT) << "W of power" << endl;
					}
					
					// Check if power used was monitored
					if(powerUsed) [[likely]] {
					
						// Update total power used
						cout << "System used " << powerUsed << "W of power in total" << endl;
					}
				#endif
				
				// Display message
				cout << "Current mining rate is " << (1 / timeElapsed) << " g/s, overall mining rate is " << (graphsProcessed / static_cast<chrono::duration<double>>(graphEndTime - miningStartTime).count()) << " g/s, and solutions found to graphs processed ration is " << solutionsFound << '/' << graphsProcessed << endl << endl;
				
				// Get graph start time
				graphStartTime = graphEndTime;
			}
			
			// Check if stopping after a specified number of graphs
			#if STOP_AFTER_NUMBER_OF_GRAPHS != 0
			
				// Get mining end time
				const chrono::steady_clock::time_point miningEndTime = chrono::steady_clock::now();
				
				// Display message
				cout << "Processing " TO_STRING(STOP_AFTER_NUMBER_OF_GRAPHS) " graphs finished in " << static_cast<chrono::duration<double, milli>>(miningEndTime - miningStartTime) << " and " << solutionsFound << " solutions were found" << endl;
				
				// Break
				break;
			#endif
			
		} while(!closing && returnStatus == EXIT_SUCCESS);
	
	} while(false);
	
	// Close CPU threads
	closeCpuThreads = true;
	startCpuTrimmingThreadsTriggerToggle = !startCpuTrimmingThreadsTriggerToggle;
	startCpuSearchingThreadsTriggerToggle = !startCpuSearchingThreadsTriggerToggle;
	startCpuRecoveringThreadsTriggerToggle = !startCpuRecoveringThreadsTriggerToggle;
	cpuTrimmingThreadsLock.unlock();
	cpuSearchingThreadsLock.unlock();
	cpuRecoveringThreadsLock.unlock();
	startCpuTrimmingThreadsConditionalVariable.notify_all();
	startCpuSearchingThreadsConditionalVariable.notify_all();
	startCpuRecoveringThreadsConditionalVariable.notify_all();
	
	// Go through all CPU trimming threads
	__builtin_assume(numberOfCpuTrimmingThreads >= 1 && numberOfCpuTrimmingThreads <= CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION);
	for(unsigned int i = 0; i < numberOfCpuTrimmingThreads; ++i) [[likely]] {
	
		// Join CPU trimming thread
		cpuTrimmingThreads[i].join();
	}
	
	// Go through all CPU searching threads
	__builtin_assume(numberOfCpuSearchingThreads >= 1 && numberOfCpuSearchingThreads <= MAX_NUMBER_OF_CPU_SEARCHING_THREADS);
	for(unsigned int i = 0; i < numberOfCpuSearchingThreads; ++i) [[likely]] {
	
		// Join CPU searching thread
		cpuSearchingThreads[i].join();
	}
	
	// Go through all CPU recovering threads
	__builtin_assume(numberOfCpuRecoveringThreads >= 1 && numberOfCpuRecoveringThreads <= CPU_NUMBER_OF_RECOVERING_EDGES / CPU_RECOVERING_VECTOR_SCALE_FACTOR);
	for(unsigned int i = 0; i < numberOfCpuRecoveringThreads; ++i) [[likely]] {
	
		// Join CPU recovering thread
		cpuRecoveringThreads[i].join();
	}
	
	// Check if displaying power usage
	#if DISPLAY_POWER_USAGE
	
		// Check if using an Apple device
		#ifdef __APPLE__
		
			// Send signal to power usage thread to interrupt sleep
			pthread_kill(powerUsageThread.native_handle(), SIGUSR1);
		#endif
		
		// Join power usage thread
		powerUsageThread.join();
	#endif
	
	// Return return status
	return returnStatus;
}

// Check if mining to a stratum server
#if MINE_TO_A_STRATUM_SERVER

	// Read job message
	__attribute__((always_inline)) static inline bool readJobMessage(const char *__restrict__ message, uint8_t jobHeader[HEADER_SIZE_EXCLUDING_NONCE], uint64_t &__restrict__ jobHeight, uint64_t &__restrict__ jobId) noexcept {
	
		// Check if message doesn't contain a height
		const char *heightOffset = __builtin_strstr(message, "\"height\":");
		if(!heightOffset) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if message's height isn't valid
		const char *heightValue = &heightOffset[sizeof("\"height\":") - sizeof('\0') + (heightOffset[sizeof("\"height\":") - sizeof('\0')] == ' ')];
		char *end;
		errno = 0;
		const unsigned long long height = strtoull(heightValue, &end, DECIMAL_NUMBER_BASE);
		if(end == heightValue || !isdigit(heightValue[0]) || (heightValue[0] == '0' && isdigit(heightValue[sizeof('0')])) || errno || !height || height > UINT64_MAX) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if message doesn't contain an ID
		const char *idOffset = __builtin_strstr(message, "\"job_id\":");
		if(!idOffset) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if message's ID isn't valid
		const char *idValue = &idOffset[sizeof("\"job_id\":") - sizeof('\0') + (idOffset[sizeof("\"job_id\":") - sizeof('\0')] == ' ')];
		errno = 0;
		const unsigned long long id = strtoull(idValue, &end, DECIMAL_NUMBER_BASE);
		if(end == idValue || !isdigit(idValue[0]) || (idValue[0] == '0' && isdigit(idValue[sizeof('0')])) || errno || id > UINT64_MAX) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if message doesn't contain a pre-proof of work
		const char *preProofOfWorkOffset = __builtin_strstr(message, "\"pre_pow\":");
		if(!preProofOfWorkOffset) [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Check if message's pre-proof of work is invalid
		const char *preProofOfWork = &preProofOfWorkOffset[sizeof("\"pre_pow\":") - sizeof('\0') + (preProofOfWorkOffset[sizeof("\"pre_pow\":") - sizeof('\0')] == ' ') + sizeof('"')];
		if(__builtin_strspn(preProofOfWork, "0123456789ABCDEFabcdef") != HEADER_SIZE_EXCLUDING_NONCE * HEXADECIMAL_CHARACTER_SIZE || preProofOfWork[HEADER_SIZE_EXCLUDING_NONCE * HEXADECIMAL_CHARACTER_SIZE] != '"') [[unlikely]] {
		
			// Return false
			return false;
		}
		
		// Set job height to the message's height
		jobHeight = height;
		
		// Set job ID to the message's id
		jobId = id;
		
		// Go through all bytes in the header
		for(int i = 0; i < HEADER_SIZE_EXCLUDING_NONCE; ++i) [[likely]] {
		
			// Set byte to the message's pre-proof of work characters
			jobHeader[i] = (((preProofOfWork[i * HEXADECIMAL_CHARACTER_SIZE] | ('a' ^ 'A')) - (__builtin_expect(preProofOfWork[i * HEXADECIMAL_CHARACTER_SIZE] > '9', false) ? '0' + 'a' - '9' - 1 : '0')) << (BITS_IN_A_BYTE / HEXADECIMAL_CHARACTER_SIZE)) | ((preProofOfWork[i * HEXADECIMAL_CHARACTER_SIZE + 1] | ('a' ^ 'A')) - (__builtin_expect(preProofOfWork[i * HEXADECIMAL_CHARACTER_SIZE + 1] > '9', false) ? '0' + 'a' - '9' - 1 : '0'));
		}
		
		// Return true
		return true;
	}
#endif
