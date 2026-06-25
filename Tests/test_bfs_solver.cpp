#include "../include/bfs_solver.hpp"
#include "../include/solvability.hpp"
#include <iostream>
#include <cassert>

// Helper: replay a move sequence starting from `start`, return the
// resulting state. Used to verify a Solution's moves ACTUALLY reach goal --
// this is the test that would catch a buggy path-reconstruction even if
// the reported path LENGTH happened to look plausible.
static PuzzleState replay(const PuzzleState& start, const std::vector<PuzzleState::Move>& moves) {
    PuzzleState cur = start;
    for (auto m : moves) {
        cur = cur.applyMove(m);
    }
    return cur;
}

int main() {
    BFSSolver solver;

    // Test 1: already-solved state -> 0 moves, solvable=true
    {
        PuzzleState g;
        Solution sol = solver.solve(g);
        assert(sol.solvable);
        assert(sol.moves.empty());
        std::cout << "Test 1 passed: goal state solves in 0 moves\n";
    }

    // Test 2: one move away from goal -> solution length must be exactly 1
    {
        PuzzleState g;
        PuzzleState oneAway = g.applyMove(PuzzleState::Move::UP);
        Solution sol = solver.solve(oneAway);
        assert(sol.solvable);
        assert(sol.pathLength() == 1);
        PuzzleState result = replay(oneAway, sol.moves);
        assert(result.isGoal());
        std::cout << "Test 2 passed: 1-move scramble solved in exactly 1 move, replay reaches goal\n";
    }

    // Test 3: a few moves away -- scramble by applying a known sequence,
    // then confirm BFS finds a path of length <= that sequence's length
    // (could be shorter if there's a shortcut, but must reach goal).
    {
        PuzzleState g;
        // Apply a small sequence of moves to scramble it
        PuzzleState scrambled = g;
        std::vector<PuzzleState::Move> scrambleMoves = {
            PuzzleState::Move::UP, PuzzleState::Move::LEFT,
            PuzzleState::Move::DOWN, PuzzleState::Move::RIGHT,
            PuzzleState::Move::UP, PuzzleState::Move::LEFT
        };
        for (auto m : scrambleMoves) scrambled = scrambled.applyMove(m);

        Solution sol = solver.solve(scrambled);
        assert(sol.solvable);
        assert(sol.pathLength() <= static_cast<int>(scrambleMoves.size()));
        PuzzleState result = replay(scrambled, sol.moves);
        assert(result.isGoal());
        std::cout << "Test 3 passed: 6-move scramble solved (BFS found length "
                  << sol.pathLength() << "), replay confirms it reaches goal\n";
    }

    // Test 4: BFS must find the SHORTEST path -- verify optimality directly.
    // Take a state exactly 2 moves from goal via a path that does NOT undo
    // itself (UP then LEFT, two genuinely different moves), and confirm
    // BFS doesn't return something longer like 4 moves.
    {
        PuzzleState g;
        PuzzleState twoAway = g.applyMove(PuzzleState::Move::UP).applyMove(PuzzleState::Move::LEFT);
        Solution sol = solver.solve(twoAway);
        assert(sol.solvable);
        assert(sol.pathLength() == 2);
        std::cout << "Test 4 passed: BFS finds shortest path (2 moves), not just *a* path\n";
    }

    // Test 5: unsolvable state must be rejected immediately, not searched.
    {
        std::array<int, 16> cells = {
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 15, 14, 0   // classic unsolvable swap
        };
        PuzzleState unsolvable(cells);
        Solution sol = solver.solve(unsolvable);
        assert(sol.solvable == false);
        assert(sol.moves.empty());
        std::cout << "Test 5 passed: unsolvable state correctly rejected (no search performed)\n";
    }

    // Test 6: stats sanity -- statesExpanded and statesGenerated should be
    // positive and statesGenerated >= statesExpanded for any nontrivial search.
    {
        PuzzleState g;
        PuzzleState scrambled = g.applyMove(PuzzleState::Move::UP)
                                  .applyMove(PuzzleState::Move::LEFT)
                                  .applyMove(PuzzleState::Move::DOWN);
        Solution sol = solver.solve(scrambled);
        assert(sol.statesExpanded > 0);
        assert(sol.statesGenerated >= sol.statesExpanded);
        std::cout << "Test 6 passed: stats look sane (expanded=" << sol.statesExpanded
                  << ", generated=" << sol.statesGenerated << ")\n";
    }

    std::cout << "\nAll BFSSolver tests passed!\n";
    return 0;
}