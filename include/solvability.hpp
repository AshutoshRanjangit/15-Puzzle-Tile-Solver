#pragma once

#include "puzzle_state.hpp"
#include <vector>
#include <cstdint>

// ============================================================================
// Solvability
// ----------------------------------------------------------------------------
// Determines whether a given PuzzleState can be solved at all -- i.e. whether
// there EXISTS any sequence of legal slide moves that reaches the goal state.
//
// WHY THIS IS NEEDED: a random arrangement of 16 tiles is only solvable
// about half the time. If you skip this check and just throw BFS/A* at an
// unsolvable state, the search will run forever (or until you run out of
// memory) because it's looking for a goal that is mathematically unreachable.
//
// THE MATH (4x4 / even-width board specifically -- the rule is DIFFERENT
// for odd-width boards like the classic 8-puzzle, so don't reuse this logic
// blindly if you ever extend to a 3x3 or 5x5 puzzle):
//
//   Let inversions = number of pairs (a, b) such that a appears before b
//   when reading the board left-to-right, top-to-bottom (ignoring the
//   blank), but a > b in value.
//
//   Let blankRowFromBottom = row of the blank, counted from the BOTTOM,
//   1-indexed (bottom row = 1, the row above it = 2, etc).
//
//   The state is SOLVABLE if and only if:
//     - blankRowFromBottom is ODD  AND inversions is EVEN,   OR
//     - blankRowFromBottom is EVEN AND inversions is ODD
//
//   Equivalently: (inversions is even) != (blankRowFromBottom is even)
//   i.e. exactly one of the two is even -- they must have OPPOSITE parity.
//
// This is implemented as a static/free-function utility class (no instance
// state needed) rather than a method on PuzzleState itself, to keep
// PuzzleState focused purely on representing+transforming a state, not on
// this separate piece of combinatorial theory. Single Responsibility.
// ============================================================================

class Solvability {
public:
    // Returns true if `state` can be transformed into PuzzleState::goal()
    // via some sequence of legal moves.
    static bool isSolvable(const PuzzleState& state);

    // Exposed separately (not just as a private detail) because it's useful
    // on its own for testing/debugging, and because interview questions about
    // this project will likely ask specifically about inversion counting.
    static int countInversions(const PuzzleState& state);

private:
    // Merge-sort based inversion counter over the tile sequence (blank
    // excluded). Returns the count; sorts `arr` in place as a side effect
    // (which is fine, it's operating on a local copy passed in by value
    // from countInversions, not the PuzzleState's internal data).
    static long long mergeSortCount(std::vector<int>& arr, int left, int right);
    static long long mergeAndCount(std::vector<int>& arr, int left, int mid, int right);
};