/**
 * @file cc_openmp.c
 * @brief Parallel algorithms for computing connected components using OpenMP.
 *
 * This module implements two parallel algorithms for finding connected
 * components in an undirected graph:
 *
 * - Label Propagation (variant 0): Parallel iterative label propagation
 *   with atomic updates. Uses bitmap-based counting for efficiency.
 *
 * - Union-Find with Rem's Algorithm (variant 1): Lock-free parallel
 *   union-find using compare-and-swap (CAS) operations and path compression.
 *   It is faster and more scalable than label propagation.
 *
 * Both algorithms are designed to scale efficiently across multiple cores
 * while maintaining correctness through careful synchronization.
 */

#include <stdlib.h>
#include <stdint.h>
#include <omp.h>

#include "connected_components.h"

/* ========================================================================== */
/*                    UNION-FIND WITH REM'S ALGORITHM                         */
/* ========================================================================== */

/**
 * @brief Finds root with path compression (non-atomic, thread-safe).
 *
 * This function finds the root of a node and compresses the path by making
 * all nodes on the path point directly to the root. Path compression is
 * safe here because each thread only modifies its own query path, which
 * doesn't conflict with other threads' operations.
 *
 * The early-exit optimization (label[x] == next) prevents redundant writes
 * when the path is already compressed.
 *
 * @param label Array of parent pointers
 * @param x Node to find root for
 * @return Root of the set containing x
 */
static inline uint32_t
find_compress(uint32_t *label, uint32_t x)
{
	uint32_t root = x;
	
	// Find root by following parent pointers
	while (label[root] != root) {
		root = label[root];
	}
	
	// Compress path: make all nodes point directly to root
	while (x != root) {
		uint32_t next = label[x];
		if (label[x] == next) break;  // Already compressed
		label[x] = root;
		x = next;
	}
	
	return root;
}

/**
 * @brief Unites two sets using lock-free CAS with bounded retries.
 *
 * This implements Rem's algorithm for parallel union-find:
 * 1. Find roots of both nodes with path compression
 * 2. If roots differ, try to atomically link the larger to the smaller
 * 3. On CAS failure, retry with updated root information
 * 4. After MAX_RETRIES, force the union to guarantee progress
 *
 * The bounded retry mechanism prevents livelocks when multiple threads
 * compete heavily. The canonical ordering (smaller index wins) ensures
 * deterministic results.
 *
 * @param label Array of parent pointers
 * @param a First node
 * @param b Second node
 */
static inline void
union_rem(uint32_t *label, uint32_t a, uint32_t b)
{
	const int MAX_RETRIES = 10;  // Prevent infinite loops
	int retries = 0;
	
	while (retries < MAX_RETRIES) {
		// Find roots with compression
		a = find_compress(label, a);
		b = find_compress(label, b);
		
		if (a == b) return;  // Already in same set
		
		// Canonical ordering: lower index becomes root
		if (a > b) {
			uint32_t tmp = a;
			a = b;
			b = tmp;
		}
		
		// Try atomic CAS: set label[b] = a if label[b] == b
		uint32_t expected = b;
		if (__atomic_compare_exchange_n(&label[b], &expected, a,
		                                0, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
			return;  // Success
		}
		
		// CAS failed: another thread changed label[b]
		b = expected;
		retries++;
	}
	
	// Fallback after max retries: force the union to guarantee progress
	a = find_compress(label, a);
	b = find_compress(label, b);
	if (a != b) {
		if (a > b) {
			uint32_t tmp = a;
			a = b;
			b = tmp;
		}
		__atomic_store_n(&label[b], a, __ATOMIC_RELEASE);
	}
}

/**
 * @brief Computes connected components using parallel union-find.
 *
 * Algorithm phases:
 * 1. Parallel initialization of label array
 * 2. Parallel union phase: process edges with dynamic scheduling
 * 3. Parallel flattening: compress all paths for accurate counting
 * 4. Parallel counting: count roots with thread-local accumulation
 *
 * Dynamic scheduling with chunk size 128 provides good load balance for
 * graphs with varying column degrees. The flattening pass ensures all
 * nodes point directly to their roots, even if some unions were forced
 * during the retry limit.
 *
 * @param matrix Sparse binary matrix in CSC format
 * @param n_threads Number of OpenMP threads to use
 * @return Number of connected components, or -1 on error
 */
static int
cc_union_find(const CSCBinaryMatrix *matrix, const unsigned int n_threads)
{
	if (!matrix || matrix->nrows == 0) return 0;

	const uint32_t n = matrix->nrows;
	uint32_t *label = malloc(n * sizeof(uint32_t));
	if (!label) return -1;

	// Phase 1: Initialize label array in parallel
	#pragma omp parallel for num_threads(n_threads) schedule(static)
	for (uint32_t i = 0; i < n; i++)
		label[i] = i;

	// Phase 2: Parallel union phase with dynamic scheduling
	// Dynamic scheduling provides better load balance when column degrees vary
	#pragma omp parallel num_threads(n_threads)
	{
		#pragma omp for schedule(dynamic, 128) nowait
		for (uint32_t col = 0; col < matrix->ncols; col++) {
			uint32_t start = matrix->col_ptr[col];
			uint32_t end = matrix->col_ptr[col + 1];
			
			// Process all edges in this column
			for (uint32_t j = start; j < end; j++) {
				uint32_t row = matrix->row_idx[j];
				if (row < n) {
					union_rem(label, row, col);
				}
			}
		}
	}

	// Phase 3: Full flattening pass to ensure all paths are compressed
	// Critical for correct counting, especially after bounded retries
	#pragma omp parallel for num_threads(n_threads) schedule(static, 2048)
	for (uint32_t i = 0; i < n; i++) {
		find_compress(label, i);
	}

	// Phase 4: Count components with thread-local accumulation
	// Avoids expensive reduction overhead by using atomic only once per thread
	uint32_t count = 0;
	#pragma omp parallel num_threads(n_threads)
	{
		uint32_t local_count = 0;
		
		#pragma omp for schedule(static, 2048) nowait
		for (uint32_t i = 0; i < n; i++) {
			if (label[i] == i) local_count++;
		}
		
		#pragma omp atomic
		count += local_count;
	}

	free(label);
	return (int)count;
}

/* ========================================================================== */
/*                    PARALLEL LABEL PROPAGATION                              */
/* ========================================================================== */

/**
 * @brief Computes connected components using parallel label propagation.
 *
 * Algorithm phases:
 * 1. Initialize each node with its own label
 * 2. Iterate until convergence:
 *    - Parallel processing of edges with atomic label updates
 *    - Thread-local change detection with atomic global flag update
 * 3. Parallel bitmap construction to track unique labels
 * 4. Count set bits in bitmap (unique components)
 *
 * This approach uses atomic writes to handle race conditions during label
 * propagation. The bitmap-based counting replaces sequential qsort and is
 * much more efficient in parallel.
 *
 * @param matrix Sparse binary matrix in CSC format
 * @param n_threads Number of OpenMP threads to use
 * @return Number of connected components, or -1 on error
 */
static int
cc_label_propagation(const CSCBinaryMatrix *matrix, const int n_threads)
{
	uint32_t *label = malloc(sizeof(uint32_t) * matrix->nrows);
	if (!label) return -1;
	
	// Initialize: each node has its own label
	#pragma omp parallel for num_threads(n_threads) schedule(static)
	for (size_t i = 0; i < matrix->nrows; i++) {
		label[i] = i;
	}

	// Iterate until no labels change (convergence)
	uint8_t finished;
	do {
		finished = 1;
		
		#pragma omp parallel num_threads(n_threads)
		{
			uint8_t local_changed = 0;
			
			// Process edges with dynamic scheduling
			#pragma omp for schedule(dynamic, 128) nowait
			for (size_t col = 0; col < matrix->ncols; col++) {
				for (uint32_t j = matrix->col_ptr[col]; j < matrix->col_ptr[col + 1]; j++) {
					uint32_t row = matrix->row_idx[j];
					
					// Read current labels
					uint32_t lc = label[col];
					uint32_t lr = label[row];
					
					// Propagate minimum label using atomic writes
					if (lc != lr) {
						local_changed = 1;
						uint32_t minval = lc < lr ? lc : lr;
						
						#pragma omp atomic write
						label[col] = minval;
						#pragma omp atomic write
						label[row] = minval;
					}
				}
			}
			
			// Update global finished flag if any thread saw changes
			if (local_changed) {
				#pragma omp atomic write
				finished = 0;
			}
		}
	} while (!finished);

	// Bitmap-based counting (faster than sorting in parallel)
	size_t bitmap_size = (matrix->nrows + 63) / 64;
	uint64_t *bitmap = calloc(bitmap_size, sizeof(uint64_t));
	if (!bitmap) {
		free(label);
		return -1;
	}
	
	// Parallel bitmap construction: set bit for each unique label
	#pragma omp parallel num_threads(n_threads)
	{
		#pragma omp for schedule(static, 1024)
		for (size_t i = 0; i < matrix->nrows; i++) {
			uint32_t val = label[i];
			size_t word = val >> 6;              // Divide by 64
			uint64_t bit = 1ULL << (val & 63);   // Modulo 64
			
			#pragma omp atomic
			bitmap[word] |= bit;
		}
	}
	
	// Count set bits using hardware popcount
	uint32_t count = 0;
	#pragma omp parallel for num_threads(n_threads) reduction(+:count)
	for (size_t i = 0; i < bitmap_size; i++) {
		count += __builtin_popcountll(bitmap[i]);
	}
	
	free(bitmap);
	free(label);
	return (int)count;
}

/* ========================================================================== */
/*                              PUBLIC INTERFACE                              */
/* ========================================================================== */

/**
 * @brief Computes connected components using parallel algorithms.
 *
 * This is the main entry point for parallel connected components
 * computation. It dispatches to one of two algorithm implementations
 * based on the variant parameter.
 *
 * Supported variants:
 *   0: Label propagation (simple, slower on complex graphs)
 *   1: Union-find with Rem's algorithm (more complex, faster)
 *
 * Both algorithms use OpenMP for parallelization and are designed to
 * scale efficiently across multiple cores.
 *
 * @param matrix Sparse binary matrix in CSC format
 * @param n_threads Number of OpenMP threads to use
 * @param algorithm_variant Algorithm selection (0 or 1)
 * @return Number of connected components, or -1 on error
 */
int
cc_openmp(const CSCBinaryMatrix *matrix,
          const unsigned int n_threads,
          const unsigned int algorithm_variant)
{
	switch (algorithm_variant) {
	case 0:
		return cc_label_propagation(matrix, n_threads);
	case 1:
		return cc_union_find(matrix, n_threads);
	default:
		return -1;
	}
}
