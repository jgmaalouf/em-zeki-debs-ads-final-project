#ifndef ADS_SWIPE_GRAPH_H
#define ADS_SWIPE_GRAPH_H

#include <unordered_map>
#include <vector>

// Outcome of a right-swipe: either it created a mutual pair or not.
// Left-swipes don't return this - they're always NO_MATCH by definition.
enum class SwipeResult {
    NO_MATCH,
    MATCH,
};

// Directed graph of right-swipes. An edge from A to B means A liked B.
// A "match" is a pair of edges in both directions - it isn't stored
// separately, we just check for the reverse edge.
//
// Left-swipes don't appear here at all. We track those only via the
// "already seen" list on the Session, since they don't need to survive.
class SwipeGraph {
public:
    // Records a right-swipe. Returns MATCH iff `to` had previously
    // right-swiped `from` (so the edge already existed in reverse).
    SwipeResult swipeRight(int from, int to);

    // Used by rewind - removes the edge as if the swipe never happened.
    void unswipeRight(int from, int to);

    bool hasEdge(int from, int to) const;

    // All users that mutually liked `userID` (i.e. matches).
    std::vector<int> getMatches(int userID) const;

    // BFS over the mutual-edge subgraph. Returns user IDs at distance
    // 2..maxDegree from `userID`, excluding self and direct matches.
    // Used to surface "people your matches matched with" - the classic
    // friend-of-friend recommendation pattern.
    std::vector<int> getSuggestionsByDegree(int userID, int maxDegree) const;

private:
    // Adjacency map: from -> list of users `from` has right-swiped.
    // We insert-if-absent so repeat right-swipes stay idempotent.
    std::unordered_map<int, std::vector<int>> outEdges_;
};

#endif
