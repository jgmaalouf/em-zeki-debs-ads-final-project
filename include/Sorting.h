#ifndef ADS_SORTING_H
#define ADS_SORTING_H

#include <utility>
#include <vector>

// Generic, in-place merge sort.
//
// `cmp(a, b)` should return true when `a` should come before `b` in
// the final order - same convention as std::sort. We allocate one
// scratch buffer up front and reuse it across all merges, so memory
// is O(n) instead of O(n log n).
//
// Recursion is implemented with the "explicit-self lambda" trick so
// we don't need to declare a separate helper function. The lambda
// takes itself as `self` and recurses through that.
template <typename T, typename Compare>
void mergeSort(std::vector<T>& v, Compare cmp) {
    std::vector<T> buffer(v.size());

    // Merge two adjacent sorted runs [lo, mid) and [mid, hi) into
    // buffer, then copy back. We move elements (not copy) so this is
    // cheap even for big values like strings.
    auto merge = [&](std::size_t lo, std::size_t mid, std::size_t hi) {
        std::size_t i = lo, j = mid, k = lo;
        while (i < mid && j < hi) {
            // Take from the right run iff its head is strictly less
            // than the left's. The "strictly" keeps the sort stable.
            if (cmp(v[j], v[i])) buffer[k++] = std::move(v[j++]);
            else                 buffer[k++] = std::move(v[i++]);
        }
        while (i < mid) buffer[k++] = std::move(v[i++]);
        while (j < hi) buffer[k++] = std::move(v[j++]);
        for (std::size_t x = lo; x < hi; ++x) v[x] = std::move(buffer[x]);
    };

    auto rec = [&](auto& self, std::size_t lo, std::size_t hi) -> void {
        if (hi - lo <= 1) return;  // 0 or 1 elements - already sorted
        std::size_t mid = lo + (hi - lo) / 2;
        self(self, lo, mid);
        self(self, mid, hi);
        merge(lo, mid, hi);
    };

    if (v.size() > 1) rec(rec, 0, v.size());
}

#endif
