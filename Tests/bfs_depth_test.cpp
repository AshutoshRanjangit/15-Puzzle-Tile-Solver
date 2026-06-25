#include "../include/puzzle_state.hpp"
#include "../include/bfs_solver.hpp"
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>

// Generates a scramble by doing N random LEGAL moves from goal, avoiding
// immediately undoing the previous move (cuts branching factor from 4 to
// at most 3 after the first move -- a free pruning win). This guarantees
// solvability by construction since we only ever apply legal moves.
PuzzleState scrambleByMoves(int n, unsigned seed) {
    std::mt19937 rng(seed);
    PuzzleState cur;
    PuzzleState::Move lastMove = PuzzleState::Move::UP; // sentinel, unused until hasLast=true
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
    BFSSolver solver;
    for (int depth : {5, 10, 15, 18, 20}) {
        PuzzleState scrambled = scrambleByMoves(depth, 42 + depth);
        auto t0 = std::chrono::steady_clock::now();
        Solution sol = solver.solve(scrambled);
        auto t1 = std::chrono::steady_clock::now();
        double secs = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "Scramble depth " << depth
                  << " -> BFS solution length " << sol.pathLength()
                  << ", expanded " << sol.statesExpanded
                  << " states, time " << secs << "s\n";
    }
    return 0;
}