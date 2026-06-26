// ============================================================================
// main.cpp -- CLI entry point
// ----------------------------------------------------------------------------
// USAGE:
//   puzzle15 scramble <depth> <bfs|astar>
//   puzzle15 custom "<16 space-separated values, 0=blank>" <bfs|astar>
// ============================================================================

#include "../include/puzzle_state.hpp"
#include "../include/solvability.hpp"
#include "../include/bfs_solver.hpp"
#include "../include/astar_solver.hpp"
#include "../include/heuristic.hpp"

#include <iostream>
#include <sstream>
#include <memory>
#include <random>
#include <algorithm>
#include <array>
#include <string>
#include <stdexcept>

namespace {

std::string moveToString(PuzzleState::Move m) {
    switch (m) {
        case PuzzleState::Move::UP:    return "UP";
        case PuzzleState::Move::DOWN:  return "DOWN";
        case PuzzleState::Move::LEFT:  return "LEFT";
        case PuzzleState::Move::RIGHT: return "RIGHT";
    }
    return "?";
}

PuzzleState::Move oppositeMove(PuzzleState::Move m) {
    switch (m) {
        case PuzzleState::Move::UP:    return PuzzleState::Move::DOWN;
        case PuzzleState::Move::DOWN:  return PuzzleState::Move::UP;
        case PuzzleState::Move::LEFT:  return PuzzleState::Move::RIGHT;
        case PuzzleState::Move::RIGHT: return PuzzleState::Move::LEFT;
    }
    return m;
}

PuzzleState scrambleByMoves(int numMoves) {
    std::mt19937 rng(std::random_device{}());
    PuzzleState cur;
    PuzzleState::Move lastMove = PuzzleState::Move::UP;
    bool hasLast = false;

    for (int i = 0; i < numMoves; ++i) {
        auto moves = cur.legalMoves();
        if (hasLast) {
            auto opp = oppositeMove(lastMove);
            moves.erase(std::remove(moves.begin(), moves.end(), opp), moves.end());
        }
        std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
        auto chosen = moves[dist(rng)];
        cur = cur.applyMove(chosen);
        lastMove = chosen;
        hasLast = true;
    }
    return cur;
}

void printSolutionReport(const PuzzleState& start, const Solution& sol, const std::string& solverName) {
    std::cout << "\nStart state:\n" << start.toString();

    if (!sol.solvable) {
        std::cout << "\nThis configuration is UNSOLVABLE (failed inversion-count/blank-row parity check).\n";
        std::cout << "No search was performed -- BFS/A* would never terminate on an unsolvable input.\n";
        return;
    }

    std::cout << "\nSolver: " << solverName << "\n";
    std::cout << "Solution length: " << sol.pathLength() << " moves\n";
    std::cout << "States expanded: " << sol.statesExpanded << "\n";
    std::cout << "States generated: " << sol.statesGenerated << "\n";
    std::cout << "Time taken: " << sol.timeSeconds << " seconds\n";

    if (sol.pathLength() > 0) {
        std::cout << "\nMove sequence:\n";
        for (size_t i = 0; i < sol.moves.size(); ++i) {
            std::cout << (i + 1) << ". " << moveToString(sol.moves[i]) << "\n";
        }
    } else {
        std::cout << "\n(Already solved -- no moves needed.)\n";
    }
}

std::unique_ptr<ISolver> makeSolver(const std::string& name) {
    if (name == "bfs") return std::make_unique<BFSSolver>();
    if (name == "astar") return std::make_unique<AStarSolver>(std::make_shared<ManhattanDistance>());
    return nullptr;
}

void printUsage() {
    std::cout<<
        "Usage:\n"
        "  puzzle15 scramble <depth> <bfs|astar>\n"
        "      Generate a scramble (random legal moves) and solve it.\n"
        "      Example: puzzle15 scramble 23 astar\n"
        "\n"
        "  puzzle15 custom \"<16 space-separated values, 0=blank>\" <bfs|astar>\n"
        "      Solve a specific board you provide, row-major order.\n"
        "      Example: puzzle15 custom \"1 2 3 4 5 6 7 8 9 10 11 0 13 14 15 12\" astar\n";
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "scramble") {
        if (argc != 4) { printUsage(); return 1; }
        int depth = std::stoi(argv[2]);
        std::string solverName = argv[3];

        auto solver = makeSolver(solverName);
        if (!solver) {
            std::cerr << "Unknown solver '" << solverName << "', expected 'bfs' or 'astar'\n";
            return 1;
        }

        PuzzleState scrambled = scrambleByMoves(depth);
        Solution sol = solver->solve(scrambled);
        printSolutionReport(scrambled, sol, solver->name());

    } else if (mode == "custom") {
        if (argc != 4) { printUsage(); return 1; }
        std::string boardStr = argv[2];
        std::string solverName = argv[3];

        auto solver = makeSolver(solverName);
        if (!solver) {
            std::cerr << "Unknown solver '" << solverName << "', expected 'bfs' or 'astar'\n";
            return 1;
        }

        std::array<int, 16> cells;
        std::istringstream iss(boardStr);
        int count = 0;
        int val;
        while (iss >> val && count < 16) {
            cells[count++] = val;
        }
        if (count != 16) {
            std::cerr << "Error: expected exactly 16 values, got " << count << "\n";
            return 1;
        }

        PuzzleState state;
        try {
            state = PuzzleState(cells);
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }

        Solution sol = solver->solve(state);
        printSolutionReport(state, sol, solver->name());

    } else {
        printUsage();
        return 1;
    }

    return 0;
}