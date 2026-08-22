MTLSTR(R"(


// Header files
#include <metal_stdlib>

using namespace metal;


// Constants

// Bits in a byte
#define BITS_IN_A_BYTE 8

// Number of edges
#define NUMBER_OF_EDGES (static_cast<ulong>(1) << EDGE_BITS)

// Node mask
#define NODE_MASK (UINT_MAX >> (sizeof(uint) * BITS_IN_A_BYTE - EDGE_BITS))

// SipRound rotation
#define SIP_ROUND_ROTATION 21

// GPU number of coarse buckets per dimension
#define GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION (1 << GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)

// GPU number of least significant bits ignored during coarse bucket sorting
#define GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING (EDGE_BITS - GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)

// GPU number of fine buckets per dimension
#define GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION (1 << GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING)

// GPU number of least significant bits ignored during fine bucket sorting
#define GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING (EDGE_BITS - GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING - GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING)

// GPU fine bucket index mask
#define GPU_FINE_BUCKET_INDEX_MASK (GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION - 1)

// GPU bitmap size
#define GPU_BITMAP_SIZE ((1 << GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) / BITS_IN_A_BYTE)

// GPU Bitmap item mask
#define GPU_BITMAP_ITEM_MASK ((1 << GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) - 1)

// CPU number of coarse buckets per dimension
#define CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION (1 << CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)

// CPU number of least significant bits ignored during coarse bucket sorting
#define CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING (EDGE_BITS - CPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_COARSE_BUCKET_SORTING)


// Structures

// Trim edges parameters structure
struct TrimEdgesParameters {

	// SipHash keys
	ulong4 sipHashKeys;
};

// Recover edges parameters structure
struct RecoverEdgesParameters {
	
	// Solution node pairs first partition
	uint solutionNodePairsFirstPartition[SOLUTION_SIZE / 2];
	
	// Solution SipHash keys
	ulong4 solutionSipHashKeys;
	
	// Solution nodes
	ulong solutionNodes[SOLUTION_SIZE];
};


// Function prototypes

// Check if using more RAM for GPU trimming
#if GPU_TRIMMING_USE_MORE_RAM

	// Coarse bucket sort edges
	[[kernel]] void coarseBucketSortEdges(device uint2 *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const uint globalId [[thread_position_in_grid]], const uint localId [[thread_position_in_threadgroup]], const uint localSize [[threads_per_threadgroup]]);
	
// Otherwise
#else

	// Coarse bucket sort edges
	[[kernel]] void coarseBucketSortEdges(device uint *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const uint globalId [[thread_position_in_grid]], const uint localId [[thread_position_in_threadgroup]], const uint localSize [[threads_per_threadgroup]]);
#endif

// Update largest initial coarse bucket size
[[kernel]] void updateLargestInitialCoarseBucketSize(device uint &__restrict largestCoarseBucketSize [[buffer(3)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort localId [[thread_position_in_threadgroup]], const ushort localSize [[threads_per_threadgroup]]);

// Check if using more RAM for GPU trimming
#if GPU_TRIMMING_USE_MORE_RAM

	// Fine bucket sort initial edges
	[[kernel]] void fineBucketSortInitialEdges(device uint2 *__restrict fineBuckets [[buffer(4)]], device atomic_uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const uint2 *__restrict coarseBuckets [[buffer(0)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort2 localId [[thread_position_in_threadgroup]], const ushort2 groupId [[threadgroup_position_in_grid]]);
	
// Otherwise
#else

	// Fine bucket sort initial edges
	[[kernel]] void fineBucketSortInitialEdges(device uint2 *__restrict fineBuckets [[buffer(4)]], device atomic_uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const uint *__restrict coarseBuckets [[buffer(0)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const ushort2 localId [[thread_position_in_threadgroup]], const ushort2 localSize [[threads_per_threadgroup]], const uint2 groupId [[threadgroup_position_in_grid]]);
#endif

// Check if using more RAM for GPU trimming
#if GPU_TRIMMING_USE_MORE_RAM

	// Trim initial edges
	[[kernel]] void trimInitialEdges(device uint2 *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(4)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], const uint localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]);
	
// Otherwise
#else

	// Trim initial edges
	[[kernel]] void trimInitialEdges(device uint *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(4)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const ushort localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]);
#endif

// Update largest intermediate coarse bucket size
[[kernel]] void updateLargestIntermediateCoarseBucketSize(device uint &__restrict largestCoarseBucketSize [[buffer(3)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort localId [[thread_position_in_threadgroup]], const ushort localSize [[threads_per_threadgroup]]);

// Fine bucket sort intermediate edges
[[kernel]] void fineBucketSortIntermediateEdges(device uint2 *__restrict fineBuckets [[buffer(4)]], device atomic_uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const uint *__restrict coarseBuckets [[buffer(0)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const ushort2 localId [[thread_position_in_threadgroup]], const ushort2 localSize [[threads_per_threadgroup]], const uint2 groupId [[threadgroup_position_in_grid]]);

// Trim intermediate edges
[[kernel]] void trimIntermediateEdges(device uint2 *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(4)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const uint localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]);

// Update largest final coarse bucket size
[[kernel]] void updateLargestFinalCoarseBucketSize(device uint &__restrict largestCoarseBucketSize [[buffer(3)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort localId [[thread_position_in_threadgroup]], const ushort localSize [[threads_per_threadgroup]]);

// Fine bucket sort final edges
[[kernel]] void fineBucketSortFinalEdges(device uint2 *__restrict fineBuckets [[buffer(4)]], device atomic_uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const uint2 *__restrict coarseBuckets [[buffer(0)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort2 localId [[thread_position_in_threadgroup]], const ushort2 groupId [[threadgroup_position_in_grid]]);

// Trim final edges
[[kernel]] void trimFinalEdges(device uint2 *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(4)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], const uint localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]);

// Trim final edges and transfer edges
[[kernel]] void trimFinalEdgesAndTransferEdges(device uint2 *__restrict cpuBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCpuBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(2)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(3)]], const uint localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]);

// Recover edges
[[kernel]] void recoverEdges(device uint *__restrict solutionEdges [[buffer(0)]], constant const RecoverEdgesParameters &__restrict recoverEdgesParameters [[buffer(1)]], const uint globalId [[thread_position_in_grid]]);

// SipHash-2-4
static inline uint sipHash24(ulong4 states, const ulong nonce);

// SipRound
static inline void sipRound(thread ulong4 &states);

// Set bit in bitmap
static inline void setBitInBitmap(threadgroup atomic_uint *bitmap, const uint index);

// Is bit set in bitmap
static inline bool isBitSetInBitmap(threadgroup const atomic_uint *bitmap, const uint index);


// Supporting function implementation

// Check if using more RAM for GPU trimming
#if GPU_TRIMMING_USE_MORE_RAM

	// Coarse bucket sort edges
	[[kernel]] void coarseBucketSortEdges(device uint2 *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const uint globalId [[thread_position_in_grid]], const uint localId [[thread_position_in_threadgroup]], const uint localSize [[threads_per_threadgroup]]) {
	
// Otherwise
#else

	// Coarse bucket sort edges
	[[kernel]] void coarseBucketSortEdges(device uint *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const uint globalId [[thread_position_in_grid]], const uint localId [[thread_position_in_threadgroup]], const uint localSize [[threads_per_threadgroup]]) {
#endif

	// Declare local number of edges per coarse bucket
	threadgroup atomic_uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Get this work item's edges
	thread const uint &edge = globalId;
	
	// Check if using more RAM for GPU trimming
	#if GPU_TRIMMING_USE_MORE_RAM
	
		// Get edge's nodes
		__builtin_assume(edge < NUMBER_OF_EDGES / 2);
		const uint nodeOther = sipHash24(trimEdgesParameters.sipHashKeys, edge * 2 + NUMBER_OF_EDGES) & NODE_MASK;
		
		__builtin_assume(edge < NUMBER_OF_EDGES / 2);
		const uint node = sipHash24(trimEdgesParameters.sipHashKeys, edge * 2) & NODE_MASK;
		
		// Get the coarse bucket index for the edge's nodes
		const ushort coarseBucketIndexOther = nodeOther >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
		const ushort coarseBucketIndex = node >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
		
	// Otherwise
	#else
	
		// Get the coarse bucket index for the edge's nodes
		__builtin_assume(edge < NUMBER_OF_EDGES / 2);
		const ushort coarseBucketIndexOther = (sipHash24(trimEdgesParameters.sipHashKeys, edge * 2 + NUMBER_OF_EDGES) & NODE_MASK) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
		
		__builtin_assume(edge < NUMBER_OF_EDGES / 2);
		const ushort coarseBucketIndex = (sipHash24(trimEdgesParameters.sipHashKeys, edge * 2) & NODE_MASK) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
	#endif
	
	// Check if local ID is less than the number of local number of edges per coarse bucket
	if(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
	
		// Set all local number of edges per coarse bucket to zero as a work group
		atomic_store_explicit(&localNumberOfEdgesPerCoarseBucket[localId], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Get the local next edge indices in the local coarse bucket
	const ushort localNextEdgeIndexOther = atomic_fetch_add_explicit(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndexOther], 1, memory_order_relaxed);
	const ushort localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1, memory_order_relaxed);
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Go through all next edge indices as a work group
	for(ushort i = localId; i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; i += localSize) [[likely]] {
	
		// Get the next edge index in the coarse bucket
		nextEdgeIndex[i] = atomic_fetch_add_explicit(&numberOfEdgesPerCoarseBucket[i], atomic_load_explicit(&localNumberOfEdgesPerCoarseBucket[i], memory_order_relaxed), memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Get this work item's edge index
	const uint edgeIndex = nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex;
	
	// Check if the edge index is valid
	if(edgeIndex < GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) [[likely]] {
	
		// Check if using more RAM for GPU trimming
		#if GPU_TRIMMING_USE_MORE_RAM
		
			// Put this work item's edge's nodes in the coarse bucket
			__builtin_assume(edge < NUMBER_OF_EDGES / 2);
			coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + edgeIndex] = uint2(node, sipHash24(trimEdgesParameters.sipHashKeys, edge * 2 + 1) & NODE_MASK);
			
		// Otherwise
		#else
		
			// Put this work item's edge in the coarse bucket
			coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + edgeIndex] = edge;
		#endif
	}
	
	// Get this work item's other edge index
	const uint edgeIndexOther = nextEdgeIndex[coarseBucketIndexOther] + localNextEdgeIndexOther;
	
	// Check if the edge index is valid
	if(edgeIndexOther < GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) [[likely]] {
	
		// Check if using more RAM for GPU trimming
		#if GPU_TRIMMING_USE_MORE_RAM
		
			// Put this work item's other edge's nodes in the coarse bucket
			__builtin_assume(edge < NUMBER_OF_EDGES / 2);
			coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndexOther + edgeIndexOther] = uint2(nodeOther, sipHash24(trimEdgesParameters.sipHashKeys, edge * 2 + 1 + NUMBER_OF_EDGES) & NODE_MASK);
			
		// Otherwise
		#else
		
			// Put this work item's other edge in the coarse bucket
			__builtin_assume(edge < NUMBER_OF_EDGES / 2);
			coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndexOther + edgeIndexOther] = edge + NUMBER_OF_EDGES / 2;
		#endif
	}
}

// Update largest initial coarse bucket size
[[kernel]] void updateLargestInitialCoarseBucketSize(device uint &__restrict largestCoarseBucketSize [[buffer(3)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort localId [[thread_position_in_threadgroup]], const ushort localSize [[threads_per_threadgroup]]) {

	// Go through all coarse buckets as a work group
	uint currentLargestCoarseBucketSize = 1;
	for(ushort i = localId; i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; i += localSize) [[likely]] {
	
		// Update the current largest coarse bucket size
		currentLargestCoarseBucketSize = max(currentLargestCoarseBucketSize, simd_max(numberOfEdgesPerCoarseBucket[i]));
	}
	
	// Check if this is the first work item in the work group
	if(localId == 0) [[unlikely]] {
	
		// Set largest coarse bucket size based on the current largest coarse bucket size
		largestCoarseBucketSize = ((min(currentLargestCoarseBucketSize, static_cast<uint>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET)) + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP;
	}
}

// Check if using more RAM for GPU trimming
#if GPU_TRIMMING_USE_MORE_RAM

	// Fine bucket sort initial edges
	[[kernel]] void fineBucketSortInitialEdges(device uint2 *__restrict fineBuckets [[buffer(4)]], device atomic_uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const uint2 *__restrict coarseBuckets [[buffer(0)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort2 localId [[thread_position_in_threadgroup]], const ushort2 groupId [[threadgroup_position_in_grid]]) {
	
		// Declare local number of edges per fine bucket
		threadgroup atomic_uint localNumberOfEdgesPerFineBucket[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
		
		// Declare next edge index
		threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
		
		// Get this work group's coarse bucket index
		const uint coarseBucketIndex = groupId.y;
		
		// Get the number of edges in this work group's coarse bucket
		const uint numberOfEdges = min(numberOfEdgesPerCoarseBucket[coarseBucketIndex], static_cast<uint>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET));
		
		// Get this work item's coarse edge indices
		const uint coarseEdgeIndex = static_cast<uint>(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * (groupId.x * 2) + localId.x;
		const uint coarseEdgeIndexOther = static_cast<uint>(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * (groupId.x * 2 + 1) + localId.x;
		
		// Get if this work item's edges exist in the coarse bucket
		const bool edgeExists = coarseEdgeIndex < numberOfEdges;
		
		// Get if this work item's edge other exist in the coarse bucket
		const bool edgeExistsOther = coarseEdgeIndexOther < numberOfEdges;
		
		// Go through all local number of edges per fine bucket as a work group
		for(ushort i = localId.x; i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; i += GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
		
			// Set local number of edges per fine bucket to zero
			atomic_store_explicit(&localNumberOfEdgesPerFineBucket[i], 0, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Check if this work item's edge exists
		uint2 nodes;
		uint fineBucketIndex;
		ushort localNextEdgeIndex;
		uint2 nodesOther;
		ushort fineBucketIndexOther;
		ushort localNextEdgeIndexOther;
		if(edgeExists) [[likely]] {
		
			// Get this work item's edge's nodes
			nodes = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + coarseEdgeIndex];
			
			// Get the fine bucket index for the edge's node
			fineBucketIndex = (nodes.x >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			
			// Get the local next edge index in the local fine bucket
			localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerFineBucket[fineBucketIndex], 1, memory_order_relaxed);
			
			// Check if this work item's other edge exists
			if(edgeExistsOther) [[likely]] {
			
				// Get this work item's other edge's nodes
				nodesOther = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + coarseEdgeIndexOther];
				
				// Get the fine bucket index for the other edge's node
				fineBucketIndexOther = (nodesOther.x >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
				
				// Get the other local next edge index in the local fine bucket
				localNextEdgeIndexOther = atomic_fetch_add_explicit(&localNumberOfEdgesPerFineBucket[fineBucketIndexOther], 1, memory_order_relaxed);
			}
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Check if local ID is less than the number of next edge indices
		if(localId.x < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) [[likely]] {
		
			// Check if the local fine bucket isn't empty
			const uint localNumberOfEdges = atomic_load_explicit(&localNumberOfEdgesPerFineBucket[localId.x], memory_order_relaxed);
			if(localNumberOfEdges) [[likely]] {
			
				// Get all the next edge index in the fine bucket as a work group
				nextEdgeIndex[localId.x] = atomic_fetch_add_explicit(&numberOfEdgesPerFineBucket[static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + localId.x], localNumberOfEdges, memory_order_relaxed);
			}
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Get this work item's edge index
		const uint edgeIndex = nextEdgeIndex[fineBucketIndex % GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION] + localNextEdgeIndex;
		
		// Check if the edge index is valid
		if(edgeIndex < GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * edgeExists) [[likely]] {
		
			// Put this work item's edge's nodes in the fine bucket
			fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * (static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + fineBucketIndex) + edgeIndex] = nodes;
		}
		
		// Get this work item's other edge index
		const uint edgeIndexOther = nextEdgeIndex[fineBucketIndexOther % GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION] + localNextEdgeIndexOther;
		
		// Check if the edge index is valid
		if(edgeIndexOther < GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * edgeExistsOther) [[likely]] {
		
			// Put this work item's edge's nodes in the fine bucket
			fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * (static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + fineBucketIndexOther) + edgeIndexOther] = nodesOther;
		}
	}
	
// Otherwise
#else

	// Fine bucket sort initial edges
	[[kernel]] void fineBucketSortInitialEdges(device uint2 *__restrict fineBuckets [[buffer(4)]], device atomic_uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const uint *__restrict coarseBuckets [[buffer(0)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const ushort2 localId [[thread_position_in_threadgroup]], const ushort2 localSize [[threads_per_threadgroup]], const uint2 groupId [[threadgroup_position_in_grid]]) {
	
		// Declare local number of edges per fine bucket
		threadgroup atomic_uint localNumberOfEdgesPerFineBucket[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
		
		// Declare next edge index
		threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
		
		// Get this work group's coarse bucket index
		thread const uint &coarseBucketIndex = groupId.y;
		
		// Get the number of edges in this work group's coarse bucket
		const uint numberOfEdges = min(numberOfEdgesPerCoarseBucket[coarseBucketIndex], static_cast<uint>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET));
		
		// Get this work item's coarse edge indices
		const uint coarseEdgeIndex = static_cast<uint>(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * (groupId.x * 2) + localId.x;
		const uint coarseEdgeIndexOther = static_cast<uint>(GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * (groupId.x * 2 + 1) + localId.x;
		
		// Get if this work item's edges exist in the coarse bucket
		const bool edgeExists = coarseEdgeIndex < numberOfEdges;
		
		// Get this work item's edge if it exists
		const uint edge = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + coarseEdgeIndex * edgeExists];
		
		// Get if this work item's edge other exist in the coarse bucket
		const bool edgeExistsOther = coarseEdgeIndexOther < numberOfEdges;
		
		// Get this work item's other edge if it exists
		const uint edgeOther = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + coarseEdgeIndexOther * edgeExistsOther];
		
		// Go through all local number of edges per fine bucket as a work group
		for(ushort i = localId.x; i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; i += localSize.x) [[likely]] {
		
			// Set local number of edges per fine bucket to zero
			atomic_store_explicit(&localNumberOfEdgesPerFineBucket[i], 0, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Check if this work item's edge exists
		uint node;
		if(edgeExists) [[likely]] {
		
			// Get edge's node
			__builtin_assume(edge < NUMBER_OF_EDGES);
			node = sipHash24(trimEdgesParameters.sipHashKeys, (static_cast<ulong>(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2)) & NODE_MASK;
		}
		
		// Check if this work item's edge exists
		ushort fineBucketIndex;
		ushort localNextEdgeIndex;
		uint nodeOther;
		ushort fineBucketIndexOther;
		ushort localNextEdgeIndexOther;
		if(edgeExists) [[likely]] {
		
			// Get the fine bucket index for the edge's node
			fineBucketIndex = (node >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			
			// Get the local next edge index in the local fine bucket
			localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerFineBucket[fineBucketIndex], 1, memory_order_relaxed);
			
			// Check if this work item's other edge exists
			if(edgeExistsOther) [[likely]] {
			
				// Get other edge's node
				__builtin_assume(edgeOther < NUMBER_OF_EDGES);
				nodeOther = sipHash24(trimEdgesParameters.sipHashKeys, (static_cast<ulong>(edgeOther >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeOther * 2)) & NODE_MASK;
				
				// Get the fine bucket index for the other edge's node
				fineBucketIndexOther = (nodeOther >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
				
				// Get the other local next edge index in the local fine bucket
				localNextEdgeIndexOther = atomic_fetch_add_explicit(&localNumberOfEdgesPerFineBucket[fineBucketIndexOther], 1, memory_order_relaxed);
			}
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Go through all next edge indices as a work group
		for(ushort i = localId.x; i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; i += localSize.x) {
		
			// Get the next edge index in the fine bucket
			nextEdgeIndex[i] = atomic_fetch_add_explicit(&numberOfEdgesPerFineBucket[static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + i], atomic_load_explicit(&localNumberOfEdgesPerFineBucket[i], memory_order_relaxed), memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Get this work item's edge index
		const uint edgeIndex = nextEdgeIndex[fineBucketIndex % GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION] + localNextEdgeIndex;
		
		// Check if the edge index is valid
		if(edgeIndex < GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * edgeExists) [[likely]] {
		
			// Put this work item's edge and its node in the fine bucket
			fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * (static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + fineBucketIndex) + edgeIndex] = uint2(edge, node);
		}
		
		// Get this work item's other edge index
		const uint edgeIndexOther = nextEdgeIndex[fineBucketIndexOther % GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION] + localNextEdgeIndexOther;
		
		// Check if the edge index is valid
		if(edgeIndexOther < GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * edgeExistsOther) [[likely]] {
		
			// Put this work item's other edge and its node in the fine bucket
			fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * (static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + fineBucketIndexOther) + edgeIndexOther] = uint2(edgeOther, nodeOther);
		}
	}
#endif

// Check if using more RAM for GPU trimming
#if GPU_TRIMMING_USE_MORE_RAM

	// Trim initial edges
	[[kernel]] void trimInitialEdges(device uint2 *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(4)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], const uint localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]) {
	
		// Declare local number of edges per coarse bucket
		threadgroup atomic_uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
		
		// Declare next edge index
		threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
		
		// Declare bitmap
		threadgroup atomic_uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
		
		// Get this work group's fine bucket index
		const uint fineBucketIndex = groupId;
		
		// Go through all bitmap parts as a work group
		for(ushort i = localId; i < GPU_BITMAP_SIZE / sizeof(uint); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
		
			// Set bitmap part to zero
			atomic_store_explicit(&bitmap[i], 0, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Get the edges in this work group's fine bucket
		constant const uint2 *edges = &fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * fineBucketIndex];
		
		// Get the number of edges in this work group's fine bucket
		const uint numberOfEdges = min(numberOfEdgesPerFineBucket[fineBucketIndex], static_cast<uint>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET));
		
		// Go through all of this work group's edges as a work group
		for(uint i = localId; i < numberOfEdges; i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
		
			// Set edge's node in the bitmap
			setBitInBitmap(bitmap, edges[i].x & GPU_BITMAP_ITEM_MASK);
		}
		
		// Go through all next edge indices as a work group
		for(ushort i = localId; i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
		
			// Set local number of edges per coarse bucket to zero
			atomic_store_explicit(&localNumberOfEdgesPerCoarseBucket[i], 0, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Go through all of this work group's edges as a work group
		for(uint i = 0; i < numberOfEdges; i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
		
			// Get this work item's edge's nodes
			const uint2 nodes = edges[min(i + localId, numberOfEdges - 1)];
			
			// Get if this work item's edge survives by having a node pair in the bitmap
			const bool edgeSurvives = (i + localId < numberOfEdges) && isBitSetInBitmap(bitmap, (nodes.x ^ 1) & GPU_BITMAP_ITEM_MASK);
			
			// Check if this work item's edge survives
			ushort coarseBucketIndex;
			ushort localNextEdgeIndex;
			if(edgeSurvives) [[likely]] {
			
				// Get the coarse bucket index for the edge's other node
				coarseBucketIndex = nodes.y >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
				
				// Get the local next edge index in the local coarse bucket
				localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1, memory_order_relaxed);
			}
			
			// Synchronize work group
			threadgroup_barrier(mem_flags::mem_threadgroup);
			
			// Check if local ID is less than the number of next edge indices
			if(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
			
				// Check if the local coarse bucket isn't empty
				const uint localNumberOfEdges = atomic_exchange_explicit(&localNumberOfEdgesPerCoarseBucket[localId], 0, memory_order_relaxed);
				if(localNumberOfEdges) [[likely]] {
				
					// Get all the next edge index in the coarse bucket as a work group
					nextEdgeIndex[localId] = atomic_fetch_add_explicit(&numberOfEdgesPerCoarseBucket[localId], localNumberOfEdges, memory_order_relaxed);
				}
			}
			
			// Synchronize work group
			threadgroup_barrier(mem_flags::mem_threadgroup);
			
			// Check if this work item's edge survives
			if(edgeSurvives) [[likely]] {
			
				// Put this work item's edge's nodes in the coarse bucket
				coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = nodes.yx;
			}
		}
	}
	
// Otherwise
#else

	// Trim initial edges
	[[kernel]] void trimInitialEdges(device uint *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(4)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const ushort localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]) {
	
		// Declare local number of edges per coarse bucket
		threadgroup atomic_uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
		
		// Declare next edge index
		threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
		
		// Declare bitmap
		threadgroup atomic_uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
		
		// Get this work group's fine bucket index
		thread const ushort &fineBucketIndex = groupId;
		
		// Get the edges in this work group's fine bucket
		constant const uint2 *edges = &fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * fineBucketIndex];
		
		// Get the number of edges in this work group's fine bucket
		const uint numberOfEdges = min(numberOfEdgesPerFineBucket[fineBucketIndex], static_cast<uint>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET));
		
		// Go through all bitmap parts as a work group
		for(uint i = localId; i < GPU_BITMAP_SIZE / sizeof(uint); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
		
			// Set bitmap part to zero
			atomic_store_explicit(&bitmap[i], 0, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Go through all of this work group's edges as a work group
		for(uint i = localId; i < numberOfEdges; i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
		
			// Set edge's node in the bitmap
			setBitInBitmap(bitmap, edges[i].y & GPU_BITMAP_ITEM_MASK);
		}
		
		// Check if local ID is less than the number of local number of edges per coarse bucket
		if(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
		
			// Set all local number of edges per coarse bucket to zero as a work group
			atomic_store_explicit(&localNumberOfEdgesPerCoarseBucket[localId], 0, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Go through all of this work group's edges as a work group
		for(uint i = 0; i < numberOfEdges; i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
		
			// Get this work item's edge and node
			const uint2 edgeAndNode = edges[min(i + localId, numberOfEdges - 1)];
			
			// Get if this work item's edge survives by having a node pair in the bitmap
			const bool edgeSurvives = (i + localId < numberOfEdges) & isBitSetInBitmap(bitmap, (edgeAndNode.y ^ 1) & GPU_BITMAP_ITEM_MASK);
			
			// Check if this work item's edge survives
			uint coarseBucketIndex;
			ushort localNextEdgeIndex;
			if(edgeSurvives) [[likely]] {
			
				// Get the coarse bucket index for the edge's other node
				__builtin_assume(edgeAndNode.x < NUMBER_OF_EDGES);
				coarseBucketIndex = (sipHash24(trimEdgesParameters.sipHashKeys, (static_cast<ulong>(edgeAndNode.x >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeAndNode.x * 2 + 1)) & NODE_MASK) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
				
				// Get the local next edge index in the local coarse bucket
				localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1, memory_order_relaxed);
			}
			
			// Synchronize work group
			threadgroup_barrier(mem_flags::mem_threadgroup);
			
			// Check if local ID is less than the number of next edge indices
			if(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
			
				// Get all the next edge index in the coarse bucket as a work group
				nextEdgeIndex[localId] = atomic_fetch_add_explicit(&numberOfEdgesPerCoarseBucket[localId], atomic_exchange_explicit(&localNumberOfEdgesPerCoarseBucket[localId], 0, memory_order_relaxed), memory_order_relaxed);
			}
			
			// Synchronize work group
			threadgroup_barrier(mem_flags::mem_threadgroup);
			
			// Check if this work item's edge survives
			if(edgeSurvives) [[likely]] {
			
				// Put this work item's edge in the coarse bucket
				coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = edgeAndNode.x;
			}
		}
	}
#endif

// Update largest intermediate coarse bucket size
[[kernel]] void updateLargestIntermediateCoarseBucketSize(device uint &__restrict largestCoarseBucketSize [[buffer(3)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort localId [[thread_position_in_threadgroup]], const ushort localSize [[threads_per_threadgroup]]) {

	// Go through all coarse buckets as a work group
	uint currentLargestCoarseBucketSize = 1;
	for(ushort i = localId; i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; i += localSize) [[likely]] {
	
		// Update the current largest coarse bucket size
		currentLargestCoarseBucketSize = max(currentLargestCoarseBucketSize, simd_max(numberOfEdgesPerCoarseBucket[i]));
	}
	
	// Check if this is the first work item in the work group
	if(localId == 0) [[unlikely]] {
	
		// Set largest coarse bucket size based on the current largest coarse bucket size
		largestCoarseBucketSize = ((currentLargestCoarseBucketSize + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP;
	}
}

// Fine bucket sort intermediate edges
[[kernel]] void fineBucketSortIntermediateEdges(device uint2 *__restrict fineBuckets [[buffer(4)]], device atomic_uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const uint *__restrict coarseBuckets [[buffer(0)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const ushort2 localId [[thread_position_in_threadgroup]], const ushort2 localSize [[threads_per_threadgroup]], const uint2 groupId [[threadgroup_position_in_grid]]) {

	// Declare local number of edges per fine bucket
	threadgroup atomic_uint localNumberOfEdgesPerFineBucket[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
	
	// Get this work group's coarse bucket index
	thread const uint &coarseBucketIndex = groupId.y;
	
	// Get the number of edges in this work group's coarse bucket
	const uint numberOfEdges = numberOfEdgesPerCoarseBucket[coarseBucketIndex];
	
	// Get this work item's coarse edge indices
	const uint coarseEdgeIndex = static_cast<uint>(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * (groupId.x * 2) + localId.x;
	const uint coarseEdgeIndexOther = static_cast<uint>(GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * (groupId.x * 2 + 1) + localId.x;
	
	// Get if this work item's edges exist in the coarse bucket
	const bool edgeExists = coarseEdgeIndex < numberOfEdges;
	
	// Get this work item's edge if it exists
	const uint edge = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + coarseEdgeIndex * edgeExists];
	
	// Get if this work item's edge other exist in the coarse bucket
	const bool edgeExistsOther = coarseEdgeIndexOther < numberOfEdges;
	
	// Get this work item's other edge if it exists
	const uint edgeOther = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + coarseEdgeIndexOther * edgeExistsOther];
	
	// Go through all local number of edges per fine bucket as a work group
	for(ushort i = localId.x; i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; i += localSize.x) [[likely]] {
	
		// Set local number of edges per fine bucket to zero
		atomic_store_explicit(&localNumberOfEdgesPerFineBucket[i], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Check if this work item's edge exists
	uint otherNode;
	if(edgeExists) [[likely]] {
	
		// Get edge's other node
		__builtin_assume(edge < NUMBER_OF_EDGES);
		otherNode = sipHash24(trimEdgesParameters.sipHashKeys, (static_cast<ulong>(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2 + 1)) & NODE_MASK;
	}
	
	// Check if this work item's edge exists
	ushort fineBucketIndex;
	ushort localNextEdgeIndex;
	uint otherNodeOther;
	ushort fineBucketIndexOther;
	ushort localNextEdgeIndexOther;
	if(edgeExists) [[likely]] {
	
		// Get the fine bucket index for the edge's other node
		fineBucketIndex = (otherNode >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
		
		// Get the local next edge index in the local fine bucket
		localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerFineBucket[fineBucketIndex], 1, memory_order_relaxed);
		
		// Check if this work item's other edge exists
		if(edgeExistsOther) [[likely]] {
		
			// Get other edge's other node
			__builtin_assume(edgeOther < NUMBER_OF_EDGES);
			otherNodeOther = sipHash24(trimEdgesParameters.sipHashKeys, (static_cast<ulong>(edgeOther >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeOther * 2 + 1)) & NODE_MASK;
			
			// Get the fine bucket index for the other edge's other node
			fineBucketIndexOther = (otherNodeOther >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			
			// Get the other local next edge index in the local fine bucket
			localNextEdgeIndexOther = atomic_fetch_add_explicit(&localNumberOfEdgesPerFineBucket[fineBucketIndexOther], 1, memory_order_relaxed);
		}
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Go through all next edge indices as a work group
	for(ushort i = localId.x; i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; i += localSize.x) {
	
		// Get the next edge index in the fine bucket
		nextEdgeIndex[i] = atomic_fetch_add_explicit(&numberOfEdgesPerFineBucket[static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + i], atomic_load_explicit(&localNumberOfEdgesPerFineBucket[i], memory_order_relaxed), memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Check if this work item's edge exists
	if(edgeExists) [[likely]] {
	
		// Put this work item's edge and its other node in the fine bucket
		fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * (static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + fineBucketIndex) + nextEdgeIndex[fineBucketIndex] + localNextEdgeIndex] = uint2(edge, otherNode);
		
		// Check if this work item's other edge exists
		if(edgeExistsOther) [[likely]] {
		
			// Put this work item's other edge and its other node in the fine bucket
			fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * (static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + fineBucketIndexOther) + nextEdgeIndex[fineBucketIndexOther] + localNextEdgeIndexOther] = uint2(edgeOther, otherNodeOther);
		}
	}
}

// Trim intermediate edges
[[kernel]] void trimIntermediateEdges(device uint2 *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(4)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const TrimEdgesParameters &__restrict trimEdgesParameters [[buffer(2)]], const uint localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]) {

	// Declare local number of edges per coarse bucket
	threadgroup atomic_uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare bitmap
	threadgroup atomic_uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
	
	// Get this work group's fine bucket index
	thread const ushort &fineBucketIndex = groupId;
	
	// Get the edges in this work group's fine bucket
	constant const uint2 *edges = &fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * fineBucketIndex];
	
	// Get the number of edges in this work group's fine bucket
	const uint numberOfEdges = numberOfEdgesPerFineBucket[fineBucketIndex];
	
	// Go through all bitmap parts as a work group
	for(ushort i = localId; i < GPU_BITMAP_SIZE / sizeof(uint); i += GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Set bitmap part to zero
		atomic_store_explicit(&bitmap[i], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Go through all of this work group's edges as a work group
	for(uint i = localId; i < numberOfEdges; i += GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Set edge's other node in the bitmap
		setBitInBitmap(bitmap, edges[i].y & GPU_BITMAP_ITEM_MASK);
	}
	
	// Check if local ID is less than the number of local number of edges per coarse bucket
	if(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
	
		// Set all local number of edges per coarse bucket to zero as a work group
		atomic_store_explicit(&localNumberOfEdgesPerCoarseBucket[localId], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Go through all of this work group's edges as a work group
	for(uint i = 0; i < numberOfEdges; i += GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Get this work item's edge and other node
		const uint2 edgeAndOtherNode = edges[min(i + localId, numberOfEdges - 1)];
		
		// Get if this work item's edge survives by having a node pair in the bitmap
		const bool edgeSurvives = (i + localId < numberOfEdges) & isBitSetInBitmap(bitmap, (edgeAndOtherNode.y ^ 1) & GPU_BITMAP_ITEM_MASK);
		
		// Check if this work item's edge survives
		uint node;
		if(edgeSurvives) [[unlikely]] {
		
			// Get edge's node
			__builtin_assume(edgeAndOtherNode.x < NUMBER_OF_EDGES);
			node = sipHash24(trimEdgesParameters.sipHashKeys, (static_cast<ulong>(edgeAndOtherNode.x >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeAndOtherNode.x * 2)) & NODE_MASK;
		}
		
		// Check if this work item's edge survives
		ushort coarseBucketIndex;
		ushort localNextEdgeIndex;
		if(edgeSurvives) [[unlikely]] {
		
			// Get the coarse bucket index for the edge's node
			coarseBucketIndex = node >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
			
			// Get the local next edge index in the local coarse bucket
			localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Check if local ID is less than the number of next edge indices
		if(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
		
			// Check if the local coarse bucket isn't empty
			const uint localNumberOfEdges = atomic_exchange_explicit(&localNumberOfEdgesPerCoarseBucket[localId], 0, memory_order_relaxed);
			if(localNumberOfEdges) [[likely]] {
			
				// Get all the next edge index in the coarse bucket as a work group
				nextEdgeIndex[localId] = atomic_fetch_add_explicit(&numberOfEdgesPerCoarseBucket[localId], localNumberOfEdges, memory_order_relaxed);
			}
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Check if this work item's edge survives
		if(edgeSurvives) [[unlikely]] {
		
			// Put this work item's edge's nodes in the coarse bucket
			coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = uint2(node, edgeAndOtherNode.y);
		}
	}
}

// Update largest final coarse bucket size
[[kernel]] void updateLargestFinalCoarseBucketSize(device uint &__restrict largestCoarseBucketSize [[buffer(3)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort localId [[thread_position_in_threadgroup]], const ushort localSize [[threads_per_threadgroup]]) {

	// Go through all coarse buckets as a work group
	uint currentLargestCoarseBucketSize = 1;
	for(ushort i = localId; i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; i += localSize) [[likely]] {
	
		// Update the current largest coarse bucket size
		currentLargestCoarseBucketSize = max(currentLargestCoarseBucketSize, simd_max(numberOfEdgesPerCoarseBucket[i]));
	}
	
	// Check if this is the first work item in the work group
	if(localId == 0) [[unlikely]] {
	
		// Set largest coarse bucket size based on the current largest coarse bucket size
		largestCoarseBucketSize = ((currentLargestCoarseBucketSize + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP;
	}
}

// Fine bucket sort final edges
[[kernel]] void fineBucketSortFinalEdges(device uint2 *__restrict fineBuckets [[buffer(4)]], device atomic_uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], constant const uint2 *__restrict coarseBuckets [[buffer(0)]], constant const uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], const ushort2 localId [[thread_position_in_threadgroup]], const ushort2 groupId [[threadgroup_position_in_grid]]) {

	// Declare local number of edges per fine bucket
	threadgroup atomic_uint localNumberOfEdgesPerFineBucket[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
	
	// Get this work group's coarse bucket index
	const uint coarseBucketIndex = groupId.y;
	
	// Get the number of edges in this work group's coarse bucket
	const uint numberOfEdges = numberOfEdgesPerCoarseBucket[coarseBucketIndex];
	
	// Get this work item's coarse edge indices
	const uint coarseEdgeIndex = static_cast<uint>(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * (groupId.x * 2) + localId.x;
	const uint coarseEdgeIndexOther = static_cast<uint>(GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) * (groupId.x * 2 + 1) + localId.x;
	
	// Get if this work item's edges exist in the coarse bucket
	const bool edgeExists = coarseEdgeIndex < numberOfEdges;
	
	// Get if this work item's edge other exist in the coarse bucket
	const bool edgeExistsOther = coarseEdgeIndexOther < numberOfEdges;
	
	// Go through all local number of edges per fine bucket as a work group
	for(ushort i = localId.x; i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; i += GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Set local number of edges per fine bucket to zero
		atomic_store_explicit(&localNumberOfEdgesPerFineBucket[i], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Check if this work item's edge exists
	uint2 nodes;
	uint fineBucketIndex;
	ushort localNextEdgeIndex;
	uint2 nodesOther;
	ushort fineBucketIndexOther;
	ushort localNextEdgeIndexOther;
	if(edgeExists) [[likely]] {
	
		// Check if using more RAM for GPU trimming
		#if GPU_TRIMMING_USE_MORE_RAM
		
			// Get this work item's edge's nodes
			nodes = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + coarseEdgeIndex];
			
		// Otherwise
		#else
		
			// Get this work item's edge's nodes
			nodes = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + coarseEdgeIndex];
		#endif
		
		// Get the fine bucket index for the edge's node or other node
		fineBucketIndex = (nodes.x >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
		
		// Get the local next edge index in the local fine bucket
		localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerFineBucket[fineBucketIndex], 1, memory_order_relaxed);
		
		// Check if this work item's other edge exists
		if(edgeExistsOther) [[likely]] {
		
			// Check if using more RAM for GPU trimming
			#if GPU_TRIMMING_USE_MORE_RAM
			
				// Get this work item's other edge's nodes
				nodesOther = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + coarseEdgeIndexOther];
				
			// Otherwise
			#else
			
				// Get this work item's other edge's nodes
				nodesOther = coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + coarseEdgeIndexOther];
			#endif
			
			// Get the fine bucket index for the other edge's node or other node
			fineBucketIndexOther = (nodesOther.x >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			
			// Get the other local next edge index in the local fine bucket
			localNextEdgeIndexOther = atomic_fetch_add_explicit(&localNumberOfEdgesPerFineBucket[fineBucketIndexOther], 1, memory_order_relaxed);
		}
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Check if local ID is less than the number of next edge indices
	if(localId.x < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) [[likely]] {
	
		// Check if the local fine bucket isn't empty
		const uint localNumberOfEdges = atomic_load_explicit(&localNumberOfEdgesPerFineBucket[localId.x], memory_order_relaxed);
		if(localNumberOfEdges) [[likely]] {
		
			// Get all the next edge index in the fine bucket as a work group
			nextEdgeIndex[localId.x] = atomic_fetch_add_explicit(&numberOfEdgesPerFineBucket[static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + localId.x], localNumberOfEdges, memory_order_relaxed);
		}
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Check if this work item's edge exists
	if(edgeExists) [[likely]] {
	
		// Put this work item's edge's nodes in the fine bucket
		fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * (static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + fineBucketIndex) + nextEdgeIndex[fineBucketIndex] + localNextEdgeIndex] = nodes;
	}
	
	// Check if this work item's other edge exists
	if(edgeExistsOther) [[likely]] {
	
		// Put this work item's edge's nodes in the fine bucket
		fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * (static_cast<uint>(GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + fineBucketIndexOther) + nextEdgeIndex[fineBucketIndexOther] + localNextEdgeIndexOther] = nodesOther;
	}
}

// Trim final edges
[[kernel]] void trimFinalEdges(device uint2 *__restrict coarseBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCoarseBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(4)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(5)]], const uint localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]) {

	// Declare local number of edges per coarse bucket
	threadgroup atomic_uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	threadgroup uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare bitmap
	threadgroup atomic_uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
	
	// Get this work group's fine bucket index
	const uint fineBucketIndex = groupId;
	
	// Go through all bitmap parts as a work group
	for(ushort i = localId; i < GPU_BITMAP_SIZE / sizeof(uint); i += GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Set bitmap part to zero
		atomic_store_explicit(&bitmap[i], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Get the edges in this work group's fine bucket
	constant const uint2 *edges = &fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * fineBucketIndex];
	
	// Get the number of edges in this work group's fine bucket
	const uint numberOfEdges = numberOfEdgesPerFineBucket[fineBucketIndex];
	
	// Go through all of this work group's edges as a work group
	for(uint i = localId; i < numberOfEdges; i += GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Set edge's node or other node in the bitmap
		setBitInBitmap(bitmap, edges[i].x & GPU_BITMAP_ITEM_MASK);
	}
	
	// Go through all next edge indices as a work group
	for(ushort i = localId; i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION; i += GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Set local number of edges per coarse bucket to zero
		atomic_store_explicit(&localNumberOfEdgesPerCoarseBucket[i], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Go through all of this work group's edges as a work group
	for(uint i = 0; i < numberOfEdges; i += GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Get this work item's edge's nodes
		const uint2 nodes = edges[min(i + localId, numberOfEdges - 1)];
		
		// Get if this work item's edge survives by having a node pair in the bitmap
		const bool edgeSurvives = (i + localId < numberOfEdges) && isBitSetInBitmap(bitmap, (nodes.x ^ 1) & GPU_BITMAP_ITEM_MASK);
		
		// Check if this work item's edge survives
		ushort coarseBucketIndex;
		ushort localNextEdgeIndex;
		if(edgeSurvives) [[likely]] {
		
			// Get the coarse bucket index for the edge's node or other node
			coarseBucketIndex = nodes.y >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
			
			// Get the local next edge index in the local coarse bucket
			localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Check if local ID is less than the number of next edge indices
		if(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
		
			// Check if the local coarse bucket isn't empty
			const uint localNumberOfEdges = atomic_exchange_explicit(&localNumberOfEdgesPerCoarseBucket[localId], 0, memory_order_relaxed);
			if(localNumberOfEdges) [[likely]] {
			
				// Get all the next edge index in the coarse bucket as a work group
				nextEdgeIndex[localId] = atomic_fetch_add_explicit(&numberOfEdgesPerCoarseBucket[localId], localNumberOfEdges, memory_order_relaxed);
			}
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Check if this work item's edge survives
		if(edgeSurvives) [[likely]] {
		
			// Check if using more RAM for GPU trimming
			#if GPU_TRIMMING_USE_MORE_RAM
			
				// Put this work item's edge's nodes in the coarse bucket
				coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = nodes.yx;
				
			// Otherwise
			#else
			
				// Put this work item's edge's nodes in the coarse bucket
				coarseBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = nodes.yx;
			#endif
		}
	}
}

// Trim final edges and transfer edges
[[kernel]] void trimFinalEdgesAndTransferEdges(device uint2 *__restrict cpuBuckets [[buffer(0)]], device atomic_uint *__restrict numberOfEdgesPerCpuBucket [[buffer(1)]], constant const uint2 *__restrict fineBuckets [[buffer(2)]], constant const uint *__restrict numberOfEdgesPerFineBucket [[buffer(3)]], const uint localId [[thread_position_in_threadgroup]], const ushort groupId [[threadgroup_position_in_grid]]) {

	// Declare local number of edges per CPU bucket
	threadgroup atomic_uint localNumberOfEdgesPerCpuBucket[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	threadgroup uint nextEdgeIndex[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare bitmap
	threadgroup atomic_uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
	
	// Get this work group's fine bucket index
	const uint fineBucketIndex = groupId;
	
	// Get this work group's coarse bucket index
	const ushort coarseBucketIndex = fineBucketIndex >> GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING;
	
	// Go through all bitmap parts as a work group
	for(ushort i = localId; i < GPU_BITMAP_SIZE / sizeof(uint); i += GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Set bitmap part to zero
		atomic_store_explicit(&bitmap[i], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Get the edges in this work group's fine bucket
	constant const uint2 *edges = &fineBuckets[static_cast<ulong>(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET) * fineBucketIndex];
	
	// Get the number of edges in this work group's fine bucket
	const uint numberOfEdges = numberOfEdgesPerFineBucket[fineBucketIndex];
	
	// Go through all of this work group's edges as a work group
	for(uint i = localId; i < numberOfEdges; i += GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Set edge's node or other node in the bitmap
		setBitInBitmap(bitmap, edges[i].x & GPU_BITMAP_ITEM_MASK);
	}
	
	// Check if local ID is less than the number of local number of edges per CPU bucket
	if(localId < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
	
		// Set all local number of edges per CPU buckets to zero as a work group
		atomic_store_explicit(&localNumberOfEdgesPerCpuBucket[localId], 0, memory_order_relaxed);
	}
	
	// Synchronize work group
	threadgroup_barrier(mem_flags::mem_threadgroup);
	
	// Go through all of this work group's edges as a work group
	for(uint i = 0; i < numberOfEdges; i += GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) [[likely]] {
	
		// Get this work item's edge's nodes
		const uint2 nodes = edges[min(i + localId, numberOfEdges - 1)];
		
		// Get if this work item's edge survives by having a node pair in the bitmap
		const bool edgeSurvives = (i + localId < numberOfEdges) && isBitSetInBitmap(bitmap, (nodes.x ^ 1) & GPU_BITMAP_ITEM_MASK);
		
		// Check if this work item's edge survives
		ushort cpuBucketIndex;
		ushort localNextEdgeIndex;
		if(edgeSurvives) [[likely]] {
		
			// Get the CPU bucket index for the edge's node
			cpuBucketIndex = nodes.y >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
			
			// Get the local next edge index in the local CPU bucket
			localNextEdgeIndex = atomic_fetch_add_explicit(&localNumberOfEdgesPerCpuBucket[cpuBucketIndex], 1, memory_order_relaxed);
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Check if local ID is less than the number of next edge indices
		if(localId < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) [[likely]] {
		
			// Check if the local CPU bucket isn't empty
			const uint localNumberOfEdges = atomic_exchange_explicit(&localNumberOfEdgesPerCpuBucket[localId], 0, memory_order_relaxed);
			if(localNumberOfEdges) [[likely]] {
			
				// Get all the next edge index in the CPU bucket as a work group
				nextEdgeIndex[localId] = atomic_fetch_add_explicit(&numberOfEdgesPerCpuBucket[static_cast<uint>(CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + localId], localNumberOfEdges, memory_order_relaxed);
			}
		}
		
		// Synchronize work group
		threadgroup_barrier(mem_flags::mem_threadgroup);
		
		// Get this work item's edge index
		const uint edgeIndex = nextEdgeIndex[cpuBucketIndex % CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION] + localNextEdgeIndex;
		
		// Check if edge index is valid
		if(edgeIndex < CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * edgeSurvives) [[likely]] {
		
			// Put this work item's edge's nodes in the CPU bucket
			cpuBuckets[static_cast<ulong>(CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) * (static_cast<uint>(CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION) * coarseBucketIndex + cpuBucketIndex) + edgeIndex] = nodes.yx;
		}
	}
}

// Recover edges
[[kernel]] void recoverEdges(device uint *__restrict solutionEdges [[buffer(0)]], constant const RecoverEdgesParameters &__restrict recoverEdgesParameters [[buffer(1)]], const uint globalId [[thread_position_in_grid]]) {

	// Create recovered edge candidates
	uint3 recoveredEdgeCandidates[GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM];
	
	// Set number of recovered edge candidates to zero
	ushort numberOfRecoveredEdgeCandidates = 0;
	
	// Go through all of this work item's edges
	for(uint edge = globalId * GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM; edge < globalId * GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM + GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM; ++edge) [[likely]] {
	
		// Get edge's node
		__builtin_assume(edge < GPU_NUMBER_OF_RECOVERING_EDGES);
		const uint node = sipHash24(recoverEdgesParameters.solutionSipHashKeys, (static_cast<ulong>(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2)) & NODE_MASK;
		
		// Get node's pair in the first partition
		const uint nodePair = node >> 1;
		
		// Check if solution size is forty-two
		#if SOLUTION_SIZE == 42
		
			// Perform binary search for the node pair in the list of solution node pairs in the first partition
			uint currentIndex = (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2;
			currentIndex += (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 * (__builtin_expect(nodePair < recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex], false) ? -1 : (nodePair > recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex]));
			
			__builtin_assume(currentIndex >= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 - (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 && currentIndex <= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 + (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4);
			currentIndex += (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8 * (__builtin_expect(nodePair < recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex], false) ? -1 : (nodePair > recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex]));
			
			__builtin_assume(currentIndex >= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 - (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 - (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8 && currentIndex <= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 + (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 + (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8);
			currentIndex += (SOLUTION_SIZE / 2 + (16 - 1) / 2) / 16 * ((nodePair > recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex]) - (nodePair < recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex]));
			
			__builtin_assume(currentIndex >= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 - (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 - (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8 - (SOLUTION_SIZE / 2 + (16 - 1) / 2) / 16 && currentIndex <= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 + (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 + (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8 + (SOLUTION_SIZE / 2 + (16 - 1) / 2) / 16);
			currentIndex += (SOLUTION_SIZE / 2 + (32 - 1) / 2) / 32 * ((nodePair > recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex]) - (nodePair < recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex]));
			
			// Check if node pair is in the list of solution node pairs in the first partition
			__builtin_assume(currentIndex < SOLUTION_SIZE / 2);
			if(nodePair == recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex]) [[unlikely]] {
			
				// Save edge, its node, and its index in the list of recovered edge candidates
				recoveredEdgeCandidates[min(numberOfRecoveredEdgeCandidates++, static_cast<ushort>(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM - 1))] = uint3(edge, node, currentIndex);
			}
			
		// Otherwise
		#else
		
			// Go through all solution node pairs in the first partition
			for(uint currentIndex = 0; currentIndex < SOLUTION_SIZE / 2; ++currentIndex) [[likely]] {
			
				// Check if node pair is the solution node pair in the first partition
				if(nodePair == recoverEdgesParameters.solutionNodePairsFirstPartition[currentIndex]) [[unlikely]] {
				
					// Save edge, its node, and its index in the list of recovered edge candidates
					recoveredEdgeCandidates[min(numberOfRecoveredEdgeCandidates++, static_cast<ushort>(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM - 1))] = uint3(edge, node, currentIndex);
					
					// Break
					break;
				}
			}
		#endif
	}
	
	// Create local solution edges
	uint localSolutionEdges[SOLUTION_SIZE];
	
	// Go through all local solution edges
	for(ushort i = 0; i < SOLUTION_SIZE; ++i) [[likely]] {
	
		// Set local solution edge to zero
		localSolutionEdges[i] = 0;
	}
	
	// Go through all recovered edge candidates
	for(ushort i = 0; i < min(numberOfRecoveredEdgeCandidates, static_cast<ushort>(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM)); ++i) [[unlikely]] {
	
		// Get edge's other node
		__builtin_assume(recoveredEdgeCandidates[i].x < GPU_NUMBER_OF_RECOVERING_EDGES);
		const ulong bothNodes = as_type<ulong>(uint2(sipHash24(recoverEdgesParameters.solutionSipHashKeys, (static_cast<ulong>(recoveredEdgeCandidates[i].x >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (recoveredEdgeCandidates[i].x * 2 + 1)) & NODE_MASK, recoveredEdgeCandidates[i].y));
		
		// Set solution index from where the node pair was in the list of solution node pairs in the first partition
		__builtin_assume(recoveredEdgeCandidates[i].z < SOLUTION_SIZE / 2);
		const uint solutionIndex = recoveredEdgeCandidates[i].z * 2;
		
		// Set local solution edge to the edge if the edge is part of the solution
		__builtin_assume(solutionIndex < SOLUTION_SIZE - 1);
		localSolutionEdges[solutionIndex + (bothNodes == recoverEdgesParameters.solutionNodes[solutionIndex + 1])] |= recoveredEdgeCandidates[i].x * ((bothNodes == recoverEdgesParameters.solutionNodes[solutionIndex]) | (bothNodes == recoverEdgesParameters.solutionNodes[solutionIndex + 1]));
	}
	
	// Go through all local solution edges
	for(ushort i = 0; i < SOLUTION_SIZE; ++i) [[likely]] {
	
		// Check if local solution edge is set
		if(localSolutionEdges[i]) [[unlikely]] {
		
			// Set solution edge to the local solution edge
			solutionEdges[i] = localSolutionEdges[i];
		}
	}
}

// SipHash-2-4
static inline uint sipHash24(ulong4 states, const ulong nonce) {

	// Perform hash on states
	__builtin_assume(nonce < NUMBER_OF_EDGES * 2 - 1);
	states.w ^= nonce;
	sipRound(states);
	sipRound(states);
	__builtin_assume(nonce < NUMBER_OF_EDGES * 2 - 1);
	states.even ^= ulong2(nonce, 255);
	sipRound(states);
	sipRound(states);
	sipRound(states);
	states.even += states.odd;
	states.y = rotate(states.y, static_cast<ulong>(13));
	states.w = as_type<ulong>(as_type<ushort4>(states.w).wxyz);
	states.odd ^= states.even;
	states.z += states.y;
	states.odd = rotate(states.odd, ulong2(17, SIP_ROUND_ROTATION));
	
	// Return result
	return states.y ^ states.z ^ (states.z >> (sizeof(uint) * BITS_IN_A_BYTE)) ^ states.w;
}

// SipRound
static inline void sipRound(thread ulong4 &states) {

	// Perform SipRound on states
	states.even += states.odd;
	states.y = rotate(states.y, static_cast<ulong>(13));
	states.w = as_type<ulong>(as_type<ushort4>(states.w).wxyz);
	states.odd ^= states.even;
	states.x = as_type<ulong>(as_type<uint2>(states.x).yx);
	states.even += states.wy;
	states.odd = rotate(states.odd, ulong2(17, SIP_ROUND_ROTATION));
	states.odd ^= states.zx;
	states.z = as_type<ulong>(as_type<uint2>(states.z).yx);
}

// Set bit in bitmap
static inline void setBitInBitmap(threadgroup atomic_uint *bitmap, const uint index) {

	// Set bit in bitmap
	__builtin_assume(index <= GPU_BITMAP_ITEM_MASK);
	atomic_fetch_or_explicit(&bitmap[index / (sizeof(uint) * BITS_IN_A_BYTE)], static_cast<uint>(1) << (index % (sizeof(uint) * BITS_IN_A_BYTE)), memory_order_relaxed);
}

// Is bit set in bitmap
static inline bool isBitSetInBitmap(threadgroup const atomic_uint *bitmap, const uint index) {

	// Return if bit is set in bitmap
	__builtin_assume(index <= GPU_BITMAP_ITEM_MASK);
	return atomic_load_explicit(&bitmap[index / (sizeof(uint) * BITS_IN_A_BYTE)], memory_order_relaxed) & (static_cast<uint>(1) << (index % (sizeof(uint) * BITS_IN_A_BYTE)));
}


)")
