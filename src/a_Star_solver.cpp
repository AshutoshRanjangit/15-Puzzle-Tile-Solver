#include "../include/astar_solver.hpp"
#include "../include/solvability.hpp"
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <limits>

AStarSolver::AStarSolver(std::shared_ptr<Heuristic> heuristic)
    : heuristic_(std::move(heuristic)) {}

std::string AStarSolver::name() const {
    return "A* (" + heuristic_->name() + ")";
}

namespace {

// A single entry in the open set (priority queue).
// g = exact cost from start (number of moves so far)
// f = g + h(state), the priority used for ordering
struct Node {
    PuzzleState state;
    int g;
    int f;
};

// std::priority_queue is a MAX-HEAP by default: it pops whichever element
// the comparator considers "greatest". We want the SMALLEST f popped first
// (that's the whole point of A* -- explore the most promising state next).
// So we define our comparator such that a "larger" f counts as "less than"
// in heap-order terms -- i.e. we invert the natural comparison. This makes
// the node with the SMALLEST f behave like the heap's maximum, so it's
// the one that gets popped.
struct CompareNode {
    bool operator()(const Node& a, const Node& b) const {
        return a.f > b.f; // inverted: smaller f = higher priority = popped first
    }
};

} // anonymous namespace

Solution AStarSolver::solve(const PuzzleState& start) {
    Solution result;

    // ---- Step 0: solvability gate (identical reasoning to BFSSolver) ------
    if (!Solvability::isSolvable(start)) {
        result.solvable = false;
        return result;
    }
    result.solvable = true;

    auto t0 = std::chrono::steady_clock::now();

    if (start.isGoal()) {
        result.timeSeconds = 0.0;
        return result;
    }

    // ---- bestG: best known g (cost from start) for each state seen so far.
    // Doubles as our "have we seen this" check AND lets us detect when a
    // popped queue entry is STALE (we later found a cheaper way to reach
    // that same state, so this entry's g/f are out of date and should be
    // skipped rather than re-expanded).
    std::unordered_map<PuzzleState, int, PuzzleStateHash> bestG;

    // Parent map for path reconstruction, same idea as BFSSolver: keyed by
    // child state, storing {parent, move used}. We backtrack from goal to
    // start through this map once we find the goal.
    struct ParentInfo {
        PuzzleState parent;
        PuzzleState::Move moveUsed;
    };
    std::unordered_map<PuzzleState, ParentInfo, PuzzleStateHash> parentOf;

    std::priority_queue<Node, std::vector<Node>, CompareNode> openSet;

    int startH = heuristic_->estimate(start);
    openSet.push(Node{start, 0, startH});
    bestG[start] = 0;
    result.statesGenerated = 1;

    bool found = false;

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        // Stale entry check: if we've since found a strictly better g for
        // this exact state, this popped entry is outdated -- skip it.
        // (With an admissible+consistent heuristic like Manhattan distance,
        // this should rarely if ever trigger in practice, but it's a
        // correctness safeguard, not just an optimization -- without it,
        // a non-consistent heuristic could cause incorrect results.)
        auto bestIt = bestG.find(current.state);
        if (bestIt != bestG.end() && current.g > bestIt->second) {
            continue;
        }

        result.statesExpanded++;

        if (current.state.isGoal()) {
            found = true;
            break;
        }

        for (PuzzleState::Move m : current.state.legalMoves()) {
            PuzzleState next = current.state.applyMove(m);
            int tentativeG = current.g + 1;

            auto it = bestG.find(next);
            bool isNewOrBetter = (it == bestG.end()) || (tentativeG < it->second);

            if (isNewOrBetter) {
                bestG[next] = tentativeG;
                parentOf[next] = ParentInfo{current.state, m};
                int h = heuristic_->estimate(next);
                openSet.push(Node{next, tentativeG, tentativeG + h});
                result.statesGenerated++;
            }
        }
    }

    // ---- Path reconstruction (backtracking from goal to start) ------------
    if (found) {
        std::vector<PuzzleState::Move> reversedMoves;
        PuzzleState cur = PuzzleState::goal();
        while (!(cur == start)) {
            auto it = parentOf.find(cur);
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