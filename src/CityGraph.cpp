#include "CityGraph.h"

#include <queue>
#include <utility>
#include <vector>

// Seed graph. Lebanese cities + rough km between them - enough to make
// the distance filter and Dijkstra search do something interesting on
// the demo data. To add cities, just append rows here.
CityGraph::CityGraph() {
    struct Edge { const char* a; const char* b; int km; };
    const Edge edges[] = {
        {"Beirut",   "Jounieh",  20},
        {"Jounieh",  "Byblos",   20},
        {"Byblos",   "Batroun",  15},
        {"Batroun",  "Tripoli",  30},
        {"Tripoli",  "Mina",     5},
        {"Beirut",   "Sidon",    40},
        {"Sidon",    "Tyre",     40},
        {"Sidon",    "Nabatieh", 30},
        {"Beirut",   "Aley",     20},
        {"Aley",     "Zahle",    30},
        {"Beirut",   "Zahle",    50},
        {"Zahle",    "Baalbek",  40},
        {"Beirut",   "Tripoli",  80},  // coastal shortcut, parallel to Jounieh-Byblos-...
        {"Tripoli",  "Akkar",    45},
        {"Tyre",     "Nabatieh", 35},
        {"Zahle",    "Hasbaya",  60},
    };
    for (const auto& e : edges) addEdge(e.a, e.b, e.km);
}

// Get or assign an int ID for this city name. The id is just the
// position in nameOf_, which keeps things compact and lets us index
// adj_ by id directly.
int CityGraph::internCity(const std::string& name) {
    auto it = idOf_.find(name);
    if (it != idOf_.end()) return it->second;
    int id = static_cast<int>(nameOf_.size());
    idOf_[name] = id;
    nameOf_.push_back(name);
    adj_.emplace_back();  // empty adjacency list for the new node
    return id;
}

// Push the edge into both directions - graph is undirected.
void CityGraph::addEdge(const std::string& a, const std::string& b, int weightKm) {
    int u = internCity(a);
    int v = internCity(b);
    adj_[u].emplace_back(v, weightKm);
    adj_[v].emplace_back(u, weightKm);
}

// Plain Dijkstra with a min-heap. Early-exit as soon as we pop the
// destination - we don't care about distances to other cities.
//
// The `if (d > dist[u]) continue;` line skips stale heap entries that
// were superseded by a shorter path before we got to them. Standard
// trick to avoid having to support decrease-key in std::priority_queue.
int CityGraph::getDistance(const std::string& a, const std::string& b) const {
    if (a == b) return 0;
    auto ita = idOf_.find(a);
    auto itb = idOf_.find(b);
    if (ita == idOf_.end() || itb == idOf_.end()) return kUnreachable;
    int src = ita->second;
    int dst = itb->second;

    const int n = static_cast<int>(adj_.size());
    std::vector<int> dist(n, kUnreachable);
    dist[src] = 0;

    using QItem = std::pair<int, int>;  // (distance-so-far, node-id)
    std::priority_queue<QItem, std::vector<QItem>, std::greater<QItem>> pq;
    pq.emplace(0, src);

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (u == dst) return d;        // first time we pop dst, d is optimal
        if (d > dist[u]) continue;     // outdated heap entry, ignore
        for (auto [v, w] : adj_[u]) {
            int nd = d + w;
            if (nd < dist[v]) {
                dist[v] = nd;
                pq.emplace(nd, v);
            }
        }
    }
    return kUnreachable;  // disconnected component
}
