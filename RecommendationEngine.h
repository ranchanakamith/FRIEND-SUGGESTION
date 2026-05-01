#pragma once
#include "SocialNetwork.h"
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

// Holds both reasons why someone was suggested
struct ComprehensiveSuggestion {
    string suggestedUserName;
    vector<string> sharedInterests;
    vector<string> mutualFriends;
    
    // Calculates a total match score
    int getScore() const {
        return sharedInterests.size() + mutualFriends.size();
    }
};

class RecommendationEngine {
public:
    static vector<ComprehensiveSuggestion> getSmartSuggestions(SocialNetwork& network, const string& targetName) {
        vector<ComprehensiveSuggestion> suggestions;
        User targetUser = network.getUser(targetName);
        
        if (targetUser.name == "") return suggestions; 

        unordered_set<string> targetFriends = network.getFriends(targetName);

        for (const auto& pair : network.getAllUsers()) {
            string otherName = pair.first;
            User otherUser = pair.second;

            // Skip if it is the target user or if they are already direct friends
            if (otherName == targetName || targetFriends.find(otherName) != targetFriends.end()) {
                continue;
            }

            // 1. Check for Shared Interests
            vector<string> shared;
            for (const string& interest : targetUser.interests) {
                if (otherUser.interests.find(interest) != otherUser.interests.end()) {
                    shared.push_back(interest);
                }
            }

            // 2. Check for Mutual Friends (This finds "Friends of Friends")
            vector<string> mutuals;
            unordered_set<string> otherFriends = network.getFriends(otherName);
            for (const string& myFriend : targetFriends) {
                if (otherFriends.find(myFriend) != otherFriends.end()) {
                    mutuals.push_back(myFriend);
                }
            }

            // 3. If they are a friend-of-a-friend OR share an interest, suggest them!
            if (!shared.empty() || !mutuals.empty()) {
                suggestions.push_back({otherName, shared, mutuals});
            }
        }

        // Sort the suggestions by highest score first
        sort(suggestions.begin(), suggestions.end(), [](const ComprehensiveSuggestion& a, const ComprehensiveSuggestion& b) {
            return a.getScore() > b.getScore();
        });

        return suggestions;
    }
};