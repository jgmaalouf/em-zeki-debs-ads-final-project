#ifndef ADS_USER_DATABASE_H
#define ADS_USER_DATABASE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "User.h"

// Free function - lives here because shared interests are a property of
// two users, not of any one user. Used by the compatibility score.
int countSharedInterests(const User& a, const User& b);

// In-memory store of every user, plus the per-user block list.
// Backing store is a hash map keyed by userID. Lookups are O(1) on
// average, which matters because the deck loader walks all users.
class UserDatabase {
public:
    // Returns false on duplicate IDs - the caller is expected to treat
    // that as a user error, not a crash.
    bool addUser(const User& u);

    // Pointer instead of reference so we can return null for "no such
    // user" without exceptions. Caller must null-check.
    const User* getUser(int userID) const;
    bool userExists(int userID) const;

    // Whole-map access for the deck loader, which needs to iterate
    // every user. Const-ref so callers can't mutate.
    const std::unordered_map<int, User>& getAllUsers() const;

    // Blocking is one-directional in storage but the deck filter
    // checks both directions, so a block is effectively mutual.
    void block(int by, int target);
    bool isBlocked(int by, int target) const;

    // Loads users.csv. Returns the number of rows actually inserted
    // (header is skipped, malformed rows and ID collisions are
    // dropped silently).
    int loadFromCSV(const std::string& path);

private:
    std::unordered_map<int, User> users_;
    // by-id -> list of blocked-id. We insert-if-absent so repeated
    // blocks stay idempotent.
    std::unordered_map<int, std::vector<int>> blocks_;
};

#endif
