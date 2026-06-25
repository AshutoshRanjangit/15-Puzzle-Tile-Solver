#include "../include/solvability.hpp"
#include "../include/puzzle_state.hpp"
#include <iostream>
#include <cassert>

int main() {
    // Test 1: goal state itself must be solvable (0 inversions, trivial)
    {
        PuzzleState g;
        assert(Solvability::countInversions(g) == 0);
        assert(Solvability::isSolvable(g) == true);
        std::cout << "Test 1 passed: goal state is solvable (0 inversions)\n";
    }

    // Test 2: any state reached by ONE legal move from goal must still be
    // solvable, since solvability is invariant under legal moves by
    // definition (we can always move back).
    {
        PuzzleState g;
        for (auto m : g.legalMoves()) {
            PuzzleState next = g.applyMove(m);
            assert(Solvability::isSolvable(next) == true);
        }
        std::cout << "Test 2 passed: states one move from goal are solvable\n";
    }

    // Test 3: classic UNSOLVABLE configuration -- swap the last two tiles
    // (14 and 15) in the goal layout, blank stays in bottom-right corner.
    // This is THE textbook example of an unsolvable 15-puzzle state.
    // Goal:        1  2  3  4 / 5 6 7 8 / 9 10 11 12 / 13 14 15 _
    // Unsolvable:  1  2  3  4 / 5 6 7 8 / 9 10 11 12 / 13 15 14 _
    {
        std::array<int, 16> cells = {
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 15, 14, 0
        };
        PuzzleState s(cells);
        // blank is at index 15 -> row 3 (0-indexed from top) -> row 1 from bottom (odd)
        // inversions: only pair (15,14) is inverted -> 1 inversion (odd)
        // rule: blank row from bottom ODD requires inversions EVEN to be solvable.
        // Here inversions is ODD -> NOT solvable.
        assert(Solvability::countInversions(s) == 1);
        assert(Solvability::isSolvable(s) == false);
        std::cout << "Test 3 passed: single adjacent swap (14,15) correctly detected as UNSOLVABLE\n";
    }

    // Test 4: reverse order 15,14,...,1 with blank last -- a commonly cited
    // example. With blank in bottom-right (row 1 from bottom, odd) and
    // 15 tiles fully reversed, inversion count = 15*14/2 = 105 (odd).
    // Odd blank-row requires EVEN inversions to be solvable -> 105 is odd -> unsolvable
    // UNLESS we also account for swapping last two tiles, which several
    // sources mention as the fix. Let's verify our raw reversed case first.
    {
        std::array<int, 16> cells = {
            15, 14, 13, 12,
            11, 10, 9, 8,
            7, 6, 5, 4,
            3, 2, 1, 0
        };
        PuzzleState s(cells);
        int inv = Solvability::countInversions(s);
        assert(inv == 105); // 15 choose 2 = 105, every pair is inverted
        // blank row from bottom = 1 (odd) -> needs even inversions -> 105 is odd -> unsolvable
        assert(Solvability::isSolvable(s) == false);
        std::cout << "Test 4 passed: fully reversed (105 inversions, odd) correctly UNSOLVABLE\n";
    }

    // Test 5: take test 4's board and swap the first two tiles (15,14 -> 14,15)
    // This should flip inversion parity by exactly 1 (105 -> 104, even),
    // matching the cited fix ("swap last two tiles" in some conventions,
    // here it's swapping based on whichever pair is adjacent and out of order
    // at the start) making it solvable.
    {
        std::array<int, 16> cells = {
            14, 15, 13, 12,
            11, 10, 9, 8,
            7, 6, 5, 4,
            3, 2, 1, 0
        };
        PuzzleState s(cells);
        int inv = Solvability::countInversions(s);
        assert(inv == 104); // one fewer inversion than the fully reversed case
        assert(Solvability::isSolvable(s) == true);
        std::cout << "Test 5 passed: swapping first two tiles of reversed board -> SOLVABLE (104 inversions, even)\n";
    }

    // Test 6: cross-check countInversions against a brute-force O(n^2) count
    // on a handful of boards, to make sure the merge-sort version isn't
    // secretly buggy in a way that happens to pass the hand-checked cases above.
    {
        auto bruteForceInversions = [](const PuzzleState& s) {
            std::vector<int> tiles;
            for (int i = 0; i < 16; ++i) {
                int v = s.at(i);
                if (v != 0) tiles.push_back(v);
            }
            int count = 0;
            for (size_t a = 0; a < tiles.size(); ++a)
                for (size_t b = a + 1; b < tiles.size(); ++b)
                    if (tiles[a] > tiles[b]) count++;
            return count;
        };

        std::array<int, 16> cells1 = {
            5, 1, 2, 4,
            3, 6, 7, 8,
            9, 0, 11, 12,
            13, 10, 14, 15
        };
        PuzzleState s1(cells1);
        assert(Solvability::countInversions(s1) == bruteForceInversions(s1));

        std::array<int, 16> cells2 = {
            0, 15, 14, 13,
            12, 11, 10, 9,
            8, 7, 6, 5,
            4, 3, 2, 1
        };
        PuzzleState s2(cells2);
        assert(Solvability::countInversions(s2) == bruteForceInversions(s2));

        std::cout << "Test 6 passed: merge-sort inversion count matches brute-force O(n^2) count\n";
    }

    std::cout << "\nAll Solvability tests passed!\n";
    return 0;
}