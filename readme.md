<h1 align="center">Connected Components: Parallel Implementations</h1>
<h3 align="center">Parallel and Distributed Systems</h3>

<p align="center">
  <em>Department of Electrical and Computer Engineering</em><br>
  <em>Aristotle University of Thessaloniki</em><br>
  <strong>Homework #1</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/language-C-blue.svg" alt="Language">
  <img src="https://img.shields.io/badge/build-Makefile-success" alt="Build">
  <img src="https://img.shields.io/badge/platform-Linux%20%7C%20Unix-lightgrey" alt="Platform">
</p>

---

## Overview

This project implements and benchmarks **connected components algorithms** for sparse graphs using different parallel programming paradigms. It provides comprehensive performance analysis including execution time, throughput, speedup, and efficiency metrics across different parallelization strategies.

### Key Features

- **Eight implementations** of the connected components algorithm
- **Automated benchmarking** with statistical analysis (mean, median, std dev, min/max)
- **Performance metrics**: speedup, efficiency, throughput (edges/sec)
- **Memory tracking**: peak memory usage for each implementation
- **JSON output format** for easy integration with analysis tools
- **Matrix Market / MAT-file format support** via libmatio

### Implementations

These are the algorithm types implemented:

- **Label Propagation**: Iteratively propagates minimum labels until convergence with early termination optimization. Uses bitmaps for counting the individual components.
- **Union-Find**: Uses disjoint-set data structure with path   halving optimization. Generally faster and more scalable.

Both algorithm types are implemented **using three parallelization methods and one sequential**, as follows:
- **Sequential**
- **OpenMP**
- **POSIX Threads**
- **OpenCilk**


---

## Table of Contents

- [Dependencies](#dependencies)
- [Installation](#installation)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
  - [Quick Start](#quick-start)
  - [Save Results to File](#save-results-to-file)
  - [Compare Variants](#compare-variants)
  - [Benchmark Runner](#benchmark-runner)
  - [Individual Algorithms](#individual-algorithms)
- [Performance Results](#performance-results)
- [Project Structure](#project-structure)
- [Maintenance Commands](#maintenance-commands)
- [Troubleshooting](#troubleshooting)

---

## Dependencies

### Required

| Dependency | Purpose | Installation |
|------------|---------|--------------|
| `gcc` | GNU C compiler | Usually pre-installed |
| `clang` (OpenCilk) | OpenCilk compiler | See [OpenCilk installation](https://www.opencilk.org/) |
| `libmatio` | MAT-file (.mat) file reading | `libmatio` though your package manager |
| `pthread` | POSIX threading | usually part of `glibc` |
| `openmp` | OpenMP support | `openmp` though your package manager |

### Optional

| Dependency | Purpose | Installation |
|------------|---------|--------------|
| `tree` | Project structure visualization | `sudo apt install tree` |

### Verify Dependencies

Check your setup using:

```bash
make check-deps
```

This command verifies the availability of all required compilers and libraries.

---

## Installation

1. Clone or download this repository

2. Ensure OpenCilk is installed and update the `CILK_PATH` variable in the **Makefile** to point to your OpenCilk installation:

```makefile
# Example paths (edit as needed)
CILK_PATH = /opt/opencilk
# or
CILK_PATH = /usr/local/opencilk
```

---

## Build Instructions

### Build All Targets

To compile all implementations and the benchmark runner:

```bash
make
```

This creates executables in the `bin/` directory:
- `bin/connected_components_sequential`
- `bin/connected_components_openmp`
- `bin/connected_components_pthreads`
- `bin/connected_components_cilk`
- `bin/benchmark_runner`

### Build Specific Implementations

```bash
make sequential   # Sequential baseline version
make openmp       # OpenMP parallel version
make pthreads     # Pthreads parallel version
make cilk         # OpenCilk parallel version
make runner       # Benchmark runner only
```

### Clean and Rebuild

```bash
make clean        # Remove all build artifacts
make rebuild      # Clean and build everything
```

---

## Usage

### Quick Start

The simplest way to benchmark all implementations:

```bash
make benchmark MATRIX=data/soc-LiveJournal1.mtx TRIALS=10 THREADS=8 VARIANT=1
```

**Parameters:**
- `MATRIX`: Path to input matrix file (Matrix Market / MAT-file format)
- `TRIALS`: Number of runs for each algorithm (default: 10)
- `THREADS`: Number of threads to use (default: 8)
- `VARIANT`: Variant of implementation to run (default: 0)

**Output:** JSON results printed to stdout

### Save Results to File

To automatically save results with a timestamp:

```bash
make benchmark-save MATRIX=data/soc-LiveJournal1.mtx TRIALS=10 THREADS=8 VARIANT=1
```

This saves output to `benchmarks/benchmark-result-YYYYMMDD_HHMMSS.json`

Example output file:
```
benchmarks/benchmark-result-20251122_190950.json
``` 

### Compare Variants
To compare variants with given arguments:
```bash
make benchmark-compare MATRIX=data/soc-LiveJournal1.mtx TRIALS=10 THREADS=8
```

This saves the results to `benchmarks/comparison-YYYYMMDD_HHMMSS/variant{0,1}.json`

Example result files:
```
benchmarks/comparison-20251122_190950/variant0.json
benchmarks/comparison-20251122_190950/variant1.json
``` 

### Benchmark Runner

Run all algorithms manually with custom parameters (it's time consuming though and I have already spent a lot of time on the `make benchmark` targets to even run it manually):

```bash
bin/benchmark_runner -v <variant> -t <threads> -n <trials> path/to/matrix.mtx
```

**Example:**

```bash
bin/benchmark_runner -v 0 -t 8 -n 10 data/soc-LiveJournal1.mtx
```

**Options:**
- `-t <threads>` — Number of threads (default: 8)
- `-n <trials>` — Number of benchmark trials (default: 3)
- `-v <variant>` — Algorithm variant to benchmark (default: 0)
- `-h` — Display help message

### Individual Algorithms

You can also run each implementation separately:

```bash
# Sequential baseline
bin/connected_components_sequential -v 0 -n 10 data/matrix.mtx

# OpenMP version
bin/connected_components_openmp -v 0 -t 8 -n 10 data/matrix.mtx

# Pthreads version
bin/connected_components_pthreads -v 0 -t 8 -n 10 data/matrix.mtx

# OpenCilk version
CILK_NWORKERS=8 bin/connected_components_cilk -v 0 -t 8 -n 10 data/matrix.mtx
```

**Common Options:**
- `-t <threads>` — Number of threads
- `-n <trials>` — Number of runs
- `-v <variant>` — Algorithm variant to run
- `-h` — Help message

---

## Performance Results

### Example Benchmark: Large Social Network Graph

**Dataset:** LiveJournal social network (com-LiveJournal.mtx)
- **Nodes:** 3,997,962
- **Edges:** 69,362,378
- **System:** Intel Core i7-11800H @ 2.30GHz, 8 threads
- **Connected Components Found:** 1

#### **Algorithm Type:** Label Propagation

| Algorithm  | Mean Time (s) | Throughput (Medges/s) | Speedup | Efficiency |
|------------|---------------|-----------------------|---------|------------|
| Sequential | 0.4392        | 157.9                 | 1.00×   | 100%       |
| OpenMP     | 0.1031        | 672.1                 | 4.26×   | 53.2%      |
| Pthreads   | 0.1101        | 630.2                 | 3.99×   | 49.9%      |
|**OpenCilk**| **0.0859**    | **807.3**             |**5.11×**| **63.9%**  |

---

#### **Algorithm Type:** Union Find

| Algorithm  | Mean Time (s) | Throughput (Medges/s) | Speedup | Efficiency |
|------------|---------------|-----------------------|---------|------------|
| Sequential | 0.1800        | 385.3                 | 1.00×   | 100%       |
| **OpenMP** | **0.0472**    | **1469.9**            |**3.82×**| **47.7%**  |
| Pthreads   | 0.0492        | 1411.2                | 3.66×   | 45.8%      |
| OpenCilk   | 0.0528        | 1314.2                | 3.41×   | 42.6%      |

### Metrics Explained

- **Mean Time:** Average execution time across all trials
- **Throughput:** Edges processed per second (higher is better)
- **Speedup:** Performance improvement over sequential (`T_sequential` / `T_parallel`)
- **Efficiency:** How well parallelism is utilized (`Speedup` / `Number of threads`)

---

## Project Structure

```
connected_components/
├── src/
│   ├── algorithms/      # Implementations: sequential, OpenMP, Pthreads, Cilk
│   ├── core/            # Matrix data structures (CSC format, etc.)
│   ├── utils/           # Helpers: JSON, timing, parsing, logging
│   ├── main.c           # Entry point for algorithms
│   └── runner.c         # Benchmark runner
├── Makefile             # Build automation
└── README.md
```

### View Interactive Tree

```bash
make tree
```

---

## Maintenance Commands

| Command | Description |
|---------|-------------|
| `make info` | Display build configuration and paths |
| `make list-sources` | List all `.c` source files in the project |
| `make list-binaries` | Show all compiled executables |
| `make tree` | Display project structure (requires `tree`) |
| `make check-deps` | Verify all dependencies are installed |
| `make clean` | Remove all build artifacts and binaries |
| `make rebuild` | Clean and rebuild everything from scratch |

---

## Troubleshooting

### OpenCilk Compiler Not Found

**Error:** `clang: command not found` or OpenCilk-specific errors

**Solution:** 
1. Install OpenCilk from [opencilk.org](https://www.opencilk.org/)
2. Update `CILK_PATH` in the Makefile to your installation directory
3. Verify installation: `/path/to/opencilk/bin/clang --version`

### Matrix File Not Found

**Error:** `Cannot open matrix file`

**Solution:**
- Ensure the matrix file exists in the specified path
- Check file permissions: `ls -la data/`
- Verify the file is in Matrix Market or MAT-file (.mtx/.mat) format

---

<p align="center">
  <sub>November 2025 • Aristotle University of Thessaloniki</sub>
</p>
