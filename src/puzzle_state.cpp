#include "../include/puzzle_state.hpp"
#include <stdexcept>
#include <bitset>
#include <sstream>

// --- Internal bit-packing helpers -------------------------------------------

int PuzzleState::getCell(uint64_t board, int cell_index) {
    // Shift right so the desired 4-bit nibble lands in the lowest 4 bits,
    // then mask off everything else with & 0xF (binary 1111).
    return static_cast<int>((board >> (4 * cell_index)) & 0xFULL);
}

uint64_t PuzzleState::setCell(uint64_t board, int cell_index, int value) {
    const uint64_t shift = 4 * cell_index;
    const uint64_t clearMask = ~(0xFULL << shift);     // zero out the target nibble
    board &= clearMask;                                  // clear it
    board |= (static_cast<uint64_t>(value) << shift);    // write new value into it
    return board;
}

// --- Constructors ------------------------------------------------------------

PuzzleState::PuzzleState() {
    // Goal layout: cell i holds (i+1) for i = 0..14, and cell 15 holds 0 (blank).
    // i.e. 1 2 3 4 / 5 6 7 8 / 9 10 11 12 / 13 14 15 [blank]
    board_ = 0;
    for (int i = 0; i < NUM_CELLS - 1; ++i) {
        board_ = setCell(board_, i, i + 1);
    }
    board_ = setCell(board_, NUM_CELLS - 1, 0);
    blank_pos_ = NUM_CELLS - 1;
}

PuzzleState::PuzzleState(const std::array<int, NUM_CELLS>& cells) {
    // Validate: must be a permutation of 0..15 (each value appears exactly once).
    std::bitset<NUM_CELLS> seen;
    int blank = -1;
    for (int i = 0; i < NUM_CELLS; ++i) {
        int v = cells[i];
        if (v < 0 || v >= NUM_CELLS || seen[v]) {
            throw std::invalid_argument(
                "PuzzleState: input is not a valid permutation of 0..15");
        }
        seen[v] = true;
        if (v == 0) blank = i;
    }
    // unreachable in practice (bitset check guarantees a 0 exists), but
    // defensive in case NUM_CELLS ever changes
    if (blank == -1) {
        throw std::invalid_argument("PuzzleState: no blank (0) found in input");
    }

    board_ = 0;
    for (int i = 0; i < NUM_CELLS; ++i) {
        board_ = setCell(board_, i, cells[i]);
    }
    blank_pos_ = blank;
}

// --- Queries -------------------------------------------------------------

bool PuzzleState::isGoal() const {
    return *this == goal();
}

int PuzzleState::at(int cell_index) const {
    return getCell(board_, cell_index);
}

std::vector<PuzzleState::Move> PuzzleState::legalMoves() const {
    std::vector<Move> moves;
    moves.reserve(4);

    const int row = rowOf(blank_pos_);
    const int col = colOf(blank_pos_);

    if (row > 0)        moves.push_back(Move::UP);
    if (row < SIDE - 1) moves.push_back(Move::DOWN);
    if (col > 0)         moves.push_back(Move::LEFT);
    if (col < SIDE - 1)  moves.push_back(Move::RIGHT);

    return moves;
}

PuzzleState PuzzleState::applyMove(Move m) const {
    int target = -1;
    switch (m) {
        case Move::UP:    target = blank_pos_ - SIDE; break;
        case Move::DOWN:  target = blank_pos_ + SIDE; break;
        case Move::LEFT:  target = blank_pos_ - 1;    break;
        case Move::RIGHT: target = blank_pos_ + 1;    break;
    }

    // Swap blank (0) with whatever value is at `target`.
    int movedValue = getCell(board_, target);
    uint64_t newBoard = board_;
    newBoard = setCell(newBoard, blank_pos_, movedValue);
    newBoard = setCell(newBoard, target, 0);

    return PuzzleState(newBoard, target); // target is the new blank position
}

std::string PuzzleState::toString() const {
    std::ostringstream oss;
    for (int r = 0; r < SIDE; ++r) {
        for (int c = 0; c < SIDE; ++c) {
            int v = at(r * SIDE + c);
            if (v == 0) oss << "  . ";
            else        oss << (v < 10 ? "  " : " ") << v << " ";
        }
        oss << "\n";
    }
    return oss.str();
}

const PuzzleState& PuzzleState::goal() {
    static const PuzzleState g; // default ctor builds the goal state
    return g;
}