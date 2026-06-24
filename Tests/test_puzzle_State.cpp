#include "../include/puzzle_state.hpp"
#include <iostream>
#include <cassert>

int main() {
    // Test 1: default ctor is the goal state
    PuzzleState g;
    assert(g.isGoal());
    std::cout << "Test 1 passed: default ctor is goal state\n";

    // Test 2: blank position of goal is cell 15
    assert(g.blankPos() == 15);
    std::cout << "Test 2 passed: goal blank at index 15\n";

    // Test 3: legal moves from goal (blank at bottom-right) should be UP, LEFT only
    auto moves = g.legalMoves();
    assert(moves.size() == 2);
    std::cout << "Test 3 passed: goal state has exactly 2 legal moves\n";

    // Test 4: apply UP then DOWN should return to goal
    PuzzleState afterUp = g.applyMove(PuzzleState::Move::UP);
    assert(!afterUp.isGoal());
    assert(afterUp.blankPos() == 11);
    PuzzleState backToGoal = afterUp.applyMove(PuzzleState::Move::DOWN);
    assert(backToGoal == g);
    assert(backToGoal.isGoal());
    std::cout << "Test 4 passed: UP then DOWN returns to original state\n";

    // Test 5: construct an explicit board, check at()
    std::array<int, 16> cells = {1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,0,15};
    PuzzleState custom(cells);
    assert(custom.blankPos() == 14);
    assert(custom.at(15) == 15);
    assert(!custom.isGoal());
    std::cout << "Test 5 passed: custom state constructed correctly\n";

    // Test 6: invalid input throws
    bool threw = false;
    try {
        std::array<int, 16> bad = {1,1,2,3, 4,5,6,7, 8,9,10,11, 12,13,14,15}; // dup '1', missing one val
        PuzzleState invalid(bad);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "Test 6 passed: invalid permutation throws\n";

    // Test 7: print the goal state visually
    std::cout << "\nGoal state visual:\n" << g.toString() << "\n";

    // Test 8: hash works (for use in unordered_map later)
    PuzzleStateHash hasher;
    assert(hasher(g) == hasher(g));
    PuzzleState g2; // another goal state
    assert(hasher(g) == hasher(g2)); // same state -> same hash
    std::cout << "Test 8 passed: hashing is consistent\n";

    std::cout << "\nAll PuzzleState tests passed!\n";
    return 0;
}