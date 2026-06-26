// ============================================================================
// benchmark.cpp
// ----------------------------------------------------------------------------
// Produces the timing numbers behind claims like "solves N-move scrambles
// within X seconds." No fixed target is assumed here -- run this, look at
// the real numbers, and THEN decide what's honest to write on the resume.
//
// METHODOLOGY (simplified -- no separate Scramble class):
//   Generate a scramble by applying N random LEGAL moves to the goal state,
//   using the "no immediate undo" pruning (skip whichever move would just
//   reverse the previous one).
//
//   EMPIRICAL FINDING: with this pruning active, a random walk of length N
//   lands at a TRUE OPTIMAL solve distance of exactly N roughly 90%+ of the
//   time. The rare misses undershoot by an EVEN number (N-2, N-4, ...),
//   never odd -- ties back to the permutation-parity argument in Solvability.
//
// All timing numbers should be measured with an optimized (Release/-O2)
// build -- see CMakeLists.txt for why this matters.
// ============================================================================

#include "puzzle_state.hpp"
#include "bfs_solver.hpp"
#include "astar_solver.hpp"
#include "heuristic.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>

PuzzleState::Move oppositeMove(PuzzleState::Move m) {
    switch (m) {
        case PuzzleState::Move::UP:    return PuzzleState::Move::DOWN;
        case PuzzleState::Move::DOWN:  return PuzzleState::Move::UP;
        case PuzzleState::Move::LEFT:  return PuzzleState::Move::RIGHT;
        case PuzzleState::Move::RIGHT: return PuzzleState::Move::LEFT;
    }
    return m;
}

PuzzleState scrambleByMoves(int numMoves, unsigned seed) {
    std::mt19937 rng(seed);
    PuzzleState cur;
    PuzzleState::Move lastMove = PuzzleState::Move::UP;
    bool hasLast = false;

    for (int i = 0; i < numMoves; ++i) {
        auto moves = cur.legalMoves();
        if (hasLast) {
            PuzzleState::Move opp = oppositeMove(lastMove);
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

struct DepthStats {
    int depth;
    double minTime = 1e18, maxTime = 0.0, avgTime = 0.0;
    long long avgExpanded = 0, maxExpanded = 0;
    int trialsRun = 0, trialsSolved = 0;
};

DepthStats runBenchmarkAtDepth(ISolver& solver, int depth, int numTrials, unsigned seedBase) {
    DepthStats stats;
    stats.depth = depth;

    std::vector<double> times;
    std::vector<long long> expandedCounts;

    for (int trial = 0; trial < numTrials; ++trial) {
        PuzzleState scrambled = scrambleByMoves(depth, seedBase + trial);
        stats.trialsRun++;

        Solution sol = solver.solve(scrambled);

        if (!sol.solvable) {
            std::cerr << "WARNING: scramble produced an unsolvable state at depth "
                      << depth << " -- this should be impossible, investigate!\n";
            continue;
        }

        stats.trialsSolved++;
        times.push_back(sol.timeSeconds);
        expandedCounts.push_back(sol.statesExpanded);
    }

    if (!times.empty()) {
        stats.minTime = *std::min_element(times.begin(), times.end());
        stats.maxTime = *std::max_element(times.begin(), times.end());
        stats.avgTime = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        stats.maxExpanded = *std::max_element(expandedCounts.begin(), expandedCounts.end());
        stats.avgExpanded = std::accumulate(expandedCounts.begin(), expandedCounts.end(), 0LL) / expandedCounts.size();
    }

    return stats;
}

void printHeader(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
    std::cout << std::left
               << std::setw(8)  << "Depth"
               << std::setw(12) << "MinTime(s)"
               << std::setw(12) << "AvgTime(s)"
               << std::setw(12) << "MaxTime(s)"
               << std::setw(14) << "AvgExpanded"
               << std::setw(14) << "MaxExpanded"
               << "\n";
}

void printRow(const DepthStats& s) {
    std::cout << std::left
               << std::setw(8)  << s.depth
               << std::setw(12) << std::fixed << std::setprecision(6) << s.minTime
               << std::setw(12) << std::fixed << std::setprecision(6) << s.avgTime
               << std::setw(12) << std::fixed << std::setprecision(6) << s.maxTime
               << std::setw(14) << s.avgExpanded
               << std::setw(14) << s.maxExpanded
               << "\n";
    std::cout.flush();
}

int main() {
    BFSSolver bfs;
    AStarSolver astar(std::make_shared<ManhattanDistance>());

    const unsigned SEED = 20250615;
    const int TRIALS_PER_DEPTH = 20;

    printHeader("BFS (shallow depths -- demonstrates why A* is needed)");
    for (int depth : {5, 10, 15, 18}) {
        printRow(runBenchmarkAtDepth(bfs, depth, TRIALS_PER_DEPTH, SEED + depth * 100));
    }
    printRow(runBenchmarkAtDepth(bfs, 20, 5, SEED + 2000));

    printHeader("A* + Manhattan Distance (full range, including target benchmarks)");
    for (int depth : {5, 10, 15, 18, 20, 23, 26, 29, 33, 36, 40}) {
        printRow(runBenchmarkAtDepth(astar, depth, TRIALS_PER_DEPTH, SEED + depth * 100));
    }

    std::cout << "\n=== Summary (use these to decide the resume claim) ===\n";
    for (int depth : {23, 33, 40}) {
        DepthStats s = runBenchmarkAtDepth(astar, depth, TRIALS_PER_DEPTH, SEED + depth * 1000);
        std::cout << "Depth " << depth << ": min=" << s.minTime << "s, avg=" << s.avgTime
                  << "s, worst-case=" << s.maxTime << "s (over " << s.trialsRun << " trials)\n";
    }

    return 0;
}