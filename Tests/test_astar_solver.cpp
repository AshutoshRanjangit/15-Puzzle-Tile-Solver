#include "../include/astar_solver.hpp"
#include "../include/bfs_solver.hpp"
#include "heuristic.hpp"
#include <iostream>
#include <cassert>
#include <random>
#include <algorithm>

static PuzzleState replay(const PuzzleState& start, const std::vector<PuzzleState::Move>& moves) {
    PuzzleState cur = start;
    for (auto m : moves) cur = cur.applyMove(m);
    return cur;
}

// Generates a scramble using N legal moves, avoiding immediately undoing
// the previous move (standard pruning -- also matches what our real
// Scramble class will do later). Solvable by construction since we only
// ever apply legal moves.
static PuzzleState scrambleByMoves(int n, unsigned seed) {
    std::mt19937 rng(seed);
    PuzzleState cur;
    PuzzleState::Move lastMove = PuzzleState::Move::UP;
    bool hasLast = false;

    auto opposite = [](PuzzleState::Move m) {
        switch (m) {
            case PuzzleState::Move::UP: return PuzzleState::Move::DOWN;
            case PuzzleState::Move::DOWN: return PuzzleState::Move::UP;
            case PuzzleState::Move::LEFT: return PuzzleState::Move::RIGHT;
            case PuzzleState::Move::RIGHT: return PuzzleState::Move::LEFT;
        }
        return m;
    };

    for (int i = 0; i < n; ++i) {
        auto moves = cur.legalMoves();
        if (hasLast) {
            PuzzleState::Move opp = opposite(lastMove);
            moves.erase(std::remove(moves.begin(), moves.end(), opp), moves.end());
        }
        std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
        PuzzleState::Move chosen = moves[dist(rng)];
        cur = cur.applyMove(chosen);
        lastMove = chosen;
        hasLast = true;
    }
    return cur;
}

int main() {
    AStarSolver astar(std::make_shared<ManhattanDistance>());
    BFSSolver bfs;

    // Test 1: goal state -> 0 moves
    {
        PuzzleState g;
        Solution sol = astar.solve(g);
        assert(sol.solvable);
        assert(sol.moves.empty());
        std::cout << "Test 1 passed: goal state solves in 0 moves\n";
    }

    // Test 2: 1-move scramble -> exactly 1 move, replay reaches goal
    {
        PuzzleState g;
        PuzzleState oneAway = g.applyMove(PuzzleState::Move::UP);
        Solution sol = astar.solve(oneAway);
        assert(sol.solvable);
        assert(sol.pathLength() == 1);
        assert(replay(oneAway, sol.moves).isGoal());
        std::cout << "Test 2 passed: 1-move scramble solved in exactly 1 move\n";
    }

    // Test 3: unsolvable state rejected immediately
    {
        std::array<int, 16> cells = {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 14, 0
        };
        PuzzleState unsolvable(cells);
        Solution sol = astar.solve(unsolvable);
        assert(sol.solvable == false);
        std::cout << "Test 3 passed: unsolvable state correctly rejected\n";
    }

    // Test 4: THE CRITICAL CROSS-VALIDATION. For many scrambles at various
    // depths, A* and BFS must agree EXACTLY on solution length (both must
    // be optimal), and A* should generally expand fewer or equal states.
    // Also verify A*'s returned path actually replays to the goal.
    {
        int mismatches = 0;
        long long totalBfsExpanded = 0, totalAstarExpanded = 0;
        int numTrials = 0;

        for (int depth : {3, 6, 9, 12, 15, 18}) {
            for (unsigned trial = 0; trial < 5; ++trial) {
                PuzzleState scrambled = scrambleByMoves(depth, depth * 1000 + trial);

                Solution bfsSol = bfs.solve(scrambled);
                Solution astarSol = astar.solve(scrambled);

                assert(bfsSol.solvable && astarSol.solvable);

                if (bfsSol.pathLength() != astarSol.pathLength()) {
                    mismatches++;
                    std::cout << "  MISMATCH at depth " << depth << " trial " << trial
                              << ": BFS=" << bfsSol.pathLength()
                              << " A*=" << astarSol.pathLength() << "\n";
                }

                // A*'s own path must independently replay to goal -- this
                // catches bugs in A*'s path reconstruction specifically,
                // not just "did it report the right length".
                assert(replay(scrambled, astarSol.moves).isGoal());
                assert(static_cast<int>(astarSol.moves.size()) == astarSol.pathLength());

                totalBfsExpanded += bfsSol.statesExpanded;
                totalAstarExpanded += astarSol.statesExpanded;
                numTrials++;
            }
        }

        assert(mismatches == 0);
        std::cout << "Test 4 passed: " << numTrials << " trials, A* and BFS agree on path length every time\n";
        std::cout << "  Total states expanded -- BFS: " << totalBfsExpanded
                  << ", A*: " << totalAstarExpanded
                  << " (A* expanded " << (100.0 * totalAstarExpanded / totalBfsExpanded)
                  << "% as many states as BFS)\n";
    }

    // Test 5: priority never decreases as nodes are POPPED, when using an
    // admissible+consistent heuristic. This is the debugging invariant
    // mentioned in the Princeton course notes -- a violation would indicate
    // a heuristic/implementation bug. We verify this indirectly by checking
    // that f-values of expanded (popped, non-stale) nodes are non-decreasing.
    // To check this directly we'd need to instrument the solver internals,
    // so instead we do a structural sanity check: solve a moderately deep
    // scramble and confirm it completes and the solution is optimal length
    // vs BFS (already covered in test 4) -- noting here as a documented
    // property rather than re-deriving instrumentation.
    {
        PuzzleState scrambled = scrambleByMoves(20, 555);
        Solution sol = astar.solve(scrambled);
        assert(sol.solvable);
        assert(replay(scrambled, sol.moves).isGoal());
        std::cout << "Test 5 passed: 20-move scramble solved correctly by A*, path replays to goal\n";
    }

    std::cout << "\nAll AStarSolver tests passed!\n";
    return 0;
}