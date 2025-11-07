#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "cc_union_find_like.h"
#include "connected_comp_omp.h"
#include <sys/time.h>

const char *program_name = "pardisV0";

int main(int argc, char *argv[]) {

    CSCBinaryMatrix *matrix;

    set_program_name(argv[0]);

    if (argc != 2) {
        print_error(__func__, "invalid arguments", 0);
        return 1;
    }
    
    matrix = csc_load_matrix(argv[1], "Problem", "A");
    if (!matrix)
        return 1;

  
    printf("MAX THREADS:%d\n", omp_get_max_threads());
    clock_t start, end;
    struct timeval startt, endt;

    double elapsed_seq[10];
    double elapsed_omp[10];

    int cpu_cycles_used_omp[10];
    int cpu_cycles_used_seq[10];

    int num_components_omp;
    int num_components_seq;

    for (int i = 0; i < 10; i++) {

        start = clock();
        gettimeofday(&startt, NULL);
        num_components_omp = connected_comp_omp(matrix);
        gettimeofday(&endt, NULL);
        end = clock();
        elapsed_omp[i] = (endt.tv_sec - startt.tv_sec) + 
                         (endt.tv_usec - startt.tv_usec) / 1e6;
        cpu_cycles_used_omp[i] = ((double)(end-start));

        
        
        start = clock();
        gettimeofday(&startt, NULL);
        num_components_seq = cc_union_find(matrix);
        gettimeofday(&endt, NULL);
        end = clock();
        elapsed_seq[i] = (endt.tv_sec - startt.tv_sec) + 
                         (endt.tv_usec - startt.tv_usec) / 1e6;
        cpu_cycles_used_seq[i] = ((double)(end-start));


    }

    int avg_cpu_cycles_used_omp = 0 ;
    int avg_cpu_cycles_used_seq = 0 ;
    double avg_elapsed_omp = 0 ;
    double avg_elapsed_seq = 0 ;

    for(int i=0;i<10;i++){
        avg_cpu_cycles_used_omp += cpu_cycles_used_omp[i];
        avg_cpu_cycles_used_seq += cpu_cycles_used_seq[i];
        avg_elapsed_omp += elapsed_omp[i];
        avg_elapsed_seq += elapsed_seq[i];
    }

    avg_cpu_cycles_used_omp /= 10;
    avg_cpu_cycles_used_seq /= 10;
    avg_elapsed_omp /= 10;
    avg_elapsed_seq /= 10;

    printf("Number of connected components with seqential union-find: %d, average cycles needed: %d, average time needed %lf\n", num_components_seq, avg_cpu_cycles_used_seq, avg_elapsed_seq);

    printf("Number of connected components with  parallel union-find: %u, time needed: %d, average time needed %lf\n", num_components_omp, avg_cpu_cycles_used_omp,avg_elapsed_omp);

    csc_free_matrix(matrix);
    return 0;
}
