#pragma once

#include "isolver.hpp"
#include "heuristic.hpp"
#include <memory>

// ============================================================================
// AStarSolver
// ----------------------------------------------------------------------------
// Solves the puzzle using A* search: like BFS, but instead of exploring
// states purely by depth (g), it prioritizes states by f = g + h, where h
// is a heuristic ESTIMATE of how many moves remain to the goal. This lets
// it explore a much narrower "cone" toward the goal instead of BFS's
// uniformly expanding sphere, which is what lets it handle scrambles 30+
// moves deep that would make BFS run out of time/memory.
//
// CORRECTNESS GUARANTEE: as long as the supplied heuristic is ADMISSIBLE
// (never overestimates true remaining distance -- see heuristic.hpp), A*
// is guaranteed to find a SHORTEST path, same as BFS. We rely on this to
// cross-check A*'s output against BFSSolver in tests: their path LENGTHS
// must always match, even though A* expands far fewer states to get there.
//
// WHY IT TAKES A Heuristic* (Strategy pattern, see heuristic.hpp): this
// lets us benchmark "A* + Manhattan" vs "A* + Manhattan+LinearConflict"
// later just by constructing two AStarSolver instances with different
// heuristics, with zero changes to this class.
// ============================================================================

class AStarSolver : public ISolver {
public:
    // Takes ownership of the heuristic via shared_ptr so the same Heuristic
    // instance could in principle be reused across multiple solver instances
    // (e.g. if we ever wanted two solvers sharing one heuristic object).
    explicit AStarSolver(std::shared_ptr<Heuristic> heuristic);

    Solution solve(const PuzzleState& start) override;
    std::string name() const override;

private:
    std::shared_ptr<Heuristic> heuristic_;
};