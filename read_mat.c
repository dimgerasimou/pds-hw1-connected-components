#include <stdio.h>
#include <stdlib.h>
#include <matio.h>
#include <string.h>
#include "mat_read.h"

SparseMatrix *load_sparse_array(const char *filename,
                                const char *structName,
                                const char *fieldName)
{
    mat_t *matfp = Mat_Open(filename, MAT_ACC_RDONLY);
    if (!matfp) {
        fprintf(stderr, "Cannot open %s\n", filename);
        return NULL;
    }

    matvar_t *Problem = Mat_VarRead(matfp, structName);
    if (!Problem || Problem->class_type != MAT_C_STRUCT) {
        fprintf(stderr, "Struct '%s' not found\n", structName);
        Mat_Close(matfp);
        return NULL;
    }

    matvar_t *field = Mat_VarGetStructFieldByName(Problem, fieldName, 0);
    if (!field || field->class_type != MAT_C_SPARSE) {
        fprintf(stderr, "Field '%s' is not sparse or not found\n", fieldName);
        Mat_VarFree(Problem);
        Mat_Close(matfp);
        return NULL;
    }

    if (field->data_type != MAT_T_DOUBLE) {
        fprintf(stderr, "Expected double data, got type %d\n", field->data_type);
        Mat_VarFree(Problem);
        Mat_Close(matfp);
        return NULL;
    }

    if(field->rank!=2){
        fprintf(stderr,"matrix dimensions wrong");
        Mat_VarFree(Problem);
        Mat_Close(matfp);
        return NULL;

    }

    SparseMatrix *sparse = malloc(sizeof(SparseMatrix));
    if (!sparse) return NULL;

    mat_sparse_t *s = (mat_sparse_t*)field->data;

    sparse->nrow = field->dims[0];
    sparse->ncol = field->dims[1];

    sparse->ncol_ptr = sparse->ncol + 1;
    sparse->nzcount = s->jc[sparse->ncol_ptr - 1];

    sparse->row_idx = NULL;
    sparse->col_ptr = NULL;
    sparse->val = NULL;

    sparse->row_idx = malloc(sizeof(sparse->row_idx) * sparse->nzcount);
    sparse->col_ptr = malloc(sizeof(sparse->col_ptr) * sparse->ncol_ptr);
    sparse->val     = malloc(sizeof(sparse->val)     * sparse->nzcount);

    if(sparse->row_idx==NULL || sparse->col_ptr== NULL || sparse->val== NULL){
        fprintf(stderr,"malloc failed");
        free_sparse_matrix(sparse);
        Mat_VarFree(Problem);
        Mat_Close(matfp);
        return NULL;
    }

    memcpy(sparse->row_idx, s->ir, sparse->nzcount * sizeof(unsigned int));
    memcpy(sparse->col_ptr, s->jc, sparse->ncol_ptr * sizeof(unsigned int));
    memcpy(sparse->val, (double*)s->data, sparse->nzcount * sizeof(double));


    Mat_VarFree(Problem);
    Mat_Close(matfp);

    return sparse;
}

void free_sparse_matrix(SparseMatrix *sparse) {
    if (!sparse)
        return;

    if(sparse->row_idx){
        free(sparse->row_idx);
        sparse->row_idx = NULL;
    }

    if(sparse->col_ptr){
        free(sparse->col_ptr);
        sparse->col_ptr = NULL;
    }

    if(sparse->val){
        free(sparse->val);
        sparse->val = NULL;
    }

    free(sparse);
    sparse = NULL;
}
