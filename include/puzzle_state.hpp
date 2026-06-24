#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <vector>

// ============================================================================
// PuzzleState
// ----------------------------------------------------------------------------
// Represents a single configuration of the 4x4 (15-tile) sliding puzzle.
//
// WHY a packed uint64_t instead of array<int,16> or vector<int>?
//   - The state gets copied / hashed / compared MILLIONS of times during
//     A* search on deep scrambles. A uint64_t is a primitive: copying it,
//     hashing it, comparing it with == are all O(1) machine instructions.
//     An array/vector needs element-wise work for hashing and comparison.
//   - 16 cells * 4 bits/cell = 64 bits, and each cell only needs to store
//     0-15, which fits exactly in 4 bits. No wasted space, no rounding.
//
// Cell layout: cell i (i = 0..15, row-major, 0 = top-left, 15 = bottom-right)
// is stored in bits [4*i, 4*i + 4) of the uint64_t.
//   value 0 in a cell means "blank".
// ============================================================================

class PuzzleState {
public:
    static constexpr int SIDE = 4;
    static constexpr int NUM_CELLS = SIDE * SIDE; // 16

    enum class Move { UP, DOWN, LEFT, RIGHT };

    // Construct the goal state: 1,2,3,...,15,0 (0 = blank in bottom-right)
    PuzzleState();

    // Construct from an explicit list of 16 values (row-major), 0 = blank.
    // Validates that it's a permutation of 0..15 (throws std::invalid_argument
    // if not) -- this is the encapsulation guarantee: you cannot build a
    // PuzzleState that isn't a legal arrangement of tiles.
    explicit PuzzleState(const std::array<int, NUM_CELLS>& cells);

    // --- Core queries ---
    bool isGoal() const;
    int blankPos() const { return blank_pos_; }
    uint64_t packed() const { return board_; }

    // Value at a given cell index (0-15, row-major). 0 = blank.
    int at(int cell_index) const;

    // Which moves are legal from this state right now (depends on where
    // the blank is -- e.g. blank in top row can't move UP).
    std::vector<Move> legalMoves() const;

    // Returns the new state after applying `m`. Caller must ensure `m` is
    // legal (use legalMoves() to check, or applyMoveSafe() below).
    PuzzleState applyMove(Move m) const;

    // Convenience: row/col of a given cell index, and of the blank.
    static int rowOf(int cell_index) { return cell_index / SIDE; }
    static int colOf(int cell_index) { return cell_index % SIDE; }

    // For debugging / CLI output.
    std::string toString() const;

    // --- Equality / hashing (needed for unordered_map/unordered_set) ---
    bool operator==(const PuzzleState& other) const { return board_ == other.board_; }
    bool operator!=(const PuzzleState& other) const { return board_ != other.board_; }

    // Goal state singleton, used everywhere we need "what are we aiming for"
    static const PuzzleState& goal();

private:
    uint64_t board_;   // packed board, 4 bits per cell
    int blank_pos_;     // cached index of blank (0-15), avoids re-scanning

    static int getCell(uint64_t board, int cell_index);
    static uint64_t setCell(uint64_t board, int cell_index, int value);

    // Private ctor used internally when we already know board_ and blank_pos_
    // (e.g. inside applyMove) and don't need to re-validate/re-scan.
    PuzzleState(uint64_t board, int blank_pos) : board_(board), blank_pos_(blank_pos) {}
};

// Hash functor so PuzzleState can be used as a key in unordered_map/unordered_set.
// Since board_ is already a well-distributed 64-bit value (it's a permutation
// encoding), we can use it almost directly -- std::hash<uint64_t> is sufficient,
// no need for a custom mixing function.
struct PuzzleStateHash {
    std::size_t operator()(const PuzzleState& s) const {
        return std::hash<uint64_t>()(s.packed());
    }
};