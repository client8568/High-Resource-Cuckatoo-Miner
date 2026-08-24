R"(


// Constants

// Bits in a byte
#define BITS_IN_A_BYTE 8

// Number of edges
#define NUMBER_OF_EDGES ((ulong)1 << EDGE_BITS)

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

// Check if using max RAM for GPU trimming
#if GPU_TRIMMING_USE_MAX_RAM

	// Coarse bucket sort edges
	__kernel void coarseBucketSortEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters);
	
// Otherwise
#else

	// Coarse bucket sort edges
	__kernel void coarseBucketSortEdges(__global uint *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters);
#endif

// Update largest initial coarse bucket size
__kernel void updateLargestInitialCoarseBucketSize(__global uint *restrict largestCoarseBucketSize, __global const uint *restrict numberOfEdgesPerCoarseBucket);

// Check if using max RAM for GPU trimming
#if GPU_TRIMMING_USE_MAX_RAM

	// Fine bucket sort initial edges
	__kernel void fineBucketSortInitialEdges(__global uint2 *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint2 *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict largestCoarseBucketSize);
	
// Otherwise check if using min RAM for GPU trimming
#elif GPU_TRIMMING_USE_MIN_RAM

	// Fine bucket sort initial edges
	__kernel void fineBucketSortInitialEdges(__global uint *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict largestCoarseBucketSize, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters);
	
// Otherwise
#else

	// Fine bucket sort initial edges
	__kernel void fineBucketSortInitialEdges(__global uint2 *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict largestCoarseBucketSize, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters);
#endif

// Check if using max RAM for GPU trimming
#if GPU_TRIMMING_USE_MAX_RAM

	// Trim initial edges
	__kernel void trimInitialEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket);
	
// Otherwise check if using min RAM for GPU trimming
#elif GPU_TRIMMING_USE_MIN_RAM

	// Trim initial edges
	__kernel void trimInitialEdges(__global uint *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters);
	
// Otherwise
#else

	// Trim initial edges
	__kernel void trimInitialEdges(__global uint *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters);
#endif

// Update largest intermediate coarse bucket size
__kernel void updateLargestIntermediateCoarseBucketSize(__global uint *restrict largestCoarseBucketSize, __global const uint *restrict numberOfEdgesPerCoarseBucket);

// Check if using min RAM for GPU trimming
#if GPU_TRIMMING_USE_MIN_RAM

	// Fine bucket sort intermediate edges
	__kernel void fineBucketSortIntermediateEdges(__global uint *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters, __global const uint *restrict largestCoarseBucketSize);
	
// Otherwise
#else

	// Fine bucket sort intermediate edges
	__kernel void fineBucketSortIntermediateEdges(__global uint2 *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters, __global const uint *restrict largestCoarseBucketSize);
#endif

// Check if using min RAM for GPU trimming
#if GPU_TRIMMING_USE_MIN_RAM

	// Trim intermediate edges
	__kernel void trimIntermediateEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters);
	
// Otherwise
#else

	// Trim intermediate edges
	__kernel void trimIntermediateEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters);
#endif

// Update largest final coarse bucket size
__kernel void updateLargestFinalCoarseBucketSize(__global uint *restrict largestCoarseBucketSize, __global const uint *restrict numberOfEdgesPerCoarseBucket);

// Fine bucket sort final edges
__kernel void fineBucketSortFinalEdges(__global uint2 *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint2 *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict largestCoarseBucketSize);

// Trim final edges
__kernel void trimFinalEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket);

// Trim final edges and transfer edges
__kernel void trimFinalEdgesAndTransferEdges(__global uint2 *restrict cpuBuckets, __global uint *restrict numberOfEdgesPerCpuBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket);

// Recover edges
__kernel void recoverEdges(__global uint *restrict solutionEdges, __constant const struct RecoverEdgesParameters *restrict recoverEdgesParameters);

// SipHash-2-4
static inline uint sipHash24(ulong4 states, const ulong nonce);

// SipRound
static inline void sipRound(ulong4 *states);

// Set bit in bitmap
static inline void setBitInBitmap(__local uint *bitmap, const uint index);

// Is bit set in bitmap
static inline bool isBitSetInBitmap(__local const uint *bitmap, const uint index);


// Supporting function implementation

// Check if using max RAM for GPU trimming
#if GPU_TRIMMING_USE_MAX_RAM

	// Coarse bucket sort edges
	__kernel void coarseBucketSortEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters) {
	
// Otherwise
#else

	// Coarse bucket sort edges
	__kernel void coarseBucketSortEdges(__global uint *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters) {
#endif

	// Get local ID
	const uint localId = get_local_id(0);
	
	// Get local size
	const uint localSize = get_local_size(0);
	
	// Declare local number of edges per coarse bucket
	__local uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	__local uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Get this work item's edges
	const uint edge = get_global_id(0);
	
	// Check if using max RAM for GPU trimming
	#if GPU_TRIMMING_USE_MAX_RAM
	
		// Get edge's nodes
		__builtin_assume(edge < NUMBER_OF_EDGES / 2);
		const uint nodeOther = sipHash24(trimEdgesParameters->sipHashKeys, edge * 2 + NUMBER_OF_EDGES) & NODE_MASK;
		
		__builtin_assume(edge < NUMBER_OF_EDGES / 2);
		const uint node = sipHash24(trimEdgesParameters->sipHashKeys, edge * 2) & NODE_MASK;
		
		// Get the coarse bucket index for the edge's nodes
		const ushort coarseBucketIndexOther = nodeOther >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
		const ushort coarseBucketIndex = node >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
		
	// Otherwise
	#else
	
		// Get the coarse bucket index for the edge's nodes
		__builtin_assume(edge < NUMBER_OF_EDGES / 2);
		const ushort coarseBucketIndexOther = (sipHash24(trimEdgesParameters->sipHashKeys, edge * 2 + NUMBER_OF_EDGES) & NODE_MASK) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
		
		__builtin_assume(edge < NUMBER_OF_EDGES / 2);
		const ushort coarseBucketIndex = (sipHash24(trimEdgesParameters->sipHashKeys, edge * 2) & NODE_MASK) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
	#endif
	
	// Check if local ID is less than the number of local number of edges per coarse bucket
	if(__builtin_expect(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
	
		// Set all local number of edges per coarse bucket to zero as a work group
		localNumberOfEdgesPerCoarseBucket[localId] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Get the local next edge indices in the local coarse bucket
	const ushort localNextEdgeIndexOther = atomic_add(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndexOther], 1);
	const ushort localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1);
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Go through all next edge indices as a work group
	for(ushort i = localId; __builtin_expect(i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true); i += localSize) {
	
		// Get the next edge index in the coarse bucket
		nextEdgeIndex[i] = atomic_add(&numberOfEdgesPerCoarseBucket[i], localNumberOfEdgesPerCoarseBucket[i]);
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Get this work item's edge index
	const uint edgeIndex = nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex;
	
	// Check if the edge index is valid
	if(__builtin_expect(edgeIndex < GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, true)) {
	
		// Check if using max RAM for GPU trimming
		#if GPU_TRIMMING_USE_MAX_RAM
		
			// Put this work item's edge's nodes in the coarse bucket
			__builtin_assume(edge < NUMBER_OF_EDGES / 2);
			coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + edgeIndex] = (uint2)(node, sipHash24(trimEdgesParameters->sipHashKeys, edge * 2 + 1) & NODE_MASK);
			
		// Otherwise
		#else
		
			// Put this work item's edge in the coarse bucket
			coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + edgeIndex] = edge;
		#endif
	}
	
	// Get this work item's other edge index
	const uint edgeIndexOther = nextEdgeIndex[coarseBucketIndexOther] + localNextEdgeIndexOther;
	
	// Check if the edge index is valid
	if(__builtin_expect(edgeIndexOther < GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET, true)) {
	
		// Check if using max RAM for GPU trimming
		#if GPU_TRIMMING_USE_MAX_RAM
		
			// Put this work item's other edge's nodes in the coarse bucket
			__builtin_assume(edge < NUMBER_OF_EDGES / 2);
			coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndexOther + edgeIndexOther] = (uint2)(nodeOther, sipHash24(trimEdgesParameters->sipHashKeys, edge * 2 + 1 + NUMBER_OF_EDGES) & NODE_MASK);
			
		// Otherwise
		#else
		
			// Put this work item's other edge in the coarse bucket
			__builtin_assume(edge < NUMBER_OF_EDGES / 2);
			coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndexOther + edgeIndexOther] = edge + NUMBER_OF_EDGES / 2;
		#endif
	}
}

// Update largest initial coarse bucket size
__kernel void updateLargestInitialCoarseBucketSize(__global uint *restrict largestCoarseBucketSize, __global const uint *restrict numberOfEdgesPerCoarseBucket) {

	// Go through all coarse buckets
	uint currentLargestCoarseBucketSize = 1;
	for(ushort i = 0; __builtin_expect(i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true); ++i) {
	
		// Update the current largest coarse bucket size
		currentLargestCoarseBucketSize = max(currentLargestCoarseBucketSize, numberOfEdgesPerCoarseBucket[i]);
	}
	
	// Set largest coarse bucket size based on the current largest coarse bucket size
	*largestCoarseBucketSize = ((min(currentLargestCoarseBucketSize, (uint)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET) + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP;
}

// Check if using max RAM for GPU trimming
#if GPU_TRIMMING_USE_MAX_RAM

	// Fine bucket sort initial edges
	__kernel void fineBucketSortInitialEdges(__global uint2 *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint2 *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict largestCoarseBucketSize) {
	
		// Check if work group's edges don't exist
		if(get_group_id(0) >= *largestCoarseBucketSize) {
		
			// Return
			return;
		}
		
		// Get local ID
		const ushort localId = get_local_id(0);
		
		// Get group ID
		const ushort groupId = get_group_id(0);
		
		// Declare local number of edges per fine bucket
		__local uint localNumberOfEdgesPerFineBucket[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
		
		// Declare next edge index
		__local uint nextEdgeIndex[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
		
		// Get this work group's coarse bucket index
		const uint coarseBucketIndex = get_group_id(1);
		
		// Get the number of edges in this work group's coarse bucket
		const uint numberOfEdges = min(numberOfEdgesPerCoarseBucket[coarseBucketIndex], (uint)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET);
		
		// Get this work item's coarse edge indices
		const uint coarseEdgeIndex = (uint)GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP * (groupId * 2) + localId;
		const uint coarseEdgeIndexOther = (uint)GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP * (groupId * 2 + 1) + localId;
		
		// Get if this work item's edges exist in the coarse bucket
		const bool edgeExists = coarseEdgeIndex < numberOfEdges;
		
		// Get if this work item's edge other exist in the coarse bucket
		const bool edgeExistsOther = coarseEdgeIndexOther < numberOfEdges;
		
		// Go through all local number of edges per fine bucket as a work group
		for(ushort i = localId; __builtin_expect(i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, true); i += GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
		
			// Set local number of edges per fine bucket to zero
			localNumberOfEdgesPerFineBucket[i] = 0;
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Check if this work item's edge exists
		uint2 nodes;
		uint fineBucketIndex;
		ushort localNextEdgeIndex;
		uint2 nodesOther;
		ushort fineBucketIndexOther;
		ushort localNextEdgeIndexOther;
		if(__builtin_expect(edgeExists, true)) {
		
			// Get this work item's edge's nodes
			nodes = coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + coarseEdgeIndex];
			
			// Get the fine bucket index for the edge's node
			fineBucketIndex = (nodes.x >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			
			// Get the local next edge index in the local fine bucket
			localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerFineBucket[fineBucketIndex], 1);
			
			// Check if this work item's other edge exists
			if(__builtin_expect(edgeExistsOther, true)) {
			
				// Get this work item's other edge's nodes
				nodesOther = coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + coarseEdgeIndexOther];
				
				// Get the fine bucket index for the other edge's node
				fineBucketIndexOther = (nodesOther.x >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
				
				// Get the other local next edge index in the local fine bucket
				localNextEdgeIndexOther = atomic_add(&localNumberOfEdgesPerFineBucket[fineBucketIndexOther], 1);
			}
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Check if local ID is less than the number of next edge indices
		if(__builtin_expect(localId < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, true)) {
		
			// Check if the local fine bucket isn't empty
			const uint localNumberOfEdges = localNumberOfEdgesPerFineBucket[localId];
			if(__builtin_expect(localNumberOfEdges, true)) {
			
				// Get all the next edge index in the fine bucket as a work group
				nextEdgeIndex[localId] = atomic_add(&numberOfEdgesPerFineBucket[(uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + localId], localNumberOfEdges);
			}
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Get this work item's edge index
		const uint edgeIndex = nextEdgeIndex[fineBucketIndex % GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION] + localNextEdgeIndex;
		
		// Check if the edge index is valid
		if(__builtin_expect(edgeIndex < GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * edgeExists, true)) {
		
			// Put this work item's edge's nodes in the fine bucket
			fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndex) + edgeIndex] = nodes;
		}
		
		// Get this work item's other edge index
		const uint edgeIndexOther = nextEdgeIndex[fineBucketIndexOther % GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION] + localNextEdgeIndexOther;
		
		// Check if the edge index is valid
		if(__builtin_expect(edgeIndexOther < GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * edgeExistsOther, true)) {
		
			// Put this work item's edge's nodes in the fine bucket
			fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndexOther) + edgeIndexOther] = nodesOther;
		}
	}
	
// Otherwise
#else

	// Check if using min RAM for GPU trimming
	#if GPU_TRIMMING_USE_MIN_RAM
	
		// Fine bucket sort initial edges
		__kernel void fineBucketSortInitialEdges(__global uint *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict largestCoarseBucketSize, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters) {
		
	// Otherwise
	#else
	
		// Fine bucket sort initial edges
		__kernel void fineBucketSortInitialEdges(__global uint2 *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict largestCoarseBucketSize, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters) {
	#endif
	
		// Check if work group's edges don't exist
		if(get_group_id(0) >= *largestCoarseBucketSize) {
		
			// Return
			return;
		}
		
		// Get local ID
		const ushort localId = get_local_id(0);
		
		// Get local size
		const ushort localSize = get_local_size(0);
		
		// Get group ID
		const uint groupId = get_group_id(0);
		
		// Declare local number of edges per fine bucket
		__local uint localNumberOfEdgesPerFineBucket[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
		
		// Declare next edge index
		__local uint nextEdgeIndex[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
		
		// Get this work group's coarse bucket index
		const uint coarseBucketIndex = get_group_id(1);
		
		// Get the number of edges in this work group's coarse bucket
		const uint numberOfEdges = min(numberOfEdgesPerCoarseBucket[coarseBucketIndex], (uint)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET);
		
		// Get this work item's coarse edge indices
		const uint coarseEdgeIndex = (uint)GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP * (groupId * 2) + localId;
		const uint coarseEdgeIndexOther = (uint)GPU_FINE_BUCKET_SORT_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP * (groupId * 2 + 1) + localId;
		
		// Get if this work item's edges exist in the coarse bucket
		const bool edgeExists = coarseEdgeIndex < numberOfEdges;
		
		// Get this work item's edge if it exists
		const uint edge = coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + coarseEdgeIndex * edgeExists];
		
		// Get if this work item's edge other exist in the coarse bucket
		const bool edgeExistsOther = coarseEdgeIndexOther < numberOfEdges;
		
		// Get this work item's other edge if it exists
		const uint edgeOther = coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + coarseEdgeIndexOther * edgeExistsOther];
		
		// Go through all local number of edges per fine bucket as a work group
		for(ushort i = localId; __builtin_expect(i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, true); i += localSize) {
		
			// Set local number of edges per fine bucket to zero
			localNumberOfEdgesPerFineBucket[i] = 0;
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Check if not using min RAM for GPU trimming
		#if !GPU_TRIMMING_USE_MIN_RAM
			
			// Check if this work item's edge exists
			uint node;
			if(__builtin_expect(edgeExists, true)) {
			
				// Get edge's node
				__builtin_assume(edge < NUMBER_OF_EDGES);
				node = sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2)) & NODE_MASK;
			}
		#endif
		
		// Check if this work item's edge exists
		ushort fineBucketIndex;
		ushort localNextEdgeIndex;
		#if !GPU_TRIMMING_USE_MIN_RAM
			uint nodeOther;
		#endif
		ushort fineBucketIndexOther;
		ushort localNextEdgeIndexOther;
		if(__builtin_expect(edgeExists, true)) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Get the fine bucket index for the edge's node
				__builtin_assume(edge < NUMBER_OF_EDGES);
				fineBucketIndex = (sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2)) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
				
			// Otherwise
			#else
			
				// Get the fine bucket index for the edge's node
				fineBucketIndex = (node >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			#endif
			
			// Get the local next edge index in the local fine bucket
			localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerFineBucket[fineBucketIndex], 1);
			
			// Check if this work item's other edge exists
			if(__builtin_expect(edgeExistsOther, true)) {
			
				// Check if using min RAM for GPU trimming
				#if GPU_TRIMMING_USE_MIN_RAM
				
					// Get the fine bucket index for the other edge's node
					__builtin_assume(edgeOther < NUMBER_OF_EDGES);
					fineBucketIndexOther = (sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edgeOther >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeOther * 2)) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
					
				// Otherwise
				#else
				
					// Get other edge's node
					__builtin_assume(edgeOther < NUMBER_OF_EDGES);
					nodeOther = sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edgeOther >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeOther * 2)) & NODE_MASK;
					
					// Get the fine bucket index for the other edge's node
					fineBucketIndexOther = (nodeOther >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
				#endif
				
				// Get the other local next edge index in the local fine bucket
				localNextEdgeIndexOther = atomic_add(&localNumberOfEdgesPerFineBucket[fineBucketIndexOther], 1);
			}
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Go through all next edge indices as a work group
		for(ushort i = localId; i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; i += localSize) {
		
			// Get the next edge index in the fine bucket
			nextEdgeIndex[i] = atomic_add(&numberOfEdgesPerFineBucket[(uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + i], localNumberOfEdgesPerFineBucket[i]);
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Get this work item's edge index
		const uint edgeIndex = nextEdgeIndex[fineBucketIndex % GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION] + localNextEdgeIndex;
		
		// Check if the edge index is valid
		if(__builtin_expect(edgeIndex < GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * edgeExists, true)) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Put this work item's edge in the fine bucket
				fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndex) + edgeIndex] = edge;
				
			// Otherwise
			#else
			
				// Put this work item's edge and its node in the fine bucket
				fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndex) + edgeIndex] = (uint2)(edge, node);
			#endif
		}
		
		// Get this work item's other edge index
		const uint edgeIndexOther = nextEdgeIndex[fineBucketIndexOther % GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION] + localNextEdgeIndexOther;
		
		// Check if the edge index is valid
		if(__builtin_expect(edgeIndexOther < GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * edgeExistsOther, true)) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Put this work item's other edge in the fine bucket
				fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndexOther) + edgeIndexOther] = edgeOther;
				
			// Otherwise
			#else
			
				// Put this work item's other edge and its node in the fine bucket
				fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndexOther) + edgeIndexOther] = (uint2)(edgeOther, nodeOther);
			#endif
		}
	}
#endif

// Check if using max RAM for GPU trimming
#if GPU_TRIMMING_USE_MAX_RAM

	// Trim initial edges
	__kernel void trimInitialEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket) {
	
		// Get local ID
		const uint localId = get_local_id(0);
		
		// Get group ID
		const ushort groupId = get_group_id(0);
		
		// Declare local number of edges per coarse bucket
		__local uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
		
		// Declare next edge index
		__local uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
		
		// Declare bitmap
		__local uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
		
		// Get this work group's fine bucket index
		const uint fineBucketIndex = groupId;
		
		// Go through all bitmap parts as a work group
		for(ushort i = localId; __builtin_expect(i < GPU_BITMAP_SIZE / sizeof(uint), true); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
		
			// Set bitmap part to zero
			bitmap[i] = 0;
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Get the edges in this work group's fine bucket
		__global const uint2 *edges = &fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * fineBucketIndex];
		
		// Get the number of edges in this work group's fine bucket
		const uint numberOfEdges = min(numberOfEdgesPerFineBucket[fineBucketIndex], (uint)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET);
		
		// Go through all of this work group's edges as a work group
		for(uint i = localId; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
		
			// Set edge's node in the bitmap
			setBitInBitmap(bitmap, edges[i].x & GPU_BITMAP_ITEM_MASK);
		}
		
		// Go through all next edge indices as a work group
		for(ushort i = localId; __builtin_expect(i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
		
			// Set local number of edges per coarse bucket to zero
			localNumberOfEdgesPerCoarseBucket[i] = 0;
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Go through all of this work group's edges as a work group
		for(uint i = 0; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
		
			// Get this work item's edge's nodes
			const uint2 nodes = edges[min(i + localId, numberOfEdges - 1)];
			
			// Get if this work item's edge survives by having a node pair in the bitmap
			const bool edgeSurvives = (i + localId < numberOfEdges) && isBitSetInBitmap(bitmap, (nodes.x ^ 1) & GPU_BITMAP_ITEM_MASK);
			
			// Check if this work item's edge survives
			ushort coarseBucketIndex;
			ushort localNextEdgeIndex;
			if(__builtin_expect(edgeSurvives, true)) {
			
				// Get the coarse bucket index for the edge's other node
				coarseBucketIndex = nodes.y >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
				
				// Get the local next edge index in the local coarse bucket
				localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1);
			}
			
			// Synchronize work group
			barrier(CLK_LOCAL_MEM_FENCE);
			
			// Check if local ID is less than the number of next edge indices
			if(__builtin_expect(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
			
				// Check if the local coarse bucket isn't empty
				const uint localNumberOfEdges = atomic_xchg(&localNumberOfEdgesPerCoarseBucket[localId], 0);
				if(__builtin_expect(localNumberOfEdges, true)) {
				
					// Get all the next edge index in the coarse bucket as a work group
					nextEdgeIndex[localId] = atomic_add(&numberOfEdgesPerCoarseBucket[localId], localNumberOfEdges);
				}
			}
			
			// Synchronize work group
			barrier(CLK_LOCAL_MEM_FENCE);
			
			// Check if this work item's edge survives
			if(__builtin_expect(edgeSurvives, true)) {
			
				// Put this work item's edge's nodes in the coarse bucket
				coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = nodes.yx;
			}
		}
	}
	
// Otherwise
#else

	// Check if using min RAM for GPU trimming
	#if GPU_TRIMMING_USE_MIN_RAM
	
		// Trim initial edges
		__kernel void trimInitialEdges(__global uint *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters) {
		
	// Otherwise
	#else
	
		// Trim initial edges
		__kernel void trimInitialEdges(__global uint *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters) {
	#endif
	
		// Get local ID
		const ushort localId = get_local_id(0);
		
		// Declare local number of edges per coarse bucket
		__local uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
		
		// Declare next edge index
		__local uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
		
		// Declare bitmap
		__local uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
		
		// Get this work group's fine bucket index
		const ushort fineBucketIndex = get_group_id(0);
		
		// Check if using min RAM for GPU trimming
		#if GPU_TRIMMING_USE_MIN_RAM
		
			// Get the edges in this work group's fine bucket
			__global const uint *edges = &fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * fineBucketIndex];
			
		// Otherwise
		#else
		
			// Get the edges in this work group's fine bucket
			__global const uint2 *edges = &fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * fineBucketIndex];
		#endif
		
		// Get the number of edges in this work group's fine bucket
		const uint numberOfEdges = min(numberOfEdgesPerFineBucket[fineBucketIndex], (uint)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET);
		
		// Go through all bitmap parts as a work group
		for(uint i = localId; __builtin_expect(i < GPU_BITMAP_SIZE / sizeof(uint), true); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
		
			// Set bitmap part to zero
			bitmap[i] = 0;
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Go through all of this work group's edges as a work group
		for(uint i = localId; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Set edge's node in the bitmap
				__builtin_assume(edges[i] < NUMBER_OF_EDGES);
				setBitInBitmap(bitmap, sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edges[i] >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edges[i] * 2)) & GPU_BITMAP_ITEM_MASK);
				
			// Otherwise
			#else
			
				// Set edge's node in the bitmap
				setBitInBitmap(bitmap, edges[i].y & GPU_BITMAP_ITEM_MASK);
			#endif
		}
		
		// Check if local ID is less than the number of local number of edges per coarse bucket
		if(__builtin_expect(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
		
			// Set all local number of edges per coarse bucket to zero as a work group
			localNumberOfEdgesPerCoarseBucket[localId] = 0;
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Go through all of this work group's edges as a work group
		for(uint i = 0; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_INITIAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Get this work item's edge
				const uint edge = edges[min(i + localId, numberOfEdges - 1)];
				
				// Get if this work item's edge survives by having a node pair in the bitmap
				__builtin_assume(edge < NUMBER_OF_EDGES);
				const bool edgeSurvives = (i + localId < numberOfEdges) & isBitSetInBitmap(bitmap, (sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2)) ^ 1) & GPU_BITMAP_ITEM_MASK);
				
			// Otherwise
			#else
			
				// Get this work item's edge and node
				const uint2 edgeAndNode = edges[min(i + localId, numberOfEdges - 1)];
				
				// Get if this work item's edge survives by having a node pair in the bitmap
				const bool edgeSurvives = (i + localId < numberOfEdges) & isBitSetInBitmap(bitmap, (edgeAndNode.y ^ 1) & GPU_BITMAP_ITEM_MASK);
			#endif
			
			// Check if this work item's edge survives
			uint coarseBucketIndex;
			ushort localNextEdgeIndex;
			if(__builtin_expect(edgeSurvives, true)) {
			
				// Check if using min RAM for GPU trimming
				#if GPU_TRIMMING_USE_MIN_RAM
				
					// Get the coarse bucket index for the edge's other node
					__builtin_assume(edge < NUMBER_OF_EDGES);
					coarseBucketIndex = (sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2 + 1)) & NODE_MASK) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
					
				// Otherwise
				#else
				
					// Get the coarse bucket index for the edge's other node
					__builtin_assume(edgeAndNode.x < NUMBER_OF_EDGES);
					coarseBucketIndex = (sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edgeAndNode.x >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeAndNode.x * 2 + 1)) & NODE_MASK) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
				#endif
				
				// Get the local next edge index in the local coarse bucket
				localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1);
			}
			
			// Synchronize work group
			barrier(CLK_LOCAL_MEM_FENCE);
			
			// Check if local ID is less than the number of next edge indices
			if(__builtin_expect(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
			
				// Get all the next edge index in the coarse bucket as a work group
				nextEdgeIndex[localId] = atomic_add(&numberOfEdgesPerCoarseBucket[localId], atomic_xchg(&localNumberOfEdgesPerCoarseBucket[localId], 0));
			}
			
			// Synchronize work group
			barrier(CLK_LOCAL_MEM_FENCE);
			
			// Check if this work item's edge survives
			if(__builtin_expect(edgeSurvives, true)) {
			
				// Check if using min RAM for GPU trimming
				#if GPU_TRIMMING_USE_MIN_RAM
				
					// Put this work item's edge in the coarse bucket
					coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = edge;
					
				// Otherwise
				#else
				
					// Put this work item's edge in the coarse bucket
					coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = edgeAndNode.x;
				#endif
			}
		}
	}
#endif

// Update largest intermediate coarse bucket size
__kernel void updateLargestIntermediateCoarseBucketSize(__global uint *restrict largestCoarseBucketSize, __global const uint *restrict numberOfEdgesPerCoarseBucket) {

	// Go through all coarse buckets
	uint currentLargestCoarseBucketSize = 1;
	for(ushort i = 0; __builtin_expect(i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true); ++i) {
	
		// Update the current largest coarse bucket size
		currentLargestCoarseBucketSize = max(currentLargestCoarseBucketSize, numberOfEdgesPerCoarseBucket[i]);
	}
	
	// Set largest coarse bucket size based on the current largest coarse bucket size
	*largestCoarseBucketSize = ((currentLargestCoarseBucketSize + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP;
}

// Check if using min RAM for GPU trimming
#if GPU_TRIMMING_USE_MIN_RAM

	// Fine bucket sort intermediate edges
	__kernel void fineBucketSortIntermediateEdges(__global uint *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters, __global const uint *restrict largestCoarseBucketSize) {
	
// Otherwise
#else

	// Fine bucket sort intermediate edges
	__kernel void fineBucketSortIntermediateEdges(__global uint2 *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters, __global const uint *restrict largestCoarseBucketSize) {
#endif

	// Check if work group's edges don't exist
	if(get_group_id(0) >= *largestCoarseBucketSize) {
	
		// Return
		return;
	}
	
	// Get local ID
	const ushort localId = get_local_id(0);
	
	// Get local size
	const ushort localSize = get_local_size(0);
	
	// Get group ID
	const uint groupId = get_group_id(0);
	
	// Declare local number of edges per fine bucket
	__local uint localNumberOfEdgesPerFineBucket[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	__local uint nextEdgeIndex[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
	
	// Get this work group's coarse bucket index
	const uint coarseBucketIndex = get_group_id(1);
	
	// Get the number of edges in this work group's coarse bucket
	const uint numberOfEdges = numberOfEdgesPerCoarseBucket[coarseBucketIndex];
	
	// Get this work item's coarse edge indices
	const uint coarseEdgeIndex = (uint)GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP * (groupId * 2) + localId;
	const uint coarseEdgeIndexOther = (uint)GPU_FINE_BUCKET_SORT_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP * (groupId * 2 + 1) + localId;
	
	// Get if this work item's edges exist in the coarse bucket
	const bool edgeExists = coarseEdgeIndex < numberOfEdges;
	
	// Get this work item's edge if it exists
	const uint edge = coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + coarseEdgeIndex * edgeExists];
	
	// Get if this work item's edge other exist in the coarse bucket
	const bool edgeExistsOther = coarseEdgeIndexOther < numberOfEdges;
	
	// Get this work item's other edge if it exists
	const uint edgeOther = coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + coarseEdgeIndexOther * edgeExistsOther];
	
	// Go through all local number of edges per fine bucket as a work group
	for(ushort i = localId; __builtin_expect(i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, true); i += localSize) {
	
		// Set local number of edges per fine bucket to zero
		localNumberOfEdgesPerFineBucket[i] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Check if not using min RAM for GPU trimming
	#if !GPU_TRIMMING_USE_MIN_RAM
	
		// Check if this work item's edge exists
		uint otherNode;
		if(__builtin_expect(edgeExists, true)) {
		
			// Get edge's other node
			__builtin_assume(edge < NUMBER_OF_EDGES);
			otherNode = sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2 + 1)) & NODE_MASK;
		}
	#endif
	
	// Check if this work item's edge exists
	ushort fineBucketIndex;
	ushort localNextEdgeIndex;
	#if !GPU_TRIMMING_USE_MIN_RAM
		uint otherNodeOther;
	#endif
	ushort fineBucketIndexOther;
	ushort localNextEdgeIndexOther;
	if(__builtin_expect(edgeExists, true)) {
	
		// Check if using min RAM for GPU trimming
		#if GPU_TRIMMING_USE_MIN_RAM
		
			// Get the fine bucket index for the edge's other node
			__builtin_assume(edge < NUMBER_OF_EDGES);
			fineBucketIndex = (sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2 + 1)) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			
		// Otherwise
		#else
		
			// Get the fine bucket index for the edge's other node
			fineBucketIndex = (otherNode >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
		#endif
		
		// Get the local next edge index in the local fine bucket
		localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerFineBucket[fineBucketIndex], 1);
		
		// Check if this work item's other edge exists
		if(__builtin_expect(edgeExistsOther, true)) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Get the fine bucket index for the other edge's other node
				__builtin_assume(edgeOther < NUMBER_OF_EDGES);
				fineBucketIndexOther = (sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edgeOther >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeOther * 2 + 1)) >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
				
			// Otherwise
			#else
			
				// Get other edge's other node
				__builtin_assume(edgeOther < NUMBER_OF_EDGES);
				otherNodeOther = sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edgeOther >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeOther * 2 + 1)) & NODE_MASK;
				
				// Get the fine bucket index for the other edge's other node
				fineBucketIndexOther = (otherNodeOther >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			#endif
			
			// Get the other local next edge index in the local fine bucket
			localNextEdgeIndexOther = atomic_add(&localNumberOfEdgesPerFineBucket[fineBucketIndexOther], 1);
		}
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Go through all next edge indices as a work group
	for(ushort i = localId; i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION; i += localSize) {
	
		// Get the next edge index in the fine bucket
		nextEdgeIndex[i] = atomic_add(&numberOfEdgesPerFineBucket[(uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + i], localNumberOfEdgesPerFineBucket[i]);
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Check if this work item's edge exists
	if(__builtin_expect(edgeExists, true)) {
	
		// Check if using min RAM for GPU trimming
		#if GPU_TRIMMING_USE_MIN_RAM
		
			// Put this work item's edge in the fine bucket
			fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndex) + nextEdgeIndex[fineBucketIndex] + localNextEdgeIndex] = edge;
			
		// Otherwise
		#else
		
			// Put this work item's edge and its other node in the fine bucket
			fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndex) + nextEdgeIndex[fineBucketIndex] + localNextEdgeIndex] = (uint2)(edge, otherNode);
		#endif
		
		// Check if this work item's other edge exists
		if(__builtin_expect(edgeExistsOther, true)) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Put this work item's other edge in the fine bucket
				fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndexOther) + nextEdgeIndex[fineBucketIndexOther] + localNextEdgeIndexOther] = edgeOther;
				
			// Otherwise
			#else
			
				// Put this work item's other edge and its other node in the fine bucket
				fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndexOther) + nextEdgeIndex[fineBucketIndexOther] + localNextEdgeIndexOther] = (uint2)(edgeOther, otherNodeOther);
			#endif
		}
	}
}

// Check if using min RAM for GPU trimming
#if GPU_TRIMMING_USE_MIN_RAM

	// Trim intermediate edges
	__kernel void trimIntermediateEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters) {
	
// Otherwise
#else

	// Trim intermediate edges
	__kernel void trimIntermediateEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket, __constant const struct TrimEdgesParameters *restrict trimEdgesParameters) {
#endif

	// Get local ID
	const uint localId = get_local_id(0);
	
	// Declare local number of edges per coarse bucket
	__local uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	__local uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare bitmap
	__local uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
	
	// Get this work group's fine bucket index
	const ushort fineBucketIndex = get_group_id(0);
	
	// Check if using min RAM for GPU trimming
	#if GPU_TRIMMING_USE_MIN_RAM
	
		// Get the edges in this work group's fine bucket
		__global const uint *edges = &fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * fineBucketIndex];
		
	// Otherwise
	#else
	
		// Get the edges in this work group's fine bucket
		__global const uint2 *edges = &fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * fineBucketIndex];
	#endif
	
	// Get the number of edges in this work group's fine bucket
	const uint numberOfEdges = numberOfEdgesPerFineBucket[fineBucketIndex];
	
	// Go through all bitmap parts as a work group
	for(ushort i = localId; __builtin_expect(i < GPU_BITMAP_SIZE / sizeof(uint), true); i += GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Set bitmap part to zero
		bitmap[i] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Go through all of this work group's edges as a work group
	for(uint i = localId; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Check if using min RAM for GPU trimming
		#if GPU_TRIMMING_USE_MIN_RAM
		
			// Set edge's other node in the bitmap
			__builtin_assume(edges[i] < NUMBER_OF_EDGES);
			setBitInBitmap(bitmap, sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edges[i] >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edges[i] * 2 + 1)) & GPU_BITMAP_ITEM_MASK);
			
		// Otherwise
		#else
		
			// Set edge's other node in the bitmap
			setBitInBitmap(bitmap, edges[i].y & GPU_BITMAP_ITEM_MASK);
		#endif
	}
	
	// Check if local ID is less than the number of local number of edges per coarse bucket
	if(__builtin_expect(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
	
		// Set all local number of edges per coarse bucket to zero as a work group
		localNumberOfEdgesPerCoarseBucket[localId] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Go through all of this work group's edges as a work group
	for(uint i = 0; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_INTERMEDIATE_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Check if using min RAM for GPU trimming
		#if GPU_TRIMMING_USE_MIN_RAM
		
			// Get this work item's edge
			const uint edge = edges[min(i + localId, numberOfEdges - 1)];
			
			// Get edge's other node
			__builtin_assume(edge < NUMBER_OF_EDGES);
			const uint otherNode = sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2 + 1)) & NODE_MASK;
			
			// Get if this work item's edge survives by having a node pair in the bitmap
			const bool edgeSurvives = (i + localId < numberOfEdges) & isBitSetInBitmap(bitmap, (otherNode ^ 1) & GPU_BITMAP_ITEM_MASK);
			
		// Otherwise
		#else
		
			// Get this work item's edge and other node
			const uint2 edgeAndOtherNode = edges[min(i + localId, numberOfEdges - 1)];
			
			// Get if this work item's edge survives by having a node pair in the bitmap
			const bool edgeSurvives = (i + localId < numberOfEdges) & isBitSetInBitmap(bitmap, (edgeAndOtherNode.y ^ 1) & GPU_BITMAP_ITEM_MASK);
		#endif
		
		// Check if this work item's edge survives
		uint node;
		if(__builtin_expect(edgeSurvives, false)) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Get edge's node
				__builtin_assume(edge < NUMBER_OF_EDGES);
				node = sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2)) & NODE_MASK;
				
			// Otherwise
			#else
			
				// Get edge's node
				__builtin_assume(edgeAndOtherNode.x < NUMBER_OF_EDGES);
				node = sipHash24(trimEdgesParameters->sipHashKeys, ((ulong)(edgeAndOtherNode.x >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edgeAndOtherNode.x * 2)) & NODE_MASK;
			#endif
		}
		
		// Check if this work item's edge survives
		ushort coarseBucketIndex;
		ushort localNextEdgeIndex;
		if(__builtin_expect(edgeSurvives, false)) {
		
			// Get the coarse bucket index for the edge's node
			coarseBucketIndex = node >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
			
			// Get the local next edge index in the local coarse bucket
			localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1);
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Check if local ID is less than the number of next edge indices
		if(__builtin_expect(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
		
			// Check if the local coarse bucket isn't empty
			const uint localNumberOfEdges = atomic_xchg(&localNumberOfEdgesPerCoarseBucket[localId], 0);
			if(__builtin_expect(localNumberOfEdges, true)) {
			
				// Get all the next edge index in the coarse bucket as a work group
				nextEdgeIndex[localId] = atomic_add(&numberOfEdgesPerCoarseBucket[localId], localNumberOfEdges);
			}
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Check if this work item's edge survives
		if(__builtin_expect(edgeSurvives, false)) {
		
			// Check if using min RAM for GPU trimming
			#if GPU_TRIMMING_USE_MIN_RAM
			
				// Put this work item's edge's nodes in the coarse bucket
				coarseBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = (uint2)(node, otherNode);
				
			// Otherwise
			#else
			
				// Put this work item's edge's nodes in the coarse bucket
				coarseBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = (uint2)(node, edgeAndOtherNode.y);
			#endif
		}
	}
}

// Update largest final coarse bucket size
__kernel void updateLargestFinalCoarseBucketSize(__global uint *restrict largestCoarseBucketSize, __global const uint *restrict numberOfEdgesPerCoarseBucket) {

	// Go through all coarse buckets
	uint currentLargestCoarseBucketSize = 1;
	for(ushort i = 0; __builtin_expect(i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true); ++i) {
	
		// Update the current largest coarse bucket size
		currentLargestCoarseBucketSize = max(currentLargestCoarseBucketSize, numberOfEdgesPerCoarseBucket[i]);
	}
	
	// Set largest coarse bucket size based on the current largest coarse bucket size
	*largestCoarseBucketSize = ((currentLargestCoarseBucketSize + 2 - 1) / 2 + GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP - 1) / GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP;
}

// Fine bucket sort final edges
__kernel void fineBucketSortFinalEdges(__global uint2 *restrict fineBuckets, __global uint *restrict numberOfEdgesPerFineBucket, __global const uint2 *restrict coarseBuckets, __global const uint *restrict numberOfEdgesPerCoarseBucket, __global const uint *restrict largestCoarseBucketSize) {

	// Check if work group's edges don't exist
	if(get_group_id(0) >= *largestCoarseBucketSize) {
	
		// Return
		return;
	}
	
	// Get local ID
	const ushort localId = get_local_id(0);
	
	// Get group ID
	const ushort groupId = get_group_id(0);
	
	// Declare local number of edges per fine bucket
	__local uint localNumberOfEdgesPerFineBucket[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	__local uint nextEdgeIndex[GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION];
	
	// Get this work group's coarse bucket index
	const uint coarseBucketIndex = get_group_id(1);
	
	// Get the number of edges in this work group's coarse bucket
	const uint numberOfEdges = numberOfEdgesPerCoarseBucket[coarseBucketIndex];
	
	// Get this work item's coarse edge indices
	const uint coarseEdgeIndex = (uint)GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP * (groupId * 2) + localId;
	const uint coarseEdgeIndexOther = (uint)GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP * (groupId * 2 + 1) + localId;
	
	// Get if this work item's edges exist in the coarse bucket
	const bool edgeExists = coarseEdgeIndex < numberOfEdges;
	
	// Get if this work item's edge other exist in the coarse bucket
	const bool edgeExistsOther = coarseEdgeIndexOther < numberOfEdges;
	
	// Go through all local number of edges per fine bucket as a work group
	for(ushort i = localId; __builtin_expect(i < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, true); i += GPU_FINE_BUCKET_SORT_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Set local number of edges per fine bucket to zero
		localNumberOfEdgesPerFineBucket[i] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Check if this work item's edge exists
	uint2 nodes;
	uint fineBucketIndex;
	ushort localNextEdgeIndex;
	uint2 nodesOther;
	ushort fineBucketIndexOther;
	ushort localNextEdgeIndexOther;
	if(__builtin_expect(edgeExists, true)) {
	
		// Check if using max RAM for GPU trimming
		#if GPU_TRIMMING_USE_MAX_RAM
		
			// Get this work item's edge's nodes
			nodes = coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + coarseEdgeIndex];
			
		// Otherwise
		#else
		
			// Get this work item's edge's nodes
			nodes = coarseBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + coarseEdgeIndex];
		#endif
		
		// Get the fine bucket index for the edge's node or other node
		fineBucketIndex = (nodes.x >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
		
		// Get the local next edge index in the local fine bucket
		localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerFineBucket[fineBucketIndex], 1);
		
		// Check if this work item's other edge exists
		if(__builtin_expect(edgeExistsOther, true)) {
		
			// Check if using max RAM for GPU trimming
			#if GPU_TRIMMING_USE_MAX_RAM
			
				// Get this work item's other edge's nodes
				nodesOther = coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + coarseEdgeIndexOther];
				
			// Otherwise
			#else
			
				// Get this work item's other edge's nodes
				nodesOther = coarseBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + coarseEdgeIndexOther];
			#endif
			
			// Get the fine bucket index for the other edge's node or other node
			fineBucketIndexOther = (nodesOther.x >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_FINE_BUCKET_SORTING) & GPU_FINE_BUCKET_INDEX_MASK;
			
			// Get the other local next edge index in the local fine bucket
			localNextEdgeIndexOther = atomic_add(&localNumberOfEdgesPerFineBucket[fineBucketIndexOther], 1);
		}
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Check if local ID is less than the number of next edge indices
	if(__builtin_expect(localId < GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION, true)) {
	
		// Check if the local fine bucket isn't empty
		const uint localNumberOfEdges = localNumberOfEdgesPerFineBucket[localId];
		if(__builtin_expect(localNumberOfEdges, true)) {
		
			// Get all the next edge index in the fine bucket as a work group
			nextEdgeIndex[localId] = atomic_add(&numberOfEdgesPerFineBucket[(uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + localId], localNumberOfEdges);
		}
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Check if this work item's edge exists
	if(__builtin_expect(edgeExists, true)) {
	
		// Check if using min RAM for GPU trimming
		#if GPU_TRIMMING_USE_MIN_RAM
		
			// Put this work item's edge's nodes in the fine bucket
			fineBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET / 2) * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndex) + nextEdgeIndex[fineBucketIndex] + localNextEdgeIndex] = nodes;
			
		// Otherwise
		#else
		
			// Put this work item's edge's nodes in the fine bucket
			fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndex) + nextEdgeIndex[fineBucketIndex] + localNextEdgeIndex] = nodes;
		#endif
	}
	
	// Check if this work item's other edge exists
	if(__builtin_expect(edgeExistsOther, true)) {
	
		// Check if using min RAM for GPU trimming
		#if GPU_TRIMMING_USE_MIN_RAM
		
			// Put this work item's edge's nodes in the fine bucket
			fineBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET / 2) * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndexOther) + nextEdgeIndex[fineBucketIndexOther] + localNextEdgeIndexOther] = nodesOther;
			
		// Otherwise
		#else
		
			// Put this work item's edge's nodes in the fine bucket
			fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * ((uint)GPU_NUMBER_OF_FINE_BUCKETS_PER_DIMENSION * coarseBucketIndex + fineBucketIndexOther) + nextEdgeIndex[fineBucketIndexOther] + localNextEdgeIndexOther] = nodesOther;
		#endif
	}
}

// Trim final edges
__kernel void trimFinalEdges(__global uint2 *restrict coarseBuckets, __global uint *restrict numberOfEdgesPerCoarseBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket) {

	// Get local ID
	const uint localId = get_local_id(0);
	
	// Get group ID
	const ushort groupId = get_group_id(0);
	
	// Declare local number of edges per coarse bucket
	__local uint localNumberOfEdgesPerCoarseBucket[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	__local uint nextEdgeIndex[GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare bitmap
	__local uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
	
	// Get this work group's fine bucket index
	const uint fineBucketIndex = groupId;
	
	// Go through all bitmap parts as a work group
	for(ushort i = localId; __builtin_expect(i < GPU_BITMAP_SIZE / sizeof(uint), true); i += GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Set bitmap part to zero
		bitmap[i] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Check if using min RAM for GPU trimming
	#if GPU_TRIMMING_USE_MIN_RAM
	
		// Get the edges in this work group's fine bucket
		__global const uint2 *edges = &fineBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET / 2) * fineBucketIndex];
		
	// Otherwise
	#else
	
		// Get the edges in this work group's fine bucket
		__global const uint2 *edges = &fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * fineBucketIndex];
	#endif
	
	// Get the number of edges in this work group's fine bucket
	const uint numberOfEdges = numberOfEdgesPerFineBucket[fineBucketIndex];
	
	// Go through all of this work group's edges as a work group
	for(uint i = localId; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Set edge's node or other node in the bitmap
		setBitInBitmap(bitmap, edges[i].x & GPU_BITMAP_ITEM_MASK);
	}
	
	// Go through all next edge indices as a work group
	for(ushort i = localId; __builtin_expect(i < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true); i += GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Set local number of edges per coarse bucket to zero
		localNumberOfEdgesPerCoarseBucket[i] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Go through all of this work group's edges as a work group
	for(uint i = 0; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_FINAL_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Get this work item's edge's nodes
		const uint2 nodes = edges[min(i + localId, numberOfEdges - 1)];
		
		// Get if this work item's edge survives by having a node pair in the bitmap
		const bool edgeSurvives = (i + localId < numberOfEdges) && isBitSetInBitmap(bitmap, (nodes.x ^ 1) & GPU_BITMAP_ITEM_MASK);
		
		// Check if this work item's edge survives
		ushort coarseBucketIndex;
		ushort localNextEdgeIndex;
		if(__builtin_expect(edgeSurvives, true)) {
		
			// Get the coarse bucket index for the edge's node or other node
			coarseBucketIndex = nodes.y >> GPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
			
			// Get the local next edge index in the local coarse bucket
			localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerCoarseBucket[coarseBucketIndex], 1);
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Check if local ID is less than the number of next edge indices
		if(__builtin_expect(localId < GPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
		
			// Check if the local coarse bucket isn't empty
			const uint localNumberOfEdges = atomic_xchg(&localNumberOfEdgesPerCoarseBucket[localId], 0);
			if(__builtin_expect(localNumberOfEdges, true)) {
			
				// Get all the next edge index in the coarse bucket as a work group
				nextEdgeIndex[localId] = atomic_add(&numberOfEdgesPerCoarseBucket[localId], localNumberOfEdges);
			}
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Check if this work item's edge survives
		if(__builtin_expect(edgeSurvives, true)) {
		
			// Check if using max RAM for GPU trimming
			#if GPU_TRIMMING_USE_MAX_RAM
			
				// Put this work item's edge's nodes in the coarse bucket
				coarseBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = nodes.yx;
				
			// Otherwise
			#else
			
				// Put this work item's edge's nodes in the coarse bucket
				coarseBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET / 2) * coarseBucketIndex + nextEdgeIndex[coarseBucketIndex] + localNextEdgeIndex] = nodes.yx;
			#endif
		}
	}
}

// Trim final edges and transfer edges
__kernel void trimFinalEdgesAndTransferEdges(__global uint2 *restrict cpuBuckets, __global uint *restrict numberOfEdgesPerCpuBucket, __global const uint2 *restrict fineBuckets, __global const uint *restrict numberOfEdgesPerFineBucket) {

	// Get local ID
	const uint localId = get_local_id(0);
	
	// Get group ID
	const ushort groupId = get_group_id(0);
	
	// Declare local number of edges per CPU bucket
	__local uint localNumberOfEdgesPerCpuBucket[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare next edge index
	__local uint nextEdgeIndex[CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION];
	
	// Declare bitmap
	__local uint bitmap[GPU_BITMAP_SIZE / sizeof(uint)];
	
	// Get this work group's fine bucket index
	const uint fineBucketIndex = groupId;
	
	// Get this work group's coarse bucket index
	const ushort coarseBucketIndex = fineBucketIndex >> GPU_NUMBER_OF_MOST_SIGNIFICANT_BITS_USED_FOR_FINE_BUCKET_SORTING;
	
	// Go through all bitmap parts as a work group
	for(ushort i = localId; __builtin_expect(i < GPU_BITMAP_SIZE / sizeof(uint), true); i += GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Set bitmap part to zero
		bitmap[i] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Check if using min RAM for GPU trimming
	#if GPU_TRIMMING_USE_MIN_RAM
	
		// Get the edges in this work group's fine bucket
		__global const uint2 *edges = &fineBuckets[(ulong)(GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET / 2) * fineBucketIndex];
		
	// Otherwise
	#else
	
		// Get the edges in this work group's fine bucket
		__global const uint2 *edges = &fineBuckets[(ulong)GPU_MAX_NUMBER_OF_EDGES_PER_FINE_BUCKET * fineBucketIndex];
	#endif
	
	// Get the number of edges in this work group's fine bucket
	const uint numberOfEdges = numberOfEdgesPerFineBucket[fineBucketIndex];
	
	// Go through all of this work group's edges as a work group
	for(uint i = localId; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Set edge's node or other node in the bitmap
		setBitInBitmap(bitmap, edges[i].x & GPU_BITMAP_ITEM_MASK);
	}
	
	// Check if local ID is less than the number of local number of edges per CPU bucket
	if(__builtin_expect(localId < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
	
		// Set all local number of edges per CPU buckets to zero as a work group
		localNumberOfEdgesPerCpuBucket[localId] = 0;
	}
	
	// Synchronize work group
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Go through all of this work group's edges as a work group
	for(uint i = 0; __builtin_expect(i < numberOfEdges, true); i += GPU_TRIM_FINAL_EDGES_AND_TRANSFER_EDGES_KERNEL_NUMBER_OF_WORK_ITEMS_PER_WORK_GROUP) {
	
		// Get this work item's edge's nodes
		const uint2 nodes = edges[min(i + localId, numberOfEdges - 1)];
		
		// Get if this work item's edge survives by having a node pair in the bitmap
		const bool edgeSurvives = (i + localId < numberOfEdges) && isBitSetInBitmap(bitmap, (nodes.x ^ 1) & GPU_BITMAP_ITEM_MASK);
		
		// Check if this work item's edge survives
		ushort cpuBucketIndex;
		ushort localNextEdgeIndex;
		if(__builtin_expect(edgeSurvives, true)) {
		
			// Get the CPU bucket index for the edge's node
			cpuBucketIndex = nodes.y >> CPU_NUMBER_OF_LEAST_SIGNIFICANT_BITS_IGNORED_DURING_COARSE_BUCKET_SORTING;
			
			// Get the local next edge index in the local CPU bucket
			localNextEdgeIndex = atomic_add(&localNumberOfEdgesPerCpuBucket[cpuBucketIndex], 1);
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Check if local ID is less than the number of next edge indices
		if(__builtin_expect(localId < CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION, true)) {
		
			// Check if the local CPU bucket isn't empty
			const uint localNumberOfEdges = atomic_xchg(&localNumberOfEdgesPerCpuBucket[localId], 0);
			if(__builtin_expect(localNumberOfEdges, true)) {
			
				// Get all the next edge index in the CPU bucket as a work group
				nextEdgeIndex[localId] = atomic_add(&numberOfEdgesPerCpuBucket[(uint)CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * coarseBucketIndex + localId], localNumberOfEdges);
			}
		}
		
		// Synchronize work group
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// Get this work item's edge index
		const uint edgeIndex = nextEdgeIndex[cpuBucketIndex % CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION] + localNextEdgeIndex;
		
		// Check if edge index is valid
		if(__builtin_expect(edgeIndex < CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * edgeSurvives, true)) {
		
			// Put this work item's edge's nodes in the CPU bucket
			cpuBuckets[(ulong)CPU_MAX_NUMBER_OF_EDGES_PER_COARSE_BUCKET * ((uint)CPU_NUMBER_OF_COARSE_BUCKETS_PER_DIMENSION * coarseBucketIndex + cpuBucketIndex) + edgeIndex] = nodes.yx;
		}
	}
}

// Recover edges
__kernel void recoverEdges(__global uint *restrict solutionEdges, __constant const struct RecoverEdgesParameters *restrict recoverEdgesParameters) {

	// Get global ID
	const uint globalId = get_global_id(0);
	
	// Create recovered edge candidates
	uint3 recoveredEdgeCandidates[GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM];
	
	// Set number of recovered edge candidates to zero
	ushort numberOfRecoveredEdgeCandidates = 0;
	
	// Go through all of this work item's edges
	for(uint edge = globalId * GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM; __builtin_expect(edge < globalId * GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM + GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_EDGES_PER_WORK_ITEM, true); ++edge) {
	
		// Get edge's node
		__builtin_assume(edge < GPU_NUMBER_OF_RECOVERING_EDGES);
		const uint node = sipHash24(recoverEdgesParameters->solutionSipHashKeys, ((ulong)(edge >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (edge * 2)) & NODE_MASK;
		
		// Get node's pair in the first partition
		const uint nodePair = node >> 1;
		
		// Check if solution size is forty-two
		#if SOLUTION_SIZE == 42
		
			// Perform binary search for the node pair in the list of solution node pairs in the first partition
			uint currentIndex = (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2;
			currentIndex += (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 * (__builtin_expect(nodePair < recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex], false) ? -1 : (nodePair > recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex]));
			
			__builtin_assume(currentIndex >= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 - (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 && currentIndex <= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 + (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4);
			currentIndex += (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8 * (__builtin_expect(nodePair < recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex], false) ? -1 : (nodePair > recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex]));
			
			__builtin_assume(currentIndex >= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 - (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 - (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8 && currentIndex <= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 + (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 + (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8);
			currentIndex += (SOLUTION_SIZE / 2 + (16 - 1) / 2) / 16 * ((nodePair > recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex]) - (nodePair < recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex]));
			
			__builtin_assume(currentIndex >= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 - (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 - (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8 - (SOLUTION_SIZE / 2 + (16 - 1) / 2) / 16 && currentIndex <= (SOLUTION_SIZE / 2 + (2 - 1) / 2) / 2 + (SOLUTION_SIZE / 2 + (4 - 1) / 2) / 4 + (SOLUTION_SIZE / 2 + (8 - 1) / 2) / 8 + (SOLUTION_SIZE / 2 + (16 - 1) / 2) / 16);
			currentIndex += (SOLUTION_SIZE / 2 + (32 - 1) / 2) / 32 * ((nodePair > recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex]) - (nodePair < recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex]));
			
			// Check if node pair is in the list of solution node pairs in the first partition
			__builtin_assume(currentIndex < SOLUTION_SIZE / 2);
			if(__builtin_expect(nodePair == recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex], false)) {
			
				// Save edge, its node, and its index in the list of recovered edge candidates
				recoveredEdgeCandidates[min(numberOfRecoveredEdgeCandidates++, (ushort)(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM - 1))] = (uint3)(edge, node, currentIndex);
			}
			
		// Otherwise
		#else
		
			// Go through all solution node pairs in the first partition
			for(uint currentIndex = 0; __builtin_expect(currentIndex < SOLUTION_SIZE / 2, true); ++currentIndex) {
			
				// Check if node pair is the solution node pair in the first partition
				if(__builtin_expect(nodePair == recoverEdgesParameters->solutionNodePairsFirstPartition[currentIndex], false)) {
				
					// Save edge, its node, and its index in the list of recovered edge candidates
					recoveredEdgeCandidates[min(numberOfRecoveredEdgeCandidates++, (ushort)(GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM - 1))] = (uint3)(edge, node, currentIndex);
					
					// Break
					break;
				}
			}
		#endif
	}
	
	// Create local solution edges
	uint localSolutionEdges[SOLUTION_SIZE];
	
	// Go through all local solution edges
	for(ushort i = 0; __builtin_expect(i < SOLUTION_SIZE, true); ++i) {
	
		// Set local solution edge to zero
		localSolutionEdges[i] = 0;
	}
	
	// Go through all recovered edge candidates
	for(ushort i = 0; __builtin_expect(i < min(numberOfRecoveredEdgeCandidates, (ushort)GPU_RECOVER_EDGES_KERNEL_NUMBER_OF_RECOVERED_EDGE_CANDIDATES_PER_WORK_ITEM), false); ++i) {
	
		// Get edge's other node
		__builtin_assume(recoveredEdgeCandidates[i].x < GPU_NUMBER_OF_RECOVERING_EDGES);
		const ulong bothNodes = as_ulong((uint2)(sipHash24(recoverEdgesParameters->solutionSipHashKeys, ((ulong)(recoveredEdgeCandidates[i].x >> (sizeof(uint) * BITS_IN_A_BYTE - 1)) << (sizeof(uint) * BITS_IN_A_BYTE)) | (recoveredEdgeCandidates[i].x * 2 + 1)) & NODE_MASK, recoveredEdgeCandidates[i].y));
		
		// Set solution index from where the node pair was in the list of solution node pairs in the first partition
		__builtin_assume(recoveredEdgeCandidates[i].z < SOLUTION_SIZE / 2);
		const uint solutionIndex = recoveredEdgeCandidates[i].z * 2;
		
		// Set local solution edge to the edge if the edge is part of the solution
		__builtin_assume(solutionIndex < SOLUTION_SIZE - 1);
		localSolutionEdges[solutionIndex + (bothNodes == recoverEdgesParameters->solutionNodes[solutionIndex + 1])] |= recoveredEdgeCandidates[i].x * ((bothNodes == recoverEdgesParameters->solutionNodes[solutionIndex]) | (bothNodes == recoverEdgesParameters->solutionNodes[solutionIndex + 1]));
	}
	
	// Go through all local solution edges
	for(ushort i = 0; __builtin_expect(i < SOLUTION_SIZE, true); ++i) {
	
		// Check if local solution edge is set
		if(__builtin_expect(localSolutionEdges[i], false)) {
		
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
	sipRound(&states);
	sipRound(&states);
	__builtin_assume(nonce < NUMBER_OF_EDGES * 2 - 1);
	states.even ^= (ulong2)(nonce, 255);
	sipRound(&states);
	sipRound(&states);
	sipRound(&states);
	states.even += states.odd;
	states.y = rotate(states.y, (ulong)(13));
	states.w = as_ulong(as_ushort4(states.w).wxyz);
	states.odd ^= states.even;
	states.z += states.y;
	states.odd = rotate(states.odd, (ulong2)(17, SIP_ROUND_ROTATION));
	
	// Return result
	return states.y ^ states.z ^ (states.z >> (sizeof(uint) * BITS_IN_A_BYTE)) ^ states.w;
}

// SipRound
static inline void sipRound(ulong4 *states) {

	// Perform SipRound on states
	states->even += states->odd;
	states->y = rotate(states->y, (ulong)13);
	states->w = as_ulong(as_ushort4(states->w).wxyz);
	states->odd ^= states->even;
	states->x = as_ulong(as_uint2(states->x).yx);
	states->even += states->wy;
	states->odd = rotate(states->odd, (ulong2)(17, SIP_ROUND_ROTATION));
	states->odd ^= states->zx;
	states->z = as_ulong(as_uint2(states->z).yx);
}

// Set bit in bitmap
static inline void setBitInBitmap(__local uint *bitmap, const uint index) {

	// Set bit in bitmap
	__builtin_assume(index <= GPU_BITMAP_ITEM_MASK);
	atomic_or(&bitmap[index / (sizeof(uint) * BITS_IN_A_BYTE)], (uint)1 << (index % (sizeof(uint) * BITS_IN_A_BYTE)));
}

// Is bit set in bitmap
static inline bool isBitSetInBitmap(__local const uint *bitmap, const uint index) {

	// Return if bit is set in bitmap
	__builtin_assume(index <= GPU_BITMAP_ITEM_MASK);
	return bitmap[index / (sizeof(uint) * BITS_IN_A_BYTE)] & ((uint)1 << (index % (sizeof(uint) * BITS_IN_A_BYTE)));
}


)"
