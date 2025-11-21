/**
 * @file cc_sequential.c
 * @brief Sequential algorithms for computing connected components.
 *
 * This module implements two sequential algorithms for finding connected
 * components in an undirected graph represented as a sparse binary matrix:
 *
 * - Label Propagation (variant 0): Iteratively propagates minimum labels
 *   until convergence. Simple but slow.
 *
 * - Union-Find (variant 1): Uses disjoint-set data structure with path
 *   halving optimization. Generally faster and more scalable.
 *
 * Both algorithms return the count of unique connected components.
 */

#include <stdlib.h>
#include <errno.h>
#include "connected_components.h"
#include "error.h"

/* ========================================================================== */
/*                           UNION-FIND ALGORITHM                             */
/* ========================================================================== */

/**
 * @brief Finds the root of a node with path halving optimization.
 *
 * Path halving is a one-pass variant of path compression that makes every
 * node point to its grandparent, effectively halving the path length on
 * each traversal. This provides nearly the same performance as full path
 * compression with less overhead.
 *
 * @param label Array where label[i] is the parent of node i
 * @param i Node to find root for
 * @return Root node (where label[root] == root)
 */
static inline uint32_t
find_root_halving(uint32_t *label, uint32_t i)
{
	while (label[i] != i) {
		label[i] = label[label[i]];  // Path halving: skip one level
		i = label[i];
	}
	return i;
}

/**
 * @brief Unites two nodes by attaching their roots.
 *
 * This performs union-by-index, where the root with the larger index is
 * always attached to the root with the smaller index. This maintains a
 * canonical form where component representatives are always the minimum
 * node index in each component.
 *
 * @param label Array of parent pointers
 * @param i First node
 * @param j Second node
 * @return 1 if union was performed, 0 if nodes already in same set
 */
static inline int
union_nodes_by_index(uint32_t *label, uint32_t i, uint32_t j)
{
	uint32_t root_i = find_root_halving(label, i);
	uint32_t root_j = find_root_halving(label, j);
	
	if (root_i == root_j)
		return 0;
	
	// Attach larger index to smaller (maintains canonical form)
	if (root_i < root_j) {
		label[root_j] = root_i;
	} else {
		label[root_i] = root_j;
	}
	return 1;
}

/**
 * @brief Computes connected components using union-find algorithm.
 *
 * Algorithm steps:
 * 1. Initialize each node as its own parent (singleton sets)
 * 2. For each edge (i,j), union the sets containing i and j
 * 3. Perform final path compression to flatten all trees
 * 4. Count nodes that are their own parent (roots = components)
 *
 * @param matrix Sparse binary matrix in CSC format representing graph
 * @return Number of connected components, or -1 on error
 */
static int
cc_union_find(const CSCBinaryMatrix *matrix)
{
	uint32_t *label = malloc(matrix->nrows * sizeof(uint32_t));
	if (!label) {
		print_error(__func__, "malloc() failed", errno);
		return -1;
	}
	
	// Initialize: each node is its own parent
	for (size_t i = 0; i < matrix->nrows; i++) {
		label[i] = i;
	}
	
	// Process all edges: union connected nodes
	for (size_t i = 0; i < matrix->ncols; i++) {
		for (uint32_t j = matrix->col_ptr[i]; j < matrix->col_ptr[i+1]; j++) {
			union_nodes_by_index(label, i, matrix->row_idx[j]);
		}
	}
	
	// Final compression pass: flatten all paths for accurate counting
	for (size_t i = 0; i < matrix->nrows; i++) {
		find_root_halving(label, i);
	}
	
	// Count roots (each root represents one component)
	uint32_t uniqueCount = 0;
	for (size_t i = 0; i < matrix->nrows; i++) {
		if (label[i] == i) {
			uniqueCount++;
		}
	}
	
	free(label);
	return (int)uniqueCount;
}

/* ========================================================================== */
/*                       LABEL PROPAGATION ALGORITHM                          */
/* ========================================================================== */

/**
 * @brief Swaps labels to propagate the minimum value.
 *
 * If the two nodes have different labels, both are updated to the minimum
 * of the two values.
 *
 * @param label Array of node labels
 * @param i First node
 * @param j Second node
 * @return 1 if labels were changed, 0 if already equal
 */
static inline int
swap_min(uint32_t *label, uint32_t i, uint32_t j)
{
	if (label[i] == label[j])
		return 0;
	
	if (label[i] < label[j]) {
		label[j] = label[i];
	} else {
		label[i] = label[j];
	}
	return 1;
}

/**
 * @brief Comparison function for qsort on uint32_t arrays.
 *
 * @param a Pointer to first element
 * @param b Pointer to second element
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
static int
cmp_uint32(const void *a, const void *b)
{
	uint32_t x = *(uint32_t*)a;
	uint32_t y = *(uint32_t*)b;
	if (x < y) return -1;
	if (x > y) return 1;
	return 0;
}

/**
 * @brief Computes connected components using label propagation.
 *
 * Algorithm steps:
 * 1. Initialize each node with its own index as label
 * 2. Iterate over all edges, propagating minimum labels
 * 3. Repeat until no labels change (convergence)
 * 4. Sort labels and count unique values
 *
 * This method is simple but can be slow on graphs with long chains or
 * deep structures, as it may require many iterations to converge.
 *
 * @param matrix Sparse binary matrix in CSC format representing graph
 * @return Number of connected components, or -1 on error
 */
static int
cc_label_propegation(const CSCBinaryMatrix *matrix)
{
	uint32_t *label = malloc(sizeof(uint32_t) * matrix->nrows);
	if (!label) {
		return -1;
	}
	
	// Initialize: each node labeled with its own index
	for (size_t i = 0; i < matrix->nrows; i++) {
		label[i] = i;
	}
	
	// Iterate until convergence
	uint8_t finished;
	do {
		finished = 1;
		
		// Process all edges, propagating minimum labels
		for (size_t i = 0; i < matrix->ncols; i++) {
			for (uint32_t j = matrix->col_ptr[i]; j < matrix->col_ptr[i+1]; j++) {
				uint32_t r, c;
				c = i;
				r = matrix->row_idx[j];
				
				if (swap_min(label, c, r)) {
					finished = 0;  // Labels changed, need another iteration
				}
			}
		}
	} while (!finished);
	
	// Sort labels to group identical values
	qsort(label, matrix->nrows, sizeof(uint32_t), cmp_uint32);
	
	// Count unique labels (each unique label is one component)
	uint32_t uniqueCount = 0;
	for (size_t i = 0; i < matrix->nrows; i++) {
		if (i == 0 || label[i] != label[i-1])
			uniqueCount++;
	}
	
	free(label);
	return (int)uniqueCount;
}

/* ========================================================================== */
/*                              PUBLIC INTERFACE                              */
/* ========================================================================== */

/**
 * @brief Computes connected components using sequential algorithms.
 *
 * This is the main entry point for sequential connected components
 * computation. It dispatches to one of two algorithm implementations
 * based on the variant parameter.
 *
 * Supported variants:
 *   0: Label propagation (simple, slower)
 *   1: Union-find (recommended, faster)
 *
 * @param matrix Sparse binary matrix in CSC format
 * @param n_threads Unused (for API compatibility with parallel version)
 * @param algorithm_variant Algorithm selection (0 or 1)
 * @return Number of connected components, or -1 on error
 */
int
cc_sequential(const CSCBinaryMatrix *matrix,
              const unsigned int n_threads __attribute__((unused)),
              const unsigned int algorithm_variant)
{
	switch (algorithm_variant) {
	case 0:
		return cc_label_propegation(matrix);
	case 1:
		return cc_union_find(matrix);
	default:
		break;
	}
	return -1;
}
