# Connected Components — Shared-Memory Parallel Implementations

<p align="center">
  <img src="https://img.shields.io/badge/parallelism-OpenMP%20%7C%20Pthreads%20%7C%20OpenCilk-blue">
</p>

Assignment #1 of the **Parallel and Distributed Systems** coursework:
[parallel-distributed-systems](https://github.com/dimgerasimou/parallel-distributed-systems)


Parallel implementations of the **connected components** problem for sparse graphs, focusing on **shared-memory parallelism**, multiple algorithmic variants, and **performance benchmarking**.

This project evaluates connected components algorithms in C under different shared-memory programming models, with automated measurement of execution time, throughput, speedup, and efficiency.

## Overview

The goal of this project is to study the performance characteristics of connected components algorithms under different shared-memory programming models.

It includes:
- Multiple algorithmic variants
- Multiple shared-memory parallelization approaches
- A unified benchmarking framework with statistical analysis

## Implementations

### Algorithms
- **Label Propagation**
  - Iterative label relaxation until convergence
  - Early termination and bitmap-based component counting
- **Union-Find**
  - Disjoint-set structure with path halving
  - Typically faster and more scalable

### Parallelization Models
Each algorithm is implemented using:
- **Sequential** (baseline)
- **OpenMP**
- **POSIX Threads**
- **OpenCilk**

## Features

- Multiple parallel connected components implementations
- Automated benchmarking with configurable trials and threads
- Performance metrics:
  - Execution time
  - Throughput (edges/sec)
  - Speedup and efficiency
- Peak memory usage tracking
- Machine-readable **JSON output** for analysis
- Matrix Market (`.mtx`) and MAT-file (`.mat`) input support

## Build

### Requirements
- C compiler (`gcc` or `clang`)
- OpenMP
- POSIX threads
- OpenCilk (for Cilk variant)
- `libmatio` (for `.mat` file support)

Ensure OpenCilk is installed and update the `CILK_PATH` variable in the **Makefile** to point to your OpenCilk installation:

```makefile
# Example paths (edit as needed)
CILK_PATH = /opt/opencilk
# or
CILK_PATH = /usr/local/opencilk
```

### Compile all targets
```bash
make
```

Executables are placed in `bin/`.

## Usage

### Available options
Run to see all available options:
```bash
make help
```

### Quick benchmark
```bash
make benchmark MATRIX=data/soc-LiveJournal1.mtx THREADS=8 TRIALS=10
```

Results are printed in JSON format to stdout.

### Save results to file
```bash
make benchmark-save MATRIX=data/soc-LiveJournal1.mtx THREADS=8 TRIALS=10
```

Output is stored in `benchmarks/` with a timestamp.

### Compare Variants
```bash
make benchmark-compare MATRIX=data/soc-LiveJournal1.mtx TRIALS=10 THREADS=8
```

Output is stored in `benchmarks/` with a timestamp and the version.

### Manual execution
```bash
bin/benchmark_runner -v 0 -t 8 -n 10 data/matrix.mtx
```

## Project Structure

```
src/
├── algorithms/   # Sequential, OpenMP, Pthreads, OpenCilk
├── core/         # Matrix representations and utilities
├── utils/        # Benchmarking, JSON output, helpers
├── main.c        # Algorithm entry point
└── runner.c      # Benchmark runner
```

## Performance Results

Detailed benchmark results and tables are available in: [Performance results and analysis](docs/performance.md).

Raw benchmark outputs are generated automatically and stored under `benchmarks/`.

## Notes

- Designed for shared-memory systems
- Emphasis on performance comparison rather than algorithmic novelty
- Intended for experimentation and benchmarking on Linux systems

---

<p align="center"><sub>November 2025 • Aristotle University of Thessaloniki</sub></p>
