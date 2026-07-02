# 15-Tile Sliding Puzzle Solver

A high-performance sliding puzzle solver built in **C++17** using **Breadth-First Search (BFS)** and **A\* Search** with the Manhattan distance heuristic. Designed with clean OOP architecture, verified correctness, and benchmarked performance across scramble depths up to 40 moves.

---

## What is the 15-Puzzle?

A 4×4 grid containing 15 numbered tiles and one blank cell. Tiles slide into the blank to rearrange the board. The goal is to reach this configuration:

```
  1   2   3   4
  5   6   7   8
  9  10  11  12
 13  14  15   .
```

Every move slides a tile adjacent to the blank into the blank's position — equivalently, **the blank moves up, down, left, or right**, swapping with whatever tile is there. This reframing is what makes move generation simple: the blank's neighbors are just ±1 (left/right) and ±4 (up/down) in a flattened 16-cell array.

---

## Project Structure

```
15-Puzzle-Tile-Solver/
│
├── include/
│   ├── puzzle_state.hpp      # Board representation, move generation, hashing
│   ├── solvability.hpp       # Inversion-count solvability check
│   ├── isolver.hpp           # Common solver interface (ISolver + Solution)
│   ├── bfs_solver.hpp        # BFS solver declaration
│   ├── heuristic.hpp         # Heuristic interface + ManhattanDistance
│   └── astar_solver.hpp      # A* solver declaration
│
├── src/
│   ├── puzzle_state.cpp      # Board logic and bit-packing implementation
│   ├── solvability.cpp       # Merge-sort inversion counting
│   ├── bfs_solver.cpp        # BFS implementation with path reconstruction
│   ├── heuristic.cpp         # Manhattan distance computation
│   ├── astar_solver.cpp      # A* implementation with priority queue
│   ├── main.cpp              # CLI entry point
│   └── benchmark.cpp         # Benchmarking harness
│
├── Tests/
│   ├── bfs_depth_test.cpp    # BFS depth stress test
│   ├── test_puzzle_State.cpp # PuzzleState unit tests
│   ├── test_solvability.cpp  # Solvability unit tests
│   ├── test_bfs_solver.cpp   # BFS solver unit tests
│   ├── test_heuristic.cpp    # Heuristic unit tests
│   └── test_astar_solver.cpp # A* solver unit tests
│
├── CMakelists.txt
└── .gitignore
```

---

## How Files Connect

```
main.cpp (CLI entry point)
    │
    ├── PuzzleState        → validates board, generates moves, hashes states
    │   [puzzle_state.hpp / puzzle_state.cpp]
    │
    ├── Solvability        → checks inversion count + blank row parity BEFORE search
    │   [solvability.hpp / solvability.cpp]
    │
    └── ISolver            → common interface, pick BFS or A* at runtime
        [isolver.hpp]
            │
            ├── BFSSolver  → queue-based search, parent-pointer backtracking
            │   [bfs_solver.hpp / bfs_solver.cpp]
            │
            └── AStarSolver → priority-queue search, uses Heuristic for f = g + h
                [astar_solver.hpp / astar_solver.cpp]
                    │
                    └── ManhattanDistance : Heuristic
                        [heuristic.hpp / heuristic.cpp]
```

---

## Key Design Decisions

### 1. Board Packed into a Single `uint64_t`
Each of the 16 cells holds a value 0–15 (0 = blank), which fits in 4 bits. 16 × 4 = 64 bits exactly — the entire board fits in one machine-word integer.

- **Copying** a state = copying one integer (register operation)
- **Hashing** a state = `std::hash<uint64_t>` (one instruction)
- **Comparing** two states = `board_ == other.board_` (one instruction)

This matters because A* generates and hashes **millions of states** on deep scrambles — an `array<int,16>` would require element-wise work for every hash and comparison.

### 2. Solvability Check Before Search
Not every arrangement of tiles is solvable (~50% of random configurations are mathematically unreachable from goal). If you skip this check and run BFS/A* on an unsolvable board, the search runs forever. The check uses:

**Rule (4×4 board):** Let `inversions` = number of pairs (a, b) where a appears before b in the board sequence (blank excluded) but a > b. Let `blankRowFromBottom` = row of blank counted from bottom, 1-indexed.

> Solvable if and only if `(inversions is even) ≠ (blankRowFromBottom is even)`

Inversion count is computed in **O(n log n)** using merge sort — during the merge step, whenever a right-half element is taken before exhausting the left half, all remaining left-half elements form inversions with it, counted in one shot.

### 3. BFS as the Correctness Baseline
BFS guarantees the **shortest** solution by exploring states in strictly increasing move-count order. It was built first, and A*'s output is cross-validated against it: across **30 trials at depths 3–18, zero path-length mismatches**. This means we don't just trust A* is correct — we proved it against an independently correct reference.

### 4. A* with Manhattan Distance Heuristic
A* expands states by priority `f = g + h` where:
- `g` = exact moves taken so far (known)
- `h` = Manhattan distance estimate of remaining moves

**Manhattan distance:** for each tile, compute `|row_current - row_goal| + |col_current - col_goal|`, sum across all 15 tiles (blank excluded).

**Why it's admissible (never overestimates):** each move shifts exactly one tile by one cell, so a tile needs *at least* its Manhattan distance in moves to reach its goal position. The sum never exceeds the true remaining cost.

### 5. OOP Architecture
| Concept | Where Used |
|---|---|
| **Encapsulation** | `PuzzleState`: `board_` and `blank_pos_` are private; only constructible through a validating constructor that throws on invalid permutations |
| **Inheritance** | `BFSSolver : ISolver`, `AStarSolver : ISolver`, `ManhattanDistance : Heuristic` |
| **Virtual functions** | `ISolver::solve()`, `Heuristic::estimate()` are pure virtual — enables runtime polymorphism |
| **Operator overloading** | `operator==`/`operator!=` on `PuzzleState` for O(1) state comparison; `operator()` on `PuzzleStateHash` for use as `unordered_map` key |

### 6. Path Reconstruction via Backtracking
Neither BFS nor A* remembers *how* they reached the goal — they just know they did. Path reconstruction works via a **parent-pointer map**: `child_state → {parent_state, move_used}`. Once the goal is found, walk backward through the map from goal to start, collect moves, then `reverse()` them to get the forward solution sequence.

---

## Build Instructions

### Prerequisites
- C++17 compiler (GCC/MinGW/Clang)
- CMake ≥ 3.16 (optional, or use manual g++ commands below)

### Manual Compile (Windows PowerShell / Linux Terminal)

```powershell
# Build the CLI solver
g++ -std=c++17 -O2 -I include src/puzzle_state.cpp src/solvability.cpp src/heuristic.cpp src/bfs_solver.cpp src/astar_solver.cpp src/main.cpp -o puzzle15.exe

# Build the benchmark harness
g++ -std=c++17 -O2 -I include src/puzzle_state.cpp src/solvability.cpp src/heuristic.cpp src/bfs_solver.cpp src/astar_solver.cpp src/benchmark.cpp -o benchmark.exe
```

### Using CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## Usage

### Solve a Random Scramble
```powershell
.\puzzle15.exe scramble <depth> <bfs|astar>

# Examples
.\puzzle15.exe scramble 23 astar
.\puzzle15.exe scramble 15 bfs
```

### Solve a Custom Board
Provide 16 space-separated values (row-major order, `0` = blank):
```powershell
.\puzzle15.exe custom "<16 values>" <bfs|astar>

# Example: blank in second-to-last cell
.\puzzle15.exe custom "1 2 3 4 5 6 7 8 9 10 11 12 13 14 0 15" astar
```

### Unsolvable Board Detection
```powershell
# Classic unsolvable configuration (swap 14 and 15)
.\puzzle15.exe custom "1 2 3 4 5 6 7 8 9 10 11 12 13 15 14 0" astar
# Output: "This configuration is UNSOLVABLE"
```

### Run Benchmarks
```powershell
.\benchmark.exe
```

---

## Benchmark Results

All results measured on Windows (MinGW GCC), `-O2` optimization, **20 trials per depth**.

### BFS — Exponential Blowup (Why A* is Needed)

| Depth | Avg Time | Max Time | Avg States Expanded |
|-------|----------|----------|---------------------|
| 5 | ~instant | ~instant | 41 |
| 10 | 0.005s | 0.019s | 1,845 |
| 15 | 0.144s | 0.299s | 58,727 |
| 18 | 2.149s | 4.899s | 358,969 |
| 20 | 10.66s | 27.41s | 930,293 |

### A\* + Manhattan Distance — Sub-second at All Depths

| Depth | Avg Time | Max Time | Avg States Expanded |
|-------|----------|----------|---------------------|
| 5 | ~instant | ~instant | 6 |
| 10 | 0.000154s | 0.002s | 15 |
| 20 | 0.000372s | 0.003s | 203 |
| 23 | 0.000373s | 0.003s | 463 |
| 33 | 0.026s | 0.322s | 7,183 |
| 40 | 0.113s | 1.726s | 16,351 |

### Key Comparison at Depth 20

| Metric | BFS | A* | Reduction |
|--------|-----|----|-----------|
| Avg states expanded | 930,293 | 203 | **99.97%** |
| Avg solve time | 10.66s | 0.00037s | **~28,800x faster** |

---

## Running Tests

```powershell
# From Tests/ folder
g++ -std=c++17 -O2 -I ../include test_puzzle_State.cpp ../src/puzzle_state.cpp -o test_ps.exe && .\test_ps.exe
g++ -std=c++17 -O2 -I ../include test_solvability.cpp ../src/puzzle_state.cpp ../src/solvability.cpp -o test_solv.exe && .\test_solv.exe
g++ -std=c++17 -O2 -I ../include test_bfs_solver.cpp ../src/puzzle_state.cpp ../src/solvability.cpp ../src/bfs_solver.cpp -o test_bfs.exe && .\test_bfs.exe
g++ -std=c++17 -O2 -I ../include test_heuristic.cpp ../src/puzzle_state.cpp ../src/heuristic.cpp -o test_h.exe && .\test_h.exe
g++ -std=c++17 -O2 -I ../include test_astar_solver.cpp ../src/puzzle_state.cpp ../src/solvability.cpp ../src/heuristic.cpp ../src/bfs_solver.cpp ../src/astar_solver.cpp -o test_astar.exe && .\test_astar.exe
```
<div align="center">

**Made with ❤️ by Ashutosh**


