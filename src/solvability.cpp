#include "../include/solvability.hpp"

// ----------------------------------------------------------------------------
// mergeAndCount: merges arr[left..mid] and arr[mid+1..right] (each already
// individually sorted by the recursive calls below), and counts inversions
// that span across the two halves.
//
// WHY THIS COUNTS INVERSIONS CORRECTLY:
//   By the time we merge, the left half and right half are each internally
//   sorted, but we haven't yet counted inversions BETWEEN a left-half
//   element and a right-half element.
//
//   While merging, we walk a pointer `i` through the left half and `j`
//   through the right half, always taking the smaller of arr[i]/arr[j] next.
//
//   If arr[j] < arr[i]: that means arr[j] is smaller than arr[i], BUT
//   arr[j] originally appeared AFTER arr[i] (right half comes after left
//   half in the original array). That's exactly the definition of an
//   inversion: earlier-but-larger paired with later-but-smaller.
//
//   Crucially, since the left half is sorted ascending, if arr[j] < arr[i],
//   then arr[j] is ALSO smaller than every element from arr[i] onward in
//   the left half (there are (mid - i + 1) of them). So instead of counting
//   one inversion and moving on, we count (mid - i + 1) inversions in one
//   shot -- this is what makes it O(n log n) instead of O(n^2): we never
//   compare every pair individually, we count whole blocks of inversions
//   at once whenever a right-element "jumps ahead" of remaining left-elements.
// ----------------------------------------------------------------------------
long long Solvability::mergeAndCount(std::vector<int>& arr, int left, int mid, int right) {
    std::vector<int> merged;
    merged.reserve(right - left + 1);

    int i = left;
    int j = mid + 1;
    long long invCount = 0;

    while (i <= mid && j <= right) {
        if (arr[i] <= arr[j]) {
            merged.push_back(arr[i++]);
        } else {
            // arr[j] < arr[i], and arr[i..mid] are all >= arr[i] (sorted),
            // so all of them form an inversion with arr[j].
            invCount += (mid - i + 1);
            merged.push_back(arr[j++]);
        }
    }
    while (i <= mid)   merged.push_back(arr[i++]);
    while (j <= right) merged.push_back(arr[j++]);

    // Write the merged, sorted result back into arr[left..right] so that
    // higher levels of recursion see a properly sorted subarray.
    for (int k = 0; k < static_cast<int>(merged.size()); ++k) {
        arr[left + k] = merged[k];
    }

    return invCount;
}

long long Solvability::mergeSortCount(std::vector<int>& arr, int left, int right) {
    if (left >= right) return 0; // 0 or 1 element: no inversions possible

    int mid = left + (right - left) / 2;
    long long count = 0;
    count += mergeSortCount(arr, left, mid);
    count += mergeSortCount(arr, mid + 1, right);
    count += mergeAndCount(arr, left, mid, right);
    return count;
}

int Solvability::countInversions(const PuzzleState& state) {
    // Flatten the board into a sequence, EXCLUDING the blank -- the blank
    // isn't a "tile" for inversion-counting purposes, only tiles 1-15 are
    // compared against each other.
    std::vector<int> tiles;
    tiles.reserve(PuzzleState::NUM_CELLS - 1);
    for (int i = 0; i < PuzzleState::NUM_CELLS; ++i) {
        int v = state.at(i);
        if (v != 0) tiles.push_back(v);
    }

    long long inv = mergeSortCount(tiles, 0, static_cast<int>(tiles.size()) - 1);
    return static_cast<int>(inv);
}

bool Solvability::isSolvable(const PuzzleState& state) {
    int inversions = countInversions(state);

    // Row of the blank, counted from the BOTTOM, 1-indexed.
    // PuzzleState::rowOf() gives row counted from the TOP, 0-indexed, so:
    //   rowFromTop (0-indexed) = blankPos / 4   (0 = top row, 3 = bottom row)
    //   rowFromBottom (1-indexed) = SIDE - rowFromTop
    int rowFromTop0Indexed = PuzzleState::rowOf(state.blankPos());
    int rowFromBottom1Indexed = PuzzleState::SIDE - rowFromTop0Indexed;

    bool inversionsEven = (inversions % 2 == 0);
    bool blankRowEven = (rowFromBottom1Indexed % 2 == 0);

    // Solvable iff exactly one of the two is even (opposite parity).
    // i.e. (blank row ODD and inversions EVEN) or (blank row EVEN and inversions ODD)
    return inversionsEven != blankRowEven;
}