#pragma once

#include "puzzle_state.hpp"
#include <vector>
#include <cstdint>

// ============================================================================
// ISolver
// ----------------------------------------------------------------------------
// Common interface implemented by BFSSolver and AStarSolver.
//
// WHY THIS EXISTS (the OOP angle): without this, the benchmark harness and
// main.cpp would need a different code path for each solver type. With it,
// they can hold an ISolver* (or reference) and call solve() uniformly --
// classic polymorphism. This is also what makes it easy to add a third
// solver later (e.g. IDA*) without touching any calling code.
//
// Solution struct carries everything we'd want to report: the move
// sequence itself, plus stats that matter for the benchmark writeup
// (how many states did we actually expand? that's the number that
// explains WHY A* is faster than BFS on deep scrambles, not just THAT
// it's faster).
// ============================================================================

struct Solution {
    bool solvable = false;                     // was the input even solvable?
    std::vector<PuzzleState::Move> moves;        // the move sequence, start->goal
    long long statesExpanded = 0;                // nodes popped & processed
    long long statesGenerated = 0;               // nodes pushed onto frontier
    double timeSeconds = 0.0;                    // wall-clock solve time

    int pathLength() const { return static_cast<int>(moves.size()); }
};

class ISolver {
public:
    virtual ~ISolver() = default;

    // Attempt to solve `start`. Implementations should internally check
    // Solvability::isSolvable() first and return Solution{solvable=false}
    // immediately if not -- never let an unsolvable state hang the search.
    virtual Solution solve(const PuzzleState& start) = 0;

    // Human-readable name, used in benchmark output (e.g. "BFS", "A* (Manhattan)")
    virtual std::string name() const = 0;
};