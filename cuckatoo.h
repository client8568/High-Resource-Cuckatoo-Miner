// Header guard
#ifndef CUCKATOO_H
#define CUCKATOO_H


// Header files
#include "common.h"
#include <algorithm>
#include <execution>
#include "hash_table.h"

using namespace std;


// Constants

// Cuckatoo node connections per edge
#define CUCKATOO_NODE_CONNECTIONS_PER_EDGE 2


// Structures

// Cuckatoo node connection structure
struct CuckatooNodeConnection {

	// Previous node connection index
	uint32_t previousNodeConnectionIndex;
	
	// Node
	uint32_t node;
};


// Function prototypes

// Cuckatoo get solution
__attribute__((always_inline)) static inline bool cuckatooGetSolution(uint32_t solutionNodes[SOLUTION_SIZE * NUMBER_OF_EDGE_COMPONENTS], const uint32_t *__restrict__ edges, const uint32_t numberOfEdges, CuckatooNodeConnection nodeConnections[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE], const uint32_t firstUnusedNodeConnectionsIndex, HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsFirstPartition, HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsSecondPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsFirstPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsSecondPartition) noexcept;

// Cuckatoo search node connections for solution first partition
__attribute__((always_inline)) static inline bool cuckatooSearchNodeConnectionsForSolutionFirstPartition(uint32_t solutionNodes[SOLUTION_SIZE * NUMBER_OF_EDGE_COMPONENTS], const int cycleSize, const uint32_t node, uint32_t nodeConnectionIndex, const CuckatooNodeConnection nodeConnections[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE], const HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsFirstPartition, const HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsSecondPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsFirstPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsSecondPartition, const uint32_t finalNode) noexcept;

// Cuckatoo search node connections for solution second partition
__attribute__((always_inline)) static inline bool cuckatooSearchNodeConnectionsForSolutionSecondPartition(uint32_t solutionNodes[SOLUTION_SIZE * NUMBER_OF_EDGE_COMPONENTS], const int cycleSize, const uint32_t node, uint32_t nodeConnectionIndex, const CuckatooNodeConnection nodeConnections[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE], const HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsFirstPartition, const HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsSecondPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsFirstPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsSecondPartition, const uint32_t finalNode) noexcept;


// Supporting function implementation

// Cuckatoo get solution
__attribute__((always_inline)) static inline bool cuckatooGetSolution(uint32_t solutionNodes[SOLUTION_SIZE * NUMBER_OF_EDGE_COMPONENTS], const uint32_t *__restrict__ edges, const uint32_t numberOfEdges, CuckatooNodeConnection nodeConnections[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE], const uint32_t firstUnusedNodeConnectionsIndex, HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsFirstPartition, HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsSecondPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsFirstPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsSecondPartition) noexcept {

	// Go through all edges
	for(uint32_t nodeConnectionsIndex = firstUnusedNodeConnectionsIndex; nodeConnectionsIndex < firstUnusedNodeConnectionsIndex + numberOfEdges * CUCKATOO_NODE_CONNECTIONS_PER_EDGE; nodeConnectionsIndex += CUCKATOO_NODE_CONNECTIONS_PER_EDGE, edges += NUMBER_OF_EDGE_COMPONENTS) [[likely]] {
	
		// Get edge's nodes
		uint32_t node = edges[0];
		const uint32_t &finalNode = edges[1];
		
		// Replace newest node connection for the node in the first partition
		nodeConnections[nodeConnectionsIndex] = {newestNodeConnectionsFirstPartition.replace(node, nodeConnectionsIndex), node};
		
		// Replace newest node connection for the node in the second partition
		nodeConnections[nodeConnectionsIndex + 1] = {newestNodeConnectionsSecondPartition.replace(finalNode, nodeConnectionsIndex + 1), finalNode};
		
		// Check if both nodes have a pair
		uint32_t nodeConnectionIndex = newestNodeConnectionsFirstPartition.get(node ^ 1);
		if(nodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE && newestNodeConnectionsSecondPartition.contains(finalNode ^ 1)) [[unlikely]] {
		
			// Reset visited node pairs
			visitedNodePairsFirstPartition.clear();
			visitedNodePairsSecondPartition.clear();
			
			// Go through all nodes in the cycle
			for(int cycleSize = 1;; cycleSize += 2) [[likely]] {
			
				// Set that node pair has been visited
				visitedNodePairsFirstPartition.setUnique(node >> 1);
				
				// Check if node's pair has more than one connection
				if(nodeConnections[nodeConnectionIndex].previousNodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE) [[unlikely]] {
				
					// Go through all of the node's pair's connections
					do [[unlikely]] {
					
						// Check if the connected node has a pair
						const uint32_t &connectedNode = nodeConnections[nodeConnectionIndex + 1].node;
						const uint32_t connectedNodeConnectionIndex = newestNodeConnectionsSecondPartition.get(connectedNode ^ 1);
						
						if(connectedNodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE) [[unlikely]] {
						
							// Check if the connected node's pair wasn't already visited
							if(!visitedNodePairsSecondPartition.contains(connectedNode >> 1)) [[likely]] {
							
								// Check if cycle might be a solution
								if(cycleSize == SOLUTION_SIZE - 1) [[unlikely]] {
								
									// Check if cycle is complete
									if((connectedNode ^ 1) == finalNode) [[unlikely]] {
									
										// Include edge's nodes in solution nodes
										solutionNodes[0] = finalNode;
										solutionNodes[1] = edges[0];
										
										// Include node's pair and the connected node in solution nodes
										solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS] = connectedNode;
										solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS + 1] = nodeConnections[nodeConnectionIndex].node;
										
										// Sort solution nodes in ascending order of each node pair
										sort(execution::unseq, reinterpret_cast<uint64_t *>(solutionNodes), reinterpret_cast<uint64_t *>(solutionNodes) + SOLUTION_SIZE);
										
										// Return true
										return true;
									}
								}
								
								// Otherwise
								else [[likely]] {
								
									// Check if solution was found at the connected node's pair
									if(cuckatooSearchNodeConnectionsForSolutionSecondPartition(solutionNodes, cycleSize + 1, connectedNode ^ 1, connectedNodeConnectionIndex, nodeConnections, newestNodeConnectionsFirstPartition, newestNodeConnectionsSecondPartition, visitedNodePairsFirstPartition, visitedNodePairsSecondPartition, finalNode)) [[unlikely]] {
									
										// Include edge's nodes in solution nodes
										solutionNodes[0] = finalNode;
										solutionNodes[1] = edges[0];
										
										// Include node's pair and the connected node in solution nodes
										solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS] = connectedNode;
										solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS + 1] = nodeConnections[nodeConnectionIndex].node;
										
										// Sort solution nodes in ascending order of each node pair
										sort(execution::unseq, reinterpret_cast<uint64_t *>(solutionNodes), reinterpret_cast<uint64_t *>(solutionNodes) + SOLUTION_SIZE);
										
										// Return true
										return true;
									}
								}
							}
						}
						
						// Go to next node's pair's connection
						nodeConnectionIndex = nodeConnections[nodeConnectionIndex].previousNodeConnectionIndex;
						
					} while(nodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE);
					
					// Break
					break;
				}
				
				// Go to node's pair opposite end
				node = nodeConnections[nodeConnectionIndex + 1].node;
				
				// Check if node doesn't have a pair
				uint32_t nextNodeConnectionIndex = newestNodeConnectionsSecondPartition.get(node ^ 1);
				if(nextNodeConnectionIndex == HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE) [[likely]] {
				
					// break
					break;
				}
				
				// Check if node pair was already visited
				if(visitedNodePairsSecondPartition.contains(node >> 1)) [[unlikely]] {
				
					// Break
					break;
				}
				
				// Check if cycle might be a solution
				if(cycleSize == SOLUTION_SIZE - 1) [[unlikely]] {
				
					// Check if cycle is complete
					if((node ^ 1) == finalNode) [[unlikely]] {
					
						// Include edge's nodes in solution nodes
						solutionNodes[0] = finalNode;
						solutionNodes[1] = edges[0];
						
						// Include node's pair and node in solution nodes
						solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS] = node;
						solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS + 1] = nodeConnections[nodeConnectionIndex].node;
						
						// Sort solution nodes in ascending order of each node pair
						sort(execution::unseq, reinterpret_cast<uint64_t *>(solutionNodes), reinterpret_cast<uint64_t *>(solutionNodes) + SOLUTION_SIZE);
						
						// Return true
						return true;
					}
					
					// Break
					break;
				}
				
				// Include node's pair and node in solution nodes
				solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS] = node;
				solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS + 1] = nodeConnections[nodeConnectionIndex].node;
				
				// Set that node pair has been visited
				visitedNodePairsSecondPartition.setUnique(node >> 1);
				
				// Check if node's pair has more than one connection
				if(nodeConnections[nextNodeConnectionIndex].previousNodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE) [[unlikely]] {
				
					// Go through all of the node's pair's connections
					do [[unlikely]] {
					
						// Check if the connected node has a pair
						const uint32_t &connectedNode = nodeConnections[nextNodeConnectionIndex - 1].node;
						const uint32_t connectedNodeConnectionIndex = newestNodeConnectionsFirstPartition.get(connectedNode ^ 1);
						
						if(connectedNodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE) [[unlikely]] {
						
							// Check if the connected node's pair wasn't already visited
							if(!visitedNodePairsFirstPartition.contains(connectedNode >> 1)) [[likely]] {
								
								// Check if solution was found at the connected node's pair
								if(cuckatooSearchNodeConnectionsForSolutionFirstPartition(solutionNodes, cycleSize + 2, connectedNode ^ 1, connectedNodeConnectionIndex, nodeConnections, newestNodeConnectionsFirstPartition, newestNodeConnectionsSecondPartition, visitedNodePairsFirstPartition, visitedNodePairsSecondPartition, finalNode)) [[unlikely]] {
								
									// Include edge's nodes in solution nodes
									solutionNodes[0] = finalNode;
									solutionNodes[1] = edges[0];
									
									// Include node's pair and the connected node in solution nodes
									solutionNodes[(cycleSize + 1) * NUMBER_OF_EDGE_COMPONENTS] = nodeConnections[nextNodeConnectionIndex].node;
									solutionNodes[(cycleSize + 1) * NUMBER_OF_EDGE_COMPONENTS + 1] = connectedNode;
									
									// Sort solution nodes in ascending order of each node pair
									sort(execution::unseq, reinterpret_cast<uint64_t *>(solutionNodes), reinterpret_cast<uint64_t *>(solutionNodes) + SOLUTION_SIZE);
									
									// Return true
									return true;
								}
							}
						}
						
						// Go to next node's pair's connection
						nextNodeConnectionIndex = nodeConnections[nextNodeConnectionIndex].previousNodeConnectionIndex;
						
					} while(nextNodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE);
					
					// Break
					break;
				}
				
				// Go to node's pair opposite end
				node = nodeConnections[nextNodeConnectionIndex - 1].node;
				
				// Check if node doesn't have a pair
				nodeConnectionIndex = newestNodeConnectionsFirstPartition.get(node ^ 1);
				if(nodeConnectionIndex == HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE) [[likely]] {
				
					// break
					break;
				}
				
				// Check if node pair was already visited
				if(visitedNodePairsFirstPartition.contains(node >> 1)) [[unlikely]] {
				
					// Break
					break;
				}
				
				// Include node's pair and node in solution nodes
				solutionNodes[(cycleSize + 1) * NUMBER_OF_EDGE_COMPONENTS] = nodeConnections[nextNodeConnectionIndex].node;
				solutionNodes[(cycleSize + 1) * NUMBER_OF_EDGE_COMPONENTS + 1] = node;
			}
		}
	}
	
	// Return false
	return false;
}

// Cuckatoo search node connections for solution first partition
__attribute__((always_inline)) static inline bool cuckatooSearchNodeConnectionsForSolutionFirstPartition(uint32_t solutionNodes[SOLUTION_SIZE * NUMBER_OF_EDGE_COMPONENTS], const int cycleSize, const uint32_t node, uint32_t nodeConnectionIndex, const CuckatooNodeConnection nodeConnections[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE], const HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsFirstPartition, const HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsSecondPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsFirstPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsSecondPartition, const uint32_t finalNode) noexcept {

	// Set that node pair has been visited
	const uint32_t visitedNodePairIndex = visitedNodePairsFirstPartition.setUniqueAndGetIndex(node >> 1);
	
	// Go through all of the node's connections
	do [[unlikely]] {
	
		// Check if the connected node has a pair
		const uint32_t &connectedNode = nodeConnections[nodeConnectionIndex + 1].node;
		const uint32_t connectedNodeConnectionIndex = newestNodeConnectionsSecondPartition.get(connectedNode ^ 1);
		
		if(connectedNodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE) [[unlikely]] {
		
			// Check if the connected node's pair wasn't already visited
			if(!visitedNodePairsSecondPartition.contains(connectedNode >> 1)) [[likely]] {
			
				// Check if cycle might be a solution
				if(cycleSize == SOLUTION_SIZE - 1) [[unlikely]] {
				
					// Check if cycle is complete
					if((connectedNode ^ 1) == finalNode) [[unlikely]] {
					
						// Include node and connected node in solution nodes
						solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS] = connectedNode;
						solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS + 1] = node;
						
						// Return true
						return true;
					}
				}
				
				// Otherwise
				else [[likely]] {
				
					// Check if solution was found at the connected node's pair
					if(cuckatooSearchNodeConnectionsForSolutionSecondPartition(solutionNodes, cycleSize + 1, connectedNode ^ 1, connectedNodeConnectionIndex, nodeConnections, newestNodeConnectionsFirstPartition, newestNodeConnectionsSecondPartition, visitedNodePairsFirstPartition, visitedNodePairsSecondPartition, finalNode)) [[unlikely]] {
					
						// Include node and connected node in solution nodes
						solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS] = connectedNode;
						solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS + 1] = node;
						
						// Return true
						return true;
					}
				}
			}
		}
		
		// Go to next node's connection
		nodeConnectionIndex = nodeConnections[nodeConnectionIndex].previousNodeConnectionIndex;
		
	} while(nodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE);
	
	// Set that node pair hasn't been visited
	visitedNodePairsFirstPartition.removeMostRecentSetUique(visitedNodePairIndex);
	
	// Return false
	return false;
}

// Cuckatoo search node connections for cuckatoo solution second partition
__attribute__((always_inline)) static inline bool cuckatooSearchNodeConnectionsForSolutionSecondPartition(uint32_t solutionNodes[SOLUTION_SIZE * NUMBER_OF_EDGE_COMPONENTS], const int cycleSize, const uint32_t node, uint32_t nodeConnectionIndex, const CuckatooNodeConnection nodeConnections[MAX_NUMBER_OF_EDGES_AFTER_TRIMMING * CUCKATOO_NODE_CONNECTIONS_PER_EDGE], const HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsFirstPartition, const HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING> &__restrict__ newestNodeConnectionsSecondPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsFirstPartition, HashTable<SOLUTION_SIZE / 2, true> &__restrict__ visitedNodePairsSecondPartition, const uint32_t finalNode) noexcept {

	// Set that node pair has been visited
	const uint32_t visitedNodePairIndex = visitedNodePairsSecondPartition.setUniqueAndGetIndex(node >> 1);
	
	// Go through all of the node's connections
	do [[unlikely]] {
	
		// Check if the connected node has a pair
		const uint32_t &connectedNode = nodeConnections[nodeConnectionIndex - 1].node;
		const uint32_t connectedNodeConnectionIndex = newestNodeConnectionsFirstPartition.get(connectedNode ^ 1);
		
		if(connectedNodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE) [[unlikely]] {
		
			// Check if the connected node's pair wasn't already visited
			if(!visitedNodePairsFirstPartition.contains(connectedNode >> 1)) [[likely]] {
			
				// Check if solution was found at the connected node's pair
				if(cuckatooSearchNodeConnectionsForSolutionFirstPartition(solutionNodes, cycleSize + 1, connectedNode ^ 1, connectedNodeConnectionIndex, nodeConnections, newestNodeConnectionsFirstPartition, newestNodeConnectionsSecondPartition, visitedNodePairsFirstPartition, visitedNodePairsSecondPartition, finalNode)) [[unlikely]] {
				
					// Include node and connected node in solution nodes
					solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS] = node;
					solutionNodes[cycleSize * NUMBER_OF_EDGE_COMPONENTS + 1] = connectedNode;
					
					// Return true
					return true;
				}
			}
		}
		
		// Go to next node's connection
		nodeConnectionIndex = nodeConnections[nodeConnectionIndex].previousNodeConnectionIndex;
		
	} while(nodeConnectionIndex != HashTable<MAX_NUMBER_OF_EDGES_AFTER_TRIMMING>::NO_VALUE);
	
	// Set that node pair hasn't been visited
	visitedNodePairsSecondPartition.removeMostRecentSetUique(visitedNodePairIndex);
	
	// Return false
	return false;
}


#endif
