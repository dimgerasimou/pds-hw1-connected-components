/**
 * @file json.h
 * @brief Minimal JSON parser for benchmark output
 * 
 * This parser is specifically designed for the benchmark JSON format.
 * It's simple, focused, and avoids external dependencies. The parser
 * reads JSON output from connected components benchmark programs and
 * stores the data in structured C types for easy access and manipulation.
 */

#ifndef JSON_H
#define JSON_H

#include "benchmark.h"

/**
 * @struct BenchmarkData
 * @brief Complete parsed benchmark data
 * 
 * Top-level structure containing all information from a single benchmark
 * execution, including system info, matrix info, parameters, and results.
 */
typedef struct {
	SystemInfo sys_info;          /**< System information */
	MatrixInfo matrix_info;       /**< Matrix/graph information */
	BenchmarkInfo benchmark_info; /**< Benchmark parameters */
	Result result;                /**< Algorithm result */
	int valid;                    /**< Flag indicating successful parsing */
} BenchmarkData;

/**
 * @brief Parse JSON benchmark output into structured data
 * 
 * @param json Null-terminated JSON string to parse
 * @param data Pointer to BenchmarkData structure to populate
 * @return 1 on success, 0 on parse failure
 * 
 * @note The function sets data->valid to 1 on success, 0 on failure
 * 
 * @code
 * BenchmarkData data;
 * if (parse_benchmark_data(json_output, &data)) {
 *     printf("Algorithm: %s\n", data.result.algorithm);
 *     printf("Mean time: %.6f s\n", data.result.stats.mean_time_s);
 * }
 * @endcode
 */
int parse_benchmark_data(const char *json, BenchmarkData *data);

/**
 * @brief Print system information as formatted JSON
 * 
 * @param info Pointer to SystemInfo structure to print
 * @param indent_level Number of spaces to indent the output
 * 
 * @note Output is written to stdout
 */
void print_sys_info(const SystemInfo *info, int indent_level);

/**
 * @brief Print matrix information as formatted JSON
 * 
 * @param info Pointer to MatrixInfo structure to print
 * @param indent_level Number of spaces to indent the output
 * 
 * @note Output is written to stdout
 */
void print_matrix_info(const MatrixInfo *info, int indent_level);

/**
 * @brief Print benchmark parameters as formatted JSON
 * 
 * @param info Pointer to BenchmarkInfo structure to print
 * @param indent_level Number of spaces to indent the output
 * 
 * @note Output is written to stdout
 */
void print_benchmark_info(const BenchmarkInfo *info, int indent_level);

/**
 * @brief Print algorithm result as formatted JSON
 * 
 * @param result Pointer to Result structure to print
 * @param indent_level Number of spaces to indent the output
 * 
 * @note If result->has_metrics is true, speedup and efficiency are included
 * @note Output is written to stdout
 */
void print_result(const Result *result, int indent_level);

#endif // JSON_H
