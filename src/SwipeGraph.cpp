#include "SwipeGraph.h"

#include <algorithm>
#include <queue>

namespace {

// Linear "is x in v?" - small adjacency lists make this cheap.
bool contains(const std::vector<int>& v, int x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}

}

// Record a right-swipe and tell the caller if it just made a match.
// Self-swipes are quietly dropped - shouldn't happen, but cheap to guard.
SwipeResult SwipeGraph::swipeRight(int from, int to) {
    if (from == to) return SwipeResult::NO_MATCH;
    auto& edges = outEdges_[from];  // [] auto-creates the inner vector
    if (!contains(edges, to)) edges.push_back(to);  // keep idempotent
    auto it = outEdges_.find(to);
    if (it != outEdges_.end() && contains(it->second, from)) {
        // The other side had already liked us - it's a match.
        return SwipeResult::MATCH;
    }
    return SwipeResult::NO_MATCH;
}

// Remove the edge. Used by deck rewind so a "bring it back" undoes
// any match the swipe might have produced.
void SwipeGraph::unswipeRight(int from, int to) {
    auto it = outEdges_.find(from);
    if (it == outEdges_.end()) return;
    auto& edges = it->second;
    edges.erase(std::remove(edges.begin(), edges.end(), to), edges.end());
}

bool SwipeGraph::hasEdge(int from, int to) const {
    auto it = outEdges_.find(from);
    if (it == outEdges_.end()) return false;
    return contains(it->second, to);
}

// Walk all out-edges from this user and keep the ones where the
// reverse edge exists. That's exactly the set of mutual matches.
std::vector<int> SwipeGraph::getMatches(int userID) const {
    std::vector<int> matches;
    auto it = outEdges_.find(userID);
    if (it == outEdges_.end()) return matches;
    for (int v : it->second) {
        auto rev = outEdges_.find(v);
        if (rev != outEdges_.end() && contains(rev->second, userID)) {
            matches.push_back(v);
        }
    }
    return matches;
}

namespace {

// Helper: do u and v mutually like each other? Used as the edge
// predicate in the FoF BFS - we only step across confirmed matches.
bool mutual(const std::unordered_map<int, std::vector<int>>& g,
            int u, int v) {
    auto a = g.find(u);
    if (a == g.end() || !contains(a->second, v)) return false;
    auto b = g.find(v);
    return b != g.end() && contains(b->second, u);
}

}

// Friend-of-friend search. Plain BFS over the mutual-match subgraph
// starting from `userID`, capped at `maxDegree` hops. We then take
// everyone strictly farther than depth 1 (so direct matches are
// excluded) and within `maxDegree`.
std::vector<int> SwipeGraph::getSuggestionsByDegree(int userID, int maxDegree) const {
    std::vector<int> out;
    if (maxDegree < 2) return out;  // depth 1 == direct match, not a suggestion
    auto root = outEdges_.find(userID);
    if (root == outEdges_.end()) return out;

    // Collect direct matches up front so we can exclude them at the
    // end. (BFS will visit them at depth 1, but we want to suggest
    // people you don't already know.)
    std::vector<int> directMatches;
    for (int v : root->second) {
        if (mutual(outEdges_, userID, v)) directMatches.push_back(v);
    }

    // Standard BFS by depth. `distance` doubles as the visited set.
    std::unordered_map<int, int> distance;
    distance[userID] = 0;
    std::queue<int> q;
    q.push(userID);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        int d = distance[u];
        if (d >= maxDegree) continue;  // don't explore past the cap
        auto it = outEdges_.find(u);
        if (it == outEdges_.end()) continue;
        for (int v : it->second) {
            if (!mutual(outEdges_, u, v)) continue;  // only step across mutuals
            if (distance.count(v)) continue;          // already visited
            distance[v] = d + 1;
            q.push(v);
        }
    }

    // Pull out everyone in depth band [2, maxDegree] who isn't already
    // a direct match. Order is whatever the hash map gives us; the
    // caller is fine with that.
    for (auto& [v, d] : distance) {
        if (d >= 2 && d <= maxDegree && v != userID && !contains(directMatches, v)) {
            out.push_back(v);
        }
    }
    return out;
}
