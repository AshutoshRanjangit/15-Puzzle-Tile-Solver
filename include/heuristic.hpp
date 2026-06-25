#pragma once

#include "puzzle_state.hpp"

// ============================================================================
// Heuristic (Strategy Pattern)
// ----------------------------------------------------------------------------
// Abstract interface for "estimate how many moves remain to reach the goal."
// AStarSolver depends ONLY on this interface, not on any concrete heuristic.
// This is deliberate: it means we can swap ManhattanDistance for a stronger
// heuristic (e.g. Manhattan + Linear Conflict) later WITHOUT touching
// AStarSolver's code at all -- just construct it with a different Heuristic*.
// This is exactly the kind of thing worth mentioning in an interview: "I used
// the Strategy pattern so I could A/B benchmark different heuristics."
//
// CONTRACT (very important -- violating this breaks A*'s optimality guarantee):
//   estimate(state) must be ADMISSIBLE: it must NEVER return a value greater
//   than the true minimum number of moves from `state` to the goal. If a
//   heuristic ever overestimates, A* can return a suboptimal (too-long)
//   solution, because it might deprioritize a state that actually IS on the
//   shortest path.
// ============================================================================

class Heuristic {
public:
    virtual ~Heuristic() = default;

    // Estimated number of moves from `state` to PuzzleState::goal().
    // Must be admissible (see contract above).
    virtual int estimate(const PuzzleState& state) const = 0;

    virtual std::string name() const = 0;
};

// ============================================================================
// ManhattanDistance
// ----------------------------------------------------------------------------
// For each tile 1..15 (blank excluded), compute the grid distance between
// its CURRENT position and its GOAL position:
//     |row_current - row_goal| + |col_current - col_goal|
// and sum this over all 15 tiles.
//
// WHY THIS IS ADMISSIBLE: each legal move slides exactly one tile by exactly
// one grid cell. So a single tile needs AT LEAST its Manhattan distance many
// moves to reach its goal position -- you cannot do better even with a
// perfectly lucky sequence, because every move only changes one tile's
// position by 1 cell. Summing this lower bound across all tiles still gives
// a value that never exceeds the true number of moves needed (each actual
// move can reduce the SUM by at most 1, since only one tile moves per turn).
//
// PERFORMANCE NOTE: this is computed from scratch every time it's called.
// A common optimization (not done yet, flagged for later if benchmarks need
// it) is INCREMENTAL Manhattan distance: when generating a child state from
// a parent via one move, only ONE tile changed position, so you can compute
// child_h = parent_h - (old contribution of that tile) + (new contribution)
// in O(1) instead of recomputing the full O(15) sum. Worth adding if profiling
// shows heuristic computation is a bottleneck.
// ============================================================================

class ManhattanDistance : public Heuristic {
public:
    int estimate(const PuzzleState& state) const override;
    std::string name() const override { return "Manhattan Distance"; }

private:
    // Precomputed goal row/col for each tile value (1-15), so we don't
    // recompute "where does tile 7 belong" by scanning the goal state
    // every single call.
    static int goalRow(int tileValue) { return (tileValue - 1) / PuzzleState::SIDE; }
    static int goalCol(int tileValue) { return (tileValue - 1) % PuzzleState::SIDE; }
};