#include "Compatibility.h"

#include <algorithm>
#include <cstdlib>

#include "UserDatabase.h"

namespace {

// Knobs. Bumping these changes how aggressively the deck favours
// shared interests vs. close Elo vs. short distance. Numbers were
// picked so that one shared interest is worth roughly five km of
// distance - tweak to taste.
constexpr int INTEREST_WEIGHT = 10;
constexpr int ELO_WEIGHT = 5;
constexpr int DISTANCE_WEIGHT = 2;

// Maps an Elo gap to a 0..100 "closeness" number. Every 10 Elo points
// of gap docks one point. Clamped at 0 so very wide gaps don't go
// negative and start hurting the score twice (distance already does).
int eloProximity(int eloA, int eloB) {
    int gap = std::abs(eloA - eloB);
    return std::max(0, 100 - gap / 10);
}

}

// Final score: more shared interests pulls it up, similar Elo pulls it
// up, distance pulls it down. The sign of distance is negative on
// purpose - far-away matches should rank lower even if interests
// otherwise match.
int compatibilityScore(const User& a, const User& b, int distanceKm) {
    int shared = countSharedInterests(a, b);
    int prox = eloProximity(a.eloScore, b.eloScore);
    return shared * INTEREST_WEIGHT
         + prox * ELO_WEIGHT
         - distanceKm * DISTANCE_WEIGHT;
}
