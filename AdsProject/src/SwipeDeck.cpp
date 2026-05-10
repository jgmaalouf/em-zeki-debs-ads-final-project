#include "SwipeDeck.h"

SwipeDeck::SwipeDeck(int ownerID) : ownerID_(ownerID) {}

// Replace the queue and wipe history. We empty the stack with pop()
// in a loop because std::stack doesn't expose clear().
void SwipeDeck::loadDeck(const std::vector<int>& rankedIDs) {
    upcoming_.assign(rankedIDs.begin(), rankedIDs.end());
    while (!history_.empty()) history_.pop();
}

// Peek without consuming. -1 means "deck is empty".
int SwipeDeck::peekNext() const {
    return upcoming_.empty() ? -1 : upcoming_.front();
}

// Pop the front, write the edge into the graph, push onto history so
// rewind can undo it. The graph is the only thing that knows whether
// this swipe just made a match - we just pass that result along.
SwipeOutcome SwipeDeck::swipeRight(SwipeGraph& graph) {
    if (upcoming_.empty()) {
        return SwipeOutcome{-1, SwipeDir::Right, SwipeResult::NO_MATCH};
    }
    int target = upcoming_.front();
    upcoming_.pop_front();
    SwipeResult r = graph.swipeRight(ownerID_, target);
    history_.push({target, SwipeDir::Right});
    return SwipeOutcome{target, SwipeDir::Right, r};
}

// Same shape as swipeRight but without touching the graph - a left
// swipe is purely local. Result is always NO_MATCH because nothing
// could have matched.
SwipeOutcome SwipeDeck::swipeLeft() {
    if (upcoming_.empty()) {
        return SwipeOutcome{-1, SwipeDir::Left, SwipeResult::NO_MATCH};
    }
    int target = upcoming_.front();
    upcoming_.pop_front();
    history_.push({target, SwipeDir::Left});
    return SwipeOutcome{target, SwipeDir::Left, SwipeResult::NO_MATCH};
}

// Undo the last swipe. If it was a right-swipe we have to take the
// edge back out of the graph (otherwise rewinding would still leave
// you matched). The user is then put back at the *front* of the
// queue so they're the next profile shown.
int SwipeDeck::rewind(SwipeGraph& graph) {
    if (history_.empty()) return -1;
    auto [id, dir] = history_.top();
    history_.pop();
    if (dir == SwipeDir::Right) graph.unswipeRight(ownerID_, id);
    upcoming_.push_front(id);
    return id;
}
