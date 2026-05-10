#ifndef ADS_SWIPE_DECK_H
#define ADS_SWIPE_DECK_H

#include <deque>
#include <stack>
#include <utility>
#include <vector>

#include "SwipeGraph.h"

// Which way the user swiped. Right = like, Left = pass.
enum class SwipeDir {
    Left,
    Right,
};

// What just happened on the last swipe. Returned to main so it can
// print "MATCH!" without having to re-query the graph.
struct SwipeOutcome {
    int swipedID;        // -1 if the deck was empty
    SwipeDir direction;
    SwipeResult result;
};

// Per-session view of the queue of profiles to look at, plus an undo
// stack. The deck owns no users - just IDs - so it's cheap to throw
// away and reload after a filter change.
class SwipeDeck {
public:
    explicit SwipeDeck(int ownerID);

    // Replace the queue with a fresh ranked list and clear undo
    // history (so you can't rewind across a reload).
    void loadDeck(const std::vector<int>& rankedIDs);

    // Look at the next ID without consuming it. -1 if empty.
    int peekNext() const;

    // Consume the front of the queue. swipeRight talks to the graph
    // so it can detect matches; swipeLeft is local-only.
    SwipeOutcome swipeRight(SwipeGraph& graph);
    SwipeOutcome swipeLeft();

    // Pop the most recent swipe off history and put that user back at
    // the front of the queue. If it was a right-swipe we also remove
    // the edge from the graph. Returns the restored ID, or -1 if
    // there's nothing to undo.
    int rewind(SwipeGraph& graph);

private:
    int ownerID_;
    std::deque<int> upcoming_;                              // front = next profile
    std::stack<std::pair<int, SwipeDir>> history_;          // most recent swipe on top
};

#endif
