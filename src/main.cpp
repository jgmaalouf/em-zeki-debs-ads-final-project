// CLI entry point. Two states: signed-out (auth menu) and signed-in
// (session menu). The big while-loop just bounces between them based
// on whether `session` is null.

#include <cctype>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "CityGraph.h"
#include "Session.h"
#include "SwipeDeck.h"
#include "SwipeGraph.h"
#include "User.h"
#include "UserDatabase.h"

// ---------- tiny I/O helpers ----------

// Read an int from stdin and don't move on until the user gives us one.
// Reads a whole line and parses it strictly: trailing junk like "1o3" is
// rejected, not silently truncated to 1. operator>> is a *prefix* parser
// and would happily accept "1o3" as 1, leaving "o3" in the buffer.
int readInt(const std::string& prompt) {
    std::cout << prompt;
    while (true) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "  invalid integer, try again: ";
            std::cin.clear();
            continue;
        }
        try {
            size_t consumed = 0;
            int x = std::stoi(line, &consumed);
            // Allow trailing whitespace, but reject any other trailing chars.
            while (consumed < line.size() && std::isspace(static_cast<unsigned char>(line[consumed]))) {
                ++consumed;
            }
            if (consumed != line.size()) throw std::invalid_argument("trailing garbage");
            return x;
        } catch (...) {
            std::cout << "  invalid integer, try again: ";
        }
    }
}

// Read an int that must fall in [lo, hi]. Loops on out-of-range input the
// same way readInt loops on garbage. Domain validation lives here (and at
// the callsite for cross-field rules), not in the parser.
int readIntInRange(const std::string& prompt, int lo, int hi) {
    while (true) {
        int x = readInt(prompt);
        if (x >= lo && x <= hi) return x;
        std::cout << "  must be between " << lo << " and " << hi << "\n";
    }
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// "music;hiking;coffee" -> {"music","hiking","coffee"}.
// Empty pieces are dropped so a trailing ';' doesn't add a blank tag.
std::vector<std::string> splitOnSemicolon(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ';') {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// ---------- menus & display ----------

void printAuthMenu() {
    std::cout << "\n--- Account ---\n"
              << "1. Sign in (by user ID)\n"
              << "2. Create account\n"
              << "0. Quit\n"
              << "Choose: ";
}

void printSessionMenu(const User& me) {
    std::cout << "\n--- Signed in as " << me.name << " (#" << me.userID << ") ---\n"
              << "1. Set filters (age range, max distance)\n"
              << "2. Load deck\n"
              << "3. View next profile\n"
              << "4. Swipe right\n"
              << "5. Swipe left\n"
              << "6. Rewind last swipe\n"
              << "7. View matches\n"
              << "8. View friend-of-friend suggestions (BFS, depth 2)\n"
              << "9. Block a user\n"
              << "10. Sign out\n"
              << "0. Quit\n"
              << "Choose: ";
}

// One line per profile - id, name, age, elo, city, interest tags.
void printUser(const User& u) {
    std::cout << "  [" << u.userID << "] " << u.name
              << " | age " << u.age
              << " | Elo " << u.eloScore
              << " | " << u.city
              << " | interests:";
    for (const auto& i : u.interests) std::cout << " " << i;
    std::cout << '\n';
}

// ---------- account helpers ----------

// Pick a free user ID by walking the existing IDs and adding 1 to the max.
// Tiny dataset, so this is fine - no need for a counter or UUIDs.
int nextAvailableID(const UserDatabase& db) {
    int max = 0;
    for (const auto& [id, _] : db.getAllUsers()) {
        if (id > max) max = id;
    }
    return max + 1;
}

// Build a User from stdin. Defaults kick in if the user just hits Enter.
// New accounts always start at 1000 Elo so the score makes sense relative
// to the seeded users.
User promptForNewUser(int newID) {
    User u;
    u.userID = newID;
    u.name = readLine("  name: ");
    if (u.name.empty()) u.name = "Anonymous";
    u.age = readInt("  age: ");
    u.eloScore = 1000;
    u.city = readLine("  city: ");
    if (u.city.empty()) u.city = "NewYork";
    std::string interests = readLine("  interests (semicolon-separated, e.g. 'music;hiking;coffee'): ");
    u.interests = splitOnSemicolon(interests);
    return u;
}

// Build the Session, immediately load the deck under current filters, and
// print the welcome lines. Used by both "sign in" and "create account"
// since they end in the same place.
std::unique_ptr<Session> signInAs(int id,
                                  UserDatabase& db,
                                  SwipeGraph& swipeGraph,
                                  CityGraph& cityGraph,
                                  const Filters& filters) {
    auto session = std::make_unique<Session>(id, db, swipeGraph, cityGraph);
    std::cout << "  signed in as " << db.getUser(id)->name << '\n';
    int n = session->loadDeck(filters);
    std::cout << "  deck auto-loaded with " << n << " profiles\n";
    return session;
}

// ---------- menu handlers ----------

// One iteration of the signed-out menu. Returns the new session if the
// user signed in or just created one, or nullptr if they're still
// signed out (or chose to quit, in which case `quit` is set).
std::unique_ptr<Session> handleAuth(UserDatabase& db,
                                    SwipeGraph& swipeGraph,
                                    CityGraph& cityGraph,
                                    const Filters& filters,
                                    bool& quit) {
    printAuthMenu();
    int choice = readInt("");
    switch (choice) {
        case 0:
            quit = true;
            return nullptr;

        case 1: {
            int id = readInt("  user ID: ");
            if (!db.userExists(id)) {
                std::cout << "  no such user\n";
                return nullptr;
            }
            return signInAs(id, db, swipeGraph, cityGraph, filters);
        }

        case 2: {
            int newID = nextAvailableID(db);
            std::cout << "  creating user #" << newID << '\n';
            User u = promptForNewUser(newID);
            // addUser fails on ID collision. Shouldn't happen since we
            // just picked nextAvailableID(), but better safe than mute.
            if (!db.addUser(u)) {
                std::cout << "  failed to create user (ID collision)\n";
                return nullptr;
            }
            return signInAs(newID, db, swipeGraph, cityGraph, filters);
        }

        default:
            std::cout << "  unknown choice\n";
            return nullptr;
    }
}

// One iteration of the signed-in menu. Returns false when the user signs
// out (or quits, with `quit` set) so the outer loop can switch states.
bool handleSession(Session& session,
                   UserDatabase& db,
                   Filters& filters,
                   bool& quit) {
    const User* me = db.getUser(session.userID());
    printSessionMenu(*me);
    int choice = readInt("");
    switch (choice) {
        case 0:
            quit = true;
            return false;

        case 1: {
            // Filters are kept per-process, not per-session, so they
            // persist if you sign out and back in. Could change that.
            //
            // Bounds: ages clamped to [18, 99] (legal-adult floor and a
            // sensible ceiling); distance is non-negative and capped at
            // the default "unlimited" sentinel. maxAge must be >= minAge,
            // otherwise the deck would always be empty.
            int minAge = readIntInRange("  min age (18-99): ", 18, 99);
            int maxAge;
            while (true) {
                maxAge = readIntInRange("  max age (18-99): ", 18, 99);
                if (maxAge >= minAge) break;
                std::cout << "  max age must be >= min age (" << minAge << ")\n";
            }
            int maxDist = readIntInRange("  max distance (km, 0-100000): ", 0, 100000);
            filters.minAge = minAge;
            filters.maxAge = maxAge;
            filters.maxDistanceKm = maxDist;
            return true;
        }

        case 2: {
            int n = session.loadDeck(filters);
            std::cout << "  deck loaded with " << n << " profiles\n";
            return true;
        }

        case 3: {
            // Peek doesn't consume - lets you see who's next without
            // committing to a swipe.
            int next = session.deck().peekNext();
            if (next < 0) std::cout << "  deck is empty\n";
            else printUser(*db.getUser(next));
            return true;
        }

        case 4: {
            auto out = session.deck().swipeRight(session.swipeGraph());
            if (out.swipedID < 0) {
                std::cout << "  deck is empty\n";
                return true;
            }
            session.markSeen(out.swipedID);
            std::cout << "  swiped right on user " << out.swipedID
                      << (out.result == SwipeResult::MATCH ? "  -> MATCH!" : "")
                      << '\n';
            return true;
        }

        case 5: {
            auto out = session.deck().swipeLeft();
            if (out.swipedID < 0) {
                std::cout << "  deck is empty\n";
                return true;
            }
            session.markSeen(out.swipedID);
            std::cout << "  swiped left on user " << out.swipedID << '\n';
            return true;
        }

        case 6: {
            // Rewind: undo the last swipe. If it was a right-swipe, we
            // also yank the edge from the swipe graph.
            int restored = session.deck().rewind(session.swipeGraph());
            if (restored < 0) std::cout << "  no history to rewind\n";
            else std::cout << "  rewound user " << restored << " back into deck\n";
            return true;
        }

        case 7: {
            auto matches = session.swipeGraph().getMatches(session.userID());
            if (matches.empty()) std::cout << "  no matches yet\n";
            for (int m : matches) printUser(*db.getUser(m));
            return true;
        }

        case 8: {
            // Friends-of-friends - BFS up to depth 2 over mutual edges.
            auto sug = session.swipeGraph().getSuggestionsByDegree(session.userID(), 2);
            if (sug.empty()) std::cout << "  no FoF suggestions\n";
            for (int s : sug) printUser(*db.getUser(s));
            return true;
        }

        case 9: {
            int target = readInt("  block user ID: ");
            db.block(session.userID(), target);
            std::cout << "  blocked\n";
            return true;
        }

        case 10:
            std::cout << "  signed out of " << me->name << '\n';
            return false;

        default:
            std::cout << "  unknown choice\n";
            return true;
    }
}

// ---------- main ----------

int main() {
    // Long-lived state. The graphs and DB outlive any individual session
    // so matches and blocks survive a sign-out / sign-in cycle.
    UserDatabase db;
    SwipeGraph swipeGraph;
    CityGraph cityGraph;

    int loaded = db.loadFromCSV("data/users.csv");
    if (loaded == 0)
        std::cout << "WARNING: could not load data/users.csv\n";
    else
        std::cout << "Loaded " << loaded << " users from data/users.csv\n";

    std::unique_ptr<Session> session;  // null = signed out
    Filters filters;
    bool quit = false;

    // Bounces between the two menus. signed-out branches can produce a
    // session; the signed-in branch can clear it (sign out) or set quit.
    while (!quit) {
        if (!session) {
            session = handleAuth(db, swipeGraph, cityGraph, filters, quit);
        } else if (!handleSession(*session, db, filters, quit)) {
            session.reset();
        }
    }

    std::cout << "bye\n";
    return 0;
}
