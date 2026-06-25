#include "../include/bfs_solver.hpp"
#include "../include/solvability.hpp"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <chrono>

Solution BFSSolver::solve(const PuzzleState& start) {
    Solution result;

    // ---- Step 0: solvability gate ------------------------------------------
    // Never search for something that provably can't exist. Without this,
    // an unsolvable input would make BFS enumerate the ENTIRE reachable
    // state space (trillions of states) before giving up -- effectively
    // never terminating in practice.
    if (!Solvability::isSolvable(start)) {
        result.solvable = false;
        return result;
    }
    result.solvable = true;

    auto t0 = std::chrono::steady_clock::now();

    if (start.isGoal()) {
        result.timeSeconds = 0.0;
        return result; // already solved, zero moves
    }

    // ---- Parent map for path reconstruction --------------------------------
    // Keyed by the CHILD state, storing the parent it came from and the move
    // that produced it. This is what lets us "backtrack" (walk backward)
    // from the goal once we find it, to recover the actual move sequence --
    // BFS alone only tells us we reached the goal, not how.
    struct ParentInfo {
        PuzzleState parent;
        PuzzleState::Move moveUsed;
    };
    std::unordered_map<PuzzleState, ParentInfo, PuzzleStateHash> parentOf;

    std::unordered_set<PuzzleState, PuzzleStateHash> visited;
    std::queue<PuzzleState> frontier;

    frontier.push(start);
    visited.insert(start);
    result.statesGenerated = 1;

    bool found = false;

    while (!frontier.empty() && !found) {
        PuzzleState current = frontier.front();
        frontier.pop();
        result.statesExpanded++;

        for (PuzzleState::Move m : current.legalMoves()) {
            PuzzleState next = current.applyMove(m);

            // Skip states we've already discovered -- critical for BFS
            // tractability, since many different move sequences lead to
            // the same board (e.g. UP then LEFT vs LEFT then UP from
            // certain positions can converge).
            if (visited.count(next)) continue;

            visited.insert(next);
            parentOf[next] = ParentInfo{current, m};
            result.statesGenerated++;

            if (next.isGoal()) {
                found = true;
                break; // no need to keep expanding current's other moves
            }

            frontier.push(next);
        }
    }

    // ---- Path reconstruction (backtracking from goal to start) ------------
    if (found) {
        std::vector<PuzzleState::Move> reversedMoves;
        PuzzleState cur = PuzzleState::goal();
        while (!(cur == start)) {
            auto it = parentOf.find(cur);
            // it should always be found here; if not, something upstream
            // is broken (defensive check rather than silent corruption)
            reversedMoves.push_back(it->second.moveUsed);
            cur = it->second.parent;
        }
        std::reverse(reversedMoves.begin(), reversedMoves.end());
        result.moves = std::move(reversedMoves);
    }

    auto t1 = std::chrono::steady_clock::now();
    result.timeSeconds = std::chrono::duration<double>(t1 - t0).count();

    return result;
}