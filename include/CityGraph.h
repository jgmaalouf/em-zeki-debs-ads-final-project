#ifndef ADS_CITY_GRAPH_H
#define ADS_CITY_GRAPH_H

#include <climits>
#include <string>
#include <unordered_map>
#include <vector>

// Cities + the road distances between them. Used to compute how far
// two users live apart, which feeds into both the deck filter (max
// distance) and the compatibility score.
//
// Internally an undirected weighted graph. City names are interned to
// small integer IDs so the BFS / Dijkstra inner loop touches ints
// instead of strings. Edges are seeded once in the constructor - we
// don't reload them at runtime.
class CityGraph {
public:
    // Sentinel for "no path between these cities" or "unknown city".
    // INT_MAX so callers can compare with > maxDistance and have it
    // naturally fail the filter.
    static constexpr int kUnreachable = INT_MAX;

    CityGraph();

    // Adds an undirected edge with the given length in km.
    // Both endpoints are interned on the fly if they're new.
    void addEdge(const std::string& a, const std::string& b, int weightKm);

    // Shortest-path distance in km. Returns kUnreachable if either
    // city is unknown or there is no path between them. a == b is 0.
    int getDistance(const std::string& a, const std::string& b) const;

private:
    // Look up a city's int ID, creating one if it doesn't exist yet.
    int internCity(const std::string& name);

    std::unordered_map<std::string, int> idOf_;            // name -> id
    std::vector<std::string> nameOf_;                      // id -> name (debug)
    std::vector<std::vector<std::pair<int, int>>> adj_;    // id -> [(neighbour, km)]
};

#endif
