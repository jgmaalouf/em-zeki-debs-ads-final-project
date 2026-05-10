#include "Sorting.h"

static void merge(std::vector<Scored>& v, int lo, int mid, int hi) {
    std::vector<Scored> temp(hi - lo);

    int i = lo;
    int j = mid;
    int k = 0;

    while (i < mid && j < hi) {
        if (v[i].score >= v[j].score) {
            temp[k] = v[i];
            i = i + 1;
        } else {
            temp[k] = v[j];
            j = j + 1;
        }
        k = k + 1;
    }

    while (i < mid) {
        temp[k] = v[i];
        i = i + 1;
        k = k + 1;
    }
    while (j < hi) {
        temp[k] = v[j];
        j = j + 1;
        k = k + 1;
    }

    for (int x = 0; x < hi - lo; x = x + 1) {
        v[lo + x] = temp[x];
    }
}

static void mergeSortHelper(std::vector<Scored>& v, int lo, int hi) {
    if (hi - lo <= 1) {
        return;
    }
    int mid = lo + (hi - lo) / 2;
    mergeSortHelper(v, lo, mid);
    mergeSortHelper(v, mid, hi);
    merge(v, lo, mid, hi);
}

void mergeSort(std::vector<Scored>& v) {
    int n = static_cast<int>(v.size());
    if (n < 2) {
        return;
    }
    mergeSortHelper(v, 0, n);
}
