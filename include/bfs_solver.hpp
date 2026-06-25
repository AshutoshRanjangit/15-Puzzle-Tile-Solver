#pragma once

#include "isolver.hpp"
#include "puzzle_state.hpp"

// ============================================================================
// BFSSolver
// ----------------------------------------------------------------------------
// Solves the puzzle using plain Breadth-First Search over the state graph.
//
// GUARANTEES the shortest solution (fewest moves), because BFS explores
// states in strictly increasing order of distance-from-start. This makes it
// our CORRECTNESS BASELINE: A* must always agree with BFS on path length,
// even though A* will (and should) expand far fewer states to get there.
//
// PRACTICAL LIMIT: works great for shallow scrambles (roughly up to ~20
// moves deep on a 4x4 board), but the state space grows so fast
// (branching factor ~2.13 average after excluding the reverse of the last
// move) that by ~25-30 moves deep, BFS would need to hold tens of millions
// of states in memory simultaneously before reaching the goal layer. This
// is EXACTLY the problem A* (next file) solves by being selective about
// which states to expand, instead of expanding all of them layer by layer.
// ============================================================================

class BFSSolver : public ISolver {
public:
    Solution solve(const PuzzleState& start) override;
    std::string name() const override { return "BFS"; }
};