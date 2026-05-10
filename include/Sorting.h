#ifndef ADS_SORTING_H
#define ADS_SORTING_H

#include <vector>

// A user id paired with the compatibility score we computed for them.
// We sort vectors of these to rank candidates for the swipe deck.
struct Scored {
    int id;
    int score;
};

// Sorts the vector in DESCENDING order of score (highest score first).
void mergeSort(std::vector<Scored>& v);

#endif
