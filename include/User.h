#ifndef ADS_USER_H
#define ADS_USER_H

#include <string>
#include <vector>

// A single user record. Plain data - no behaviour. We pass these around
// by const-ref everywhere; nothing here owns or mutates a User except
// the UserDatabase that holds them.
//
// `eloScore` is a soft "desirability" rating used by the compatibility
// score - new users start at 1000, like in chess.
struct User {
    int userID;
    std::string name;
    int age;
    int eloScore;
    std::string city;
    std::vector<std::string> interests;
};

#endif
