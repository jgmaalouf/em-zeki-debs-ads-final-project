#include "Session.h"

#include <algorithm>
#include <vector>

#include "Compatibility.h"
#include "Sorting.h"

Session::Session(int userID,
                 UserDatabase& db,
                 SwipeGraph& swipeGraph,
                 CityGraph& cityGraph)
    : userID_(userID),
      db_(db),
      swipeGraph_(swipeGraph),
      cityGraph_(cityGraph),
      deck_(userID) {}

int Session::userID() const { return userID_; }
SwipeDeck& Session::deck() { return deck_; }
SwipeGraph& Session::swipeGraph() { return swipeGraph_; }
void Session::markSeen(int target) {
    // Insert-if-absent so a repeated mark is a no-op.
    if (std::find(alreadySeen_.begin(), alreadySeen_.end(), target) == alreadySeen_.end()) {
        alreadySeen_.push_back(target);
    }
}

// Build the deck from scratch. Walk every user, drop the ones that
// fail any filter, score what's left, sort by score descending, and
// hand the IDs to the deck.
//
// The merge sort is used here (instead of std::sort) because the
// project shows off custom algorithms - functionally a stable sort
// would do the same job.
int Session::loadDeck(const Filters& f) {
    const User* me = db_.getUser(userID_);
    if (me == nullptr) return 0;  // session over a deleted user; bail

    std::vector<Scored> kept;
    for (const auto& [id, other] : db_.getAllUsers()) {
        if (id == userID_) continue;                                  // can't match yourself
        if (other.age < f.minAge || other.age > f.maxAge) continue;   // age band
        if (std::find(alreadySeen_.begin(), alreadySeen_.end(), id) != alreadySeen_.end())
            continue;                                                 // already swiped this session
        if (swipeGraph_.hasEdge(userID_, id)) continue;               // already right-swiped in any session (incl. matches)
        if (db_.isBlocked(userID_, id) || db_.isBlocked(id, userID_)) // blocks are bidirectional in effect
            continue;
        int distance = cityGraph_.getDistance(me->city, other.city);
        if (distance == CityGraph::kUnreachable) continue;            // disconnected city
        if (distance > f.maxDistanceKm) continue;                     // too far
        kept.push_back({id, compatibilityScore(*me, other, distance)});
    }

    // Sort by score descending. mergeSort always orders Scored that
    // way, so no comparator argument is needed.
    mergeSort(kept);

    // Strip out the score column - the deck only needs IDs.
    std::vector<int> rankedIDs;
    rankedIDs.reserve(kept.size());
    for (const auto& s : kept) rankedIDs.push_back(s.id);
    deck_.loadDeck(rankedIDs);
    return static_cast<int>(rankedIDs.size());
}
