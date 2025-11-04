#ifndef MAT_READ_H
#define MAT_READ_H

#include <stddef.h>

typedef struct {
    size_t nzcount; /* Number of non-zero values in the matrix (size of val and ..)*/
    double *val; /* Array of all non-zero values in matrix (column-indexed) */

    unsigned int *row_idx; /* Array where row_idx[i] is the row of val[i] */
    unsigned int *col_ptr; /* where col_ptr[i] is the index in val of the first non-zero in column[i]*/
    size_t ncol_ptr;
    unsigned int nrow;
    unsigned int ncol;

} SparseMatrix;

SparseMatrix *load_sparse_array(const char *filename,
                                const char *structName,
                                const char *fieldName);

void free_sparse_matrix(SparseMatrix *s);

#endif
