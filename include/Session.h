#ifndef ADS_SESSION_H
#define ADS_SESSION_H

#include <vector>

#include "CityGraph.h"
#include "SwipeDeck.h"
#include "SwipeGraph.h"
#include "UserDatabase.h"

// User-tunable knobs that drive who ends up in the deck.
// `maxDistanceKm` defaults to "effectively unlimited" so a fresh
// session shows everything until the user narrows it.
struct Filters {
    int minAge = 18;
    int maxAge = 99;
    int maxDistanceKm = 100000;
};

// Per-signed-in-user state. Holds references (not copies) to the
// shared DB and graphs - they outlive any single Session - and owns
// the deck plus an "already seen" set so we don't reshow profiles
// after a filter change.
class Session {
public:
    Session(int userID,
            UserDatabase& db,
            SwipeGraph& swipeGraph,
            CityGraph& cityGraph);

    int userID() const;
    SwipeDeck& deck();
    SwipeGraph& swipeGraph();

    // Rebuild the deck against the latest filters. Returns the size
    // of the new deck. Anyone in alreadySeen_ is skipped.
    int loadDeck(const Filters& f);

    // Mark a user as seen so they don't reappear on the next
    // loadDeck. Both swipes call this from main.
    void markSeen(int target);

private:
    int userID_;
    UserDatabase& db_;
    SwipeGraph& swipeGraph_;
    CityGraph& cityGraph_;
    SwipeDeck deck_;
    std::vector<int> alreadySeen_;  // ids that should never come back into the deck
};

#endif
