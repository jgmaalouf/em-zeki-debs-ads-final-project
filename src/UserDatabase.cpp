#include "UserDatabase.h"

#include <algorithm>
#include <fstream>

// Counts how many interests two users share. Plain nested loop -
// interest lists are tiny (a handful of strings each), so this is
// fast enough and there's nothing to allocate.
int countSharedInterests(const User& a, const User& b) {
    int count = 0;
    for (const auto& x : a.interests) {
        for (const auto& y : b.interests) {
            if (x == y) { ++count; break; }
        }
    }
    return count;
}

// emplace returns {iterator, inserted-bool}. We just forward the bool
// so the caller can tell a real insertion from a duplicate-ID no-op.
bool UserDatabase::addUser(const User& u) {
    auto [it, inserted] = users_.emplace(u.userID, u);
    return inserted;
}

const User* UserDatabase::getUser(int userID) const {
    auto it = users_.find(userID);
    return it == users_.end() ? nullptr : &it->second;
}

bool UserDatabase::userExists(int userID) const {
    return users_.find(userID) != users_.end();
}

const std::unordered_map<int, User>& UserDatabase::getAllUsers() const {
    return users_;
}

// Self-blocks would be silly - swallow them. operator[] auto-creates
// the inner list on first block from this user. We dedupe so a
// repeated block is a no-op.
void UserDatabase::block(int by, int target) {
    if (by == target) return;
    auto& list = blocks_[by];
    if (std::find(list.begin(), list.end(), target) == list.end()) {
        list.push_back(target);
    }
}

bool UserDatabase::isBlocked(int by, int target) const {
    auto it = blocks_.find(by);
    if (it == blocks_.end()) return false;
    return std::find(it->second.begin(), it->second.end(), target) != it->second.end();
}

namespace {

// Splits one CSV row into fields. No quote/escape handling - good
// enough for our seed data which has no commas inside fields.
std::vector<std::string> splitCSVLine(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : line) {
        if (c == ',') { out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);  // last field has no trailing comma
    return out;
}

// "music;hiking;coffee" -> {"music","hiking","coffee"}.
// Empty pieces are dropped to tolerate trailing semicolons.
std::vector<std::string> splitInterests(const std::string& field) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : field) {
        if (c == ';') { if (!cur.empty()) out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

}

// Expects: id,name,age,elo,city,interests (one header row, then data).
// Skips rows with fewer than 6 columns and rows whose ID collides with
// an already-loaded user. Returns the count of actual inserts so the
// caller can verify the file was found and parsed.
int UserDatabase::loadFromCSV(const std::string& path) {
    std::ifstream in(path);
    if (!in) return 0;  // file missing -> caller prints a warning

    std::string line;
    bool isHeader = true;
    int loaded = 0;
    while (std::getline(in, line)) {
        if (isHeader) {
            isHeader = false;
            continue;
        }

        auto cols = splitCSVLine(line);
        if (cols.size() < 6)
            continue;  // malformed row, skip silently

        User u;
        u.userID = std::stoi(cols[0]);
        u.name = cols[1];
        u.age = std::stoi(cols[2]);
        u.eloScore = std::stoi(cols[3]);
        u.city = cols[4];
        u.interests = splitInterests(cols[5]);

        if (addUser(u))
            ++loaded;
    }
    return loaded;
}
