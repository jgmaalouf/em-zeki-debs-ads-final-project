#ifndef ADS_COMPATIBILITY_H
#define ADS_COMPATIBILITY_H

#include "User.h"

// Scores how good a match `a` and `b` are. Higher = better.
// Distance is passed in (rather than computed here) because it's
// already on hand at the call site and recomputing it would mean
// passing the city graph all the way down.
int compatibilityScore(const User& a, const User& b, int distanceKm);

#endif
