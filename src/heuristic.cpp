#include "../include/heuristic.hpp"
#include <cstdlib> // for std::abs

int ManhattanDistance::estimate(const PuzzleState& state) const {
    int total = 0;

    for (int cellIndex = 0; cellIndex < PuzzleState::NUM_CELLS; ++cellIndex) {
        int tileValue = state.at(cellIndex);
        if (tileValue == 0) continue; // blank excluded -- no fixed "home" to measure against

        int curRow = PuzzleState::rowOf(cellIndex);
        int curCol = PuzzleState::colOf(cellIndex);
        int goalR = goalRow(tileValue);
        int goalC = goalCol(tileValue);

        total += std::abs(curRow - goalR) + std::abs(curCol - goalC);
    }

    return total;
}