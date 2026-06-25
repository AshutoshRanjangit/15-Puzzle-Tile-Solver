#include "../include/heuristic.hpp"
#include "../include/puzzle_state.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <random>

int main() {
    ManhattanDistance h;

    // Test 1: goal state -> heuristic must be exactly 0 (every tile already home)
    {
        PuzzleState g;
        assert(h.estimate(g) == 0);
        std::cout << "Test 1 passed: goal state has Manhattan distance 0\n";
    }

    // Test 2: hand-verified example from literature (Hashim et al., arxiv 2302.02985)
    // Start state (their Figure 1a), row-major with 0 = blank:
    //   t1 t4 t2 t3
    //   t1... wait -- using their exact tile sequence:
    //   <t1,t4,t2,t3, t1,t6,t7,t8, t5,t10,t11,t0, t9,t14,t15,t12>
    // NOTE: their notation has a typo-looking repeat of t1; we instead
    // reconstruct an equivalent test using OUR OWN clean board and hand
    // compute it ourselves, to avoid propagating a possible transcription
    // error from the source. This is safer than blindly trusting OCR'd
    // text from a paper.
    {
        // Custom board, hand-computed manually:
        // 1  2  3  4
        // 5  6  7  8
        // 9  10 11 0
        // 13 14 15 12
        //
        // Every tile is home EXCEPT: 12 (at index 15, should be at index 11)
        // and the blank (at index 11, "should" be at index 15 -- but blank excluded).
        // Tile 12: current row=3,col=3. goal row=2,col=3. |3-2|+|3-3| = 1.
        // So total Manhattan distance should be exactly 1.
        std::array<int, 16> cells = {
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 0,
            13, 14, 15, 12
        };
        PuzzleState s(cells);
        assert(h.estimate(s) == 1);
        std::cout << "Test 2 passed: single-tile-displaced board has h=1 (hand-verified)\n";
    }

    // Test 3: a board with multiple displaced tiles, hand-computed.
    // 2  1  3  4      tile 2 at (0,0) goal(0,1): dist 1
    // 5  6  7  8                tile 1 at (0,1) goal(0,0): dist 1
    // 9  10 11 12
    // 13 14 0  15      blank at (3,2); tile 15 at (3,3) goal(3,2): dist 1
    // Total expected: 1 + 1 + 1 = 3
    {
        std::array<int, 16> cells = {
            2, 1, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 14, 0, 15
        };
        PuzzleState s(cells);
        assert(h.estimate(s) == 3);
        std::cout << "Test 3 passed: multi-tile-displaced board has h=3 (hand-verified)\n";
    }

    // IMPORTANT LESSON LEARNED: an earlier version of this test hand-typed a
    // move sequence (UP, LEFT, DOWN, RIGHT, UP, RIGHT, ...) based on
    // intuition about board geometry, WITHOUT checking legalMoves() at each
    // step. That sequence included an illegal move partway through (RIGHT
    // when the blank was already in the rightmost column). applyMove() does
    // NOT validate legality by design (the header explicitly documents
    // "caller must ensure m is legal") -- so it silently produced a corrupted
    // board, which then made the heuristic look "inconsistent" when actually
    // the heuristic was fine and the TEST was feeding it bad input.
    //
    // Fix: always pick moves FROM legalMoves() at each step, using a
    // deterministic seeded RNG, so the sequence is guaranteed legal and the
    // test is still reproducible (same seed -> same sequence every run).
    auto pickLegalMoveSequence = [](PuzzleState start, int count, unsigned seed) {
        std::mt19937 rng(seed);
        std::vector<PuzzleState::Move> seq;
        PuzzleState cur = start;
        for (int i = 0; i < count; ++i) {
            auto legal = cur.legalMoves();
            std::uniform_int_distribution<size_t> dist(0, legal.size() - 1);
            PuzzleState::Move chosen = legal[dist(rng)];
            seq.push_back(chosen);
            cur = cur.applyMove(chosen);
        }
        return seq;
    };

    // Test 4: ADMISSIBILITY spot-check. Take the goal state, apply N legal
    // moves to reach some state S. The TRUE shortest distance from S to goal
    // is AT MOST N (since we just proved a path of length N exists by
    // construction -- just walk it backwards). Admissibility requires
    // h(S) <= true_distance <= N, so h(S) <= N must hold at every prefix.
    {
        PuzzleState cur;
        auto moveSeq = pickLegalMoveSequence(cur, 30, /*seed=*/7);
        for (size_t i = 0; i < moveSeq.size(); ++i) {
            cur = cur.applyMove(moveSeq[i]);
            int hVal = h.estimate(cur);
            int movesSoFar = static_cast<int>(i + 1);
            assert(hVal <= movesSoFar); // admissibility: h must not exceed a KNOWN upper bound
        }
        std::cout << "Test 4 passed: admissibility holds (h never exceeds known reachable distance) across "
                  << moveSeq.size() << " legal moves\n";
    }

    // Test 5: CONSISTENCY spot-check -- since exactly one tile moves per turn,
    // and that tile's Manhattan distance to its goal can change by at most 1
    // per move, the TOTAL heuristic can change by at most 1 between a parent
    // state and any of its (legal) children. This is the "triangle
    // inequality" / consistency property mentioned in the Princeton course
    // notes -- useful as an automated sanity check, since violating it would
    // indicate a bug in either the heuristic or the move logic.
    {
        PuzzleState cur;
        auto moveSeq = pickLegalMoveSequence(cur, 30, /*seed=*/99);
        int prevH = h.estimate(cur);
        for (auto m : moveSeq) {
            PuzzleState next = cur.applyMove(m);
            int nextH = h.estimate(next);
            assert(std::abs(nextH - prevH) <= 1);
            cur = next;
            prevH = nextH;
        }
        std::cout << "Test 5 passed: consistency holds (h changes by at most 1 per legal move) across "
                  << moveSeq.size() << " moves\n";
    }

    // Test 6: heuristic should generally INCREASE as we scramble further from
    // goal, though not monotonically guaranteed every single step -- so we
    // check it loosely: after 10 legal moves of scrambling, h should be
    // noticeably larger than after 2 legal moves, on average. (Not a strict
    // invariant, just a smoke test that the heuristic is actually responsive
    // to scrambling.)
    {
        PuzzleState g;
        auto shallowSeq = pickLegalMoveSequence(g, 2, /*seed=*/3);
        PuzzleState shallow = g;
        for (auto m : shallowSeq) shallow = shallow.applyMove(m);

        auto deeperSeq = pickLegalMoveSequence(g, 10, /*seed=*/3); // same seed/prefix as shallow, extended
        PuzzleState deeper = g;
        for (auto m : deeperSeq) deeper = deeper.applyMove(m);

        int hShallow = h.estimate(shallow);
        int hDeeper = h.estimate(deeper);
        std::cout << "Test 6 info: h(2-move scramble)=" << hShallow
                  << ", h(10-move scramble)=" << hDeeper << " (expect deeper >= shallow, generally)\n";
        // Not asserting strictly since this isn't a hard guarantee, just observing.
    }

    std::cout << "\nAll Heuristic tests passed!\n";
    return 0;
}