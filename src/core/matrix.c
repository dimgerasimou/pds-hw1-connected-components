/**
 * @file matrix.c
 * @brief CSC (Compressed Sparse Column) binary matrix utilities.
 *
 * This module implements loading, storing, and printing of binary sparse
 * matrices in CSC format. Two input formats are supported:
 *
 * - **MAT files (.mat)** using MATIO, expecting a struct `Problem.A`
 *   containing a MATLAB sparse matrix.
 *
 * - **Matrix Market files (.mtx)** in `coordinate` or `array` format.
 *
 * Only binary matrices are represented. Any non-zero numeric values in
 * the input are treated as 1.
 */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <matio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matrix.h"
#include "error.h"

/* ------------------------------------------------------------------------- */
/*                            Static Helper Functions                        */
/* ------------------------------------------------------------------------- */

/**
 * @brief Count the number of digits in an integer.
 *
 * Used for formatting output in csc_print_matrix().
 *
 * @param n The integer to count digits for.
 * @return Number of digits in n.
 */
static int
num_digits(int n)
{
	if (n == 0) return 1;
	if (n < 0) n = -n;

	return (int)log10(n) + 1;
}

/**
 * @brief Load a CSC matrix from a MATLAB .mat file.
 *
 * Expects a struct named "Problem" with a sparse matrix field "A".
 * The matrix must be 2-D, real-valued, and stored in MATLAB sparse format.
 *
 * @param filename Path to the .mat file.
 * @return Newly allocated CSCBinaryMatrix on success, NULL on error.
 */
static CSCBinaryMatrix*
csc_load_matrix_mat(const char *filename)
{
	const char matrix_name[] = "Problem";
	const char field_name[]  = "A";

	mat_t *matfp = Mat_Open(filename, MAT_ACC_RDONLY);
	if (!matfp) {
		print_error(__func__, "[matio] failed to open file", errno);
		return NULL;
	}

	matvar_t *Problem = Mat_VarRead(matfp, matrix_name);
	if (!Problem || Problem->class_type != MAT_C_STRUCT) {
		print_error(__func__, "[matio] error reading struct", 0);
		if (Problem) Mat_VarFree(Problem);
		Mat_Close(matfp);
		return NULL;
	}

	matvar_t *field = Mat_VarGetStructFieldByName(Problem, field_name, 0);
	if (!field || field->class_type != MAT_C_SPARSE) {
		print_error(__func__, "[matio] field invalid type or not found", 0);
		Mat_VarFree(Problem);
		Mat_Close(matfp);
		return NULL;
	}

	if (field->data_type != MAT_T_DOUBLE) {
		print_error(__func__, "[matio] invalid datatype", 0);
		Mat_VarFree(Problem);
		Mat_Close(matfp);
		return NULL;
	}

	if (field->rank != 2 || field->dims[0] != field->dims[1]) {
		print_error(__func__, "[matio] invalid matrix dimensions", 0);
		Mat_VarFree(Problem);
		Mat_Close(matfp);
		return NULL;
	}

	CSCBinaryMatrix *m = malloc(sizeof(CSCBinaryMatrix));
	if (!m) {
		print_error(__func__, "malloc() failed", errno);
		Mat_VarFree(Problem);
		Mat_Close(matfp);
		return NULL;
	}

	mat_sparse_t *s = (mat_sparse_t*)field->data;

	m->nrows = field->dims[0];
	m->ncols = field->dims[1];
	m->nnz   = s->jc[m->ncols];

	m->row_idx = malloc(sizeof(uint32_t) * m->nnz);
	m->col_ptr = malloc(sizeof(uint32_t) * (m->ncols + 1));

	if (!m->row_idx || !m->col_ptr) {
		print_error(__func__, "malloc() failed", errno);
		csc_free_matrix(m);
		Mat_VarFree(Problem);
		Mat_Close(matfp);
		return NULL;
	}

	memcpy(m->row_idx, s->ir, m->nnz * sizeof(uint32_t));
	memcpy(m->col_ptr, s->jc, (m->ncols + 1) * sizeof(uint32_t));

	Mat_VarFree(Problem);
	Mat_Close(matfp);

	return m;
}

/**
 * @brief Skip comments and blank lines in a Matrix Market file.
 *
 * Advances the file cursor until the next non-comment, non-empty line.
 *
 * @param f Open file pointer.
 * @return Always 0.
 */
static int
mm_skip_comments(FILE *f)
{
	long pos;
	int c;

	while (1) {
		pos = ftell(f);
		c = fgetc(f);
		if (c == '%') {          /* comment line */
			while ((c = fgetc(f)) != '\n' && c != EOF);
			continue;
		} else if (isspace(c)) { /* blank line */
			continue;
		}
		fseek(f, pos, SEEK_SET);
		return 0;
	}
}

/**
 * @brief Load a CSC matrix from a Matrix Market (.mtx) file.
 *
 * Supports the following formats:
 *
 * - coordinate or array
 * - pattern or real-valued
 * - general, symmetric, skew-symmetric, hermitian
 *
 * Only non-zero entries are stored (binary interpretation).
 *
 * @param filename Path to the .mtx file.
 * @return Newly allocated CSCBinaryMatrix on success, NULL on error.
 */
static CSCBinaryMatrix*
csc_load_matrix_mtx(const char *filename)
{
	FILE *f = fopen(filename, "r");
	if (!f) {
		print_error(__func__, "failed to open .mtx file", errno);
		return NULL;
	}

	/* --- Header -------------------------------------------------------- */
	char format[64], field[64], symmetry[64];

	if (fscanf(f, "%%%%MatrixMarket matrix %63s %63s %63s",
				format, field, symmetry) != 3)
	{
		print_error(__func__, "invalid MatrixMarket header", 0);
		fclose(f);
		return NULL;
	}

	int is_coordinate = (strcmp(format, "coordinate") == 0);
	//int is_array      = (strcmp(format, "array") == 0);

	int is_pattern = (strcmp(field, "pattern") == 0);

	int symmetric   = (strcmp(symmetry, "symmetric") == 0);
	int skew        = (strcmp(symmetry, "skew-symmetric") == 0);
	int hermitian   = (strcmp(symmetry, "hermitian") == 0);
	int general     = (strcmp(symmetry, "general") == 0);

	if (!general && !symmetric && !skew && !hermitian) {
		print_error(__func__, "unsupported symmetry", 0);
		fclose(f);
		return NULL;
	}

	/* --- Sizes --------------------------------------------------------- */
	mm_skip_comments(f);

	size_t nrows, ncols, nnz;
	if (is_coordinate) {
		if (fscanf(f, "%zu %zu %zu", &nrows, &ncols, &nnz) != 3) {
			print_error(__func__, "invalid size line", 0);
			fclose(f);
			return NULL;
		}
	} else { /* array */
		if (fscanf(f, "%zu %zu", &nrows, &ncols) != 2) {
			print_error(__func__, "invalid array size line", 0);
			fclose(f);
			return NULL;
		}
		nnz = nrows * ncols; /* will filter zeroes later */
	}

	/* Allocate temporary COO arrays */
	size_t max_nnz = nnz * (symmetric ? 2 : 1) + 5;
	uint32_t *coo_i = malloc(max_nnz * sizeof(uint32_t));
	uint32_t *coo_j = malloc(max_nnz * sizeof(uint32_t));
	if (!coo_i || !coo_j) {
		print_error(__func__, "malloc failed", errno);
		fclose(f);
		free(coo_i); free(coo_j);
		return NULL;
	}

	size_t count = 0;

	/* --- Read entries -------------------------------------------------- */
	if (is_coordinate) {
		/* i j [value] */
		for (size_t k = 0; k < nnz; k++) {
			size_t i, j;
			double val = 1.0;

			if (is_pattern) {
				if (fscanf(f, "%zu %zu", &i, &j) != 2) {
					print_error(__func__, "bad coordinate entry", 0);
					goto fail;
				}
			} else {
				if (fscanf(f, "%zu %zu %lf", &i, &j, &val) != 3) {
					print_error(__func__, "bad coordinate entry", 0);
					goto fail;
				}
			}

			if (val != 0.0) {
				coo_i[count] = i - 1;
				coo_j[count] = j - 1;
				count++;

				if (symmetric && i != j) {
					coo_i[count] = j - 1;
					coo_j[count] = i - 1;
					count++;
				}
			}
		}
	}
	else {
		/* array: dense stored column-major */
		for (size_t j = 0; j < ncols; j++) {
			for (size_t i = 0; i < nrows; i++) {
				double val;
				if (fscanf(f, "%lf", &val) != 1)
					goto fail;

				if (val != 0.0) {
					coo_i[count] = i;
					coo_j[count] = j;
					count++;
				}
			}
		}
	}

	fclose(f);

	/* --- Convert COO → CSC binary -------------------------------------- */
	CSCBinaryMatrix *m = malloc(sizeof(CSCBinaryMatrix));
	if (!m) goto fail;

	m->nrows = nrows;
	m->ncols = ncols;
	m->nnz   = count;

	m->row_idx = malloc(count * sizeof(uint32_t));
	m->col_ptr = malloc((ncols + 1) * sizeof(uint32_t));

	if (!m->row_idx || !m->col_ptr) {
		print_error(__func__, "malloc failed", errno);
		csc_free_matrix(m);
		goto fail;
	}

	/* count entries per column */
	memset(m->col_ptr, 0, (ncols + 1) * sizeof(uint32_t));

	for (size_t k = 0; k < count; k++)
		m->col_ptr[coo_j[k] + 1]++;

	for (size_t j = 0; j < ncols; j++)
		m->col_ptr[j+1] += m->col_ptr[j];

	/* fill rows */
	uint32_t *col_fill = calloc(ncols, sizeof(uint32_t));
	if (!col_fill) goto fail2;

	for (size_t k = 0; k < count; k++) {
		uint32_t j = coo_j[k];
		uint32_t dest = m->col_ptr[j] + col_fill[j];
		m->row_idx[dest] = coo_i[k];
		col_fill[j]++;
	}

	free(col_fill);
	free(coo_i);
	free(coo_j);

	return m;

fail2:
	csc_free_matrix(m);    

fail:
	fclose(f);
	free(coo_i);
	free(coo_j);
	return NULL;
}

/**
 * @brief Case-insensitive filename extension match.
 *
 * @param filename Path to check.
 * @param ext Expected extension (without dot).
 * @return 1 if matched, 0 otherwise.
 */
static int
ext_is(const char *filename, const char *ext)
{
	const char *dot = strrchr(filename, '.');
	if (!dot) return 0;
	dot++;

	while (*dot && *ext) {
		if (tolower((unsigned char)*dot) != tolower((unsigned char)*ext))
			return 0;
		dot++; ext++;
	}

	return (*dot == '\0' && *ext == '\0');
}


/* ------------------------------------------------------------------------- */
/*                           Public API Functions                             */
/* ------------------------------------------------------------------------- */

/**
 * @brief Load a sparse binary matrix from a .mat or .mtx file.
 *
 * Automatically dispatches to:
 * - csc_load_matrix_mtx() if the file ends in ".mtx"
 * - csc_load_matrix_mat() if the file ends in ".mat"
 *
 * @param path Path to the matrix file.
 * @return Newly allocated CSCBinaryMatrix, or NULL on failure.
 */
CSCBinaryMatrix*
csc_load_matrix(const char *path)
{
	if (ext_is(path, "mtx")) {
		return csc_load_matrix_mtx(path);
	}
	else if (ext_is(path, "mat")) {
		return csc_load_matrix_mat(path);
	} else {
		print_error(__func__, "Unrecognized matrix file extention", 0);
	}
	
	return NULL;
}

/**
 * @brief Free a CSCBinaryMatrix and its associated memory.
 *
 * Safe to call with NULL.
 *
 * @param m CSC matrix to free.
 */
void
csc_free_matrix(CSCBinaryMatrix *m)
{
	if (!m)
		return;

	if(m->row_idx){
		free(m->row_idx);
		m->row_idx = NULL;
	}

	if(m->col_ptr){
		free(m->col_ptr);
		m->col_ptr = NULL;
	}

	free(m);
	m = NULL;
}

/**
 * @brief Print a sparse binary matrix in coordinate format.
 *
 * @param m Pointer to the CSCBinaryMatrix to print.
 *
 * @note Prints as (row, col) pairs. Indices are 1-based.
 */
void
csc_print_matrix(CSCBinaryMatrix *m)
{
	int di = 0;
	int dj = 0;
	unsigned int nline = 1;
	const unsigned int maxline = 10;

	di = num_digits(m->nrows);
	dj = num_digits(m->ncols);

	printf("Binary Sparse Matrix:\nN:%zu, M:%zu, Non-Zero Elements:%zu\n\n", m->nrows, m->ncols, m->nnz);

	for (size_t i = 0; i < m->ncols; i++) {
		for(size_t j = m->col_ptr[i]; j < m->col_ptr[i+1]; j++) {
			printf("(%*d,%*d)", di, m->row_idx[j] + 1, dj, (int) i + 1);

			if (nline < maxline) {
				printf(" ");
			} else {
				printf("\n");
				nline = 0;
			}

			nline++;
		}
	}

	printf("\n");
}
