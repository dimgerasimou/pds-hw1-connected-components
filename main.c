#include <stdio.h>
#include "mat_read.h"

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("%s: invalid arguments", argv[0]);
        return 1;
    }

    SparseMatrix *sparse = load_sparse_array(argv[1], "Problem", "A");

    if (!sparse) {
        fprintf(stderr, "Failed to load sparse array\n");
        return 1;
    }

    for (unsigned int i=0; i<sparse->ncol; i++){
        for(unsigned int j= sparse->col_ptr[i]; j<sparse->col_ptr[i+1];j++) {
            printf("(%5d,%d)  %.0lf\n",sparse->row_idx[j] + 1, i + 1, sparse->val[j]);
        }
    }

    free_sparse_matrix(sparse);
    return 0;
}
