#include <iostream>
#include <string>
#include <limits>
#include "SocialNetwork.h"
#include "RecommendationEngine.h"

using namespace std;

// --- NEW AUTOMATED TEST FUNCTION ---
void runAutomatedTest(SocialNetwork& network) {
    cout << "\n=====================================\n";
    cout << "   RUNNING AUTOMATED SYSTEM TEST     \n";
    cout << "=====================================\n";
    
    // 1. Create 3 users with totally DIFFERENT interests
    network.addUser("Alice", {"coding"});
    network.addUser("Bob", {"sports"});
    network.addUser("David", {"gaming"});

    // 2. Make Alice friends with Bob
    network.addFriendship("Alice", "Bob");
    
    // 3. Make Bob friends with David
    network.addFriendship("Bob", "David");

    cout << "Setup Complete: Alice is friends with Bob. Bob is friends with David.\n";
    cout << "Therefore, David is a 'Friend of a Friend' to Alice.\n\n";

    // 4. Force suggestion for Alice
    cout << "Checking Suggestions for 'Alice'...\n";
    vector<ComprehensiveSuggestion> suggestions = RecommendationEngine::getSmartSuggestions(network, "Alice");

    if (suggestions.empty()) {
        cout << "FAILED: No suggestions found.\n";
    } else {
        for (const auto& match : suggestions) {
            cout << "SUCCESS! Suggested -> " << match.suggestedUserName << " (Match Score: " << match.getScore() << ")\n";
            if (!match.mutualFriends.empty()) {
                cout << "    [Because you both know: ";
                for (size_t i = 0; i < match.mutualFriends.size(); ++i) {
                    cout << match.mutualFriends[i];
                    if (i < match.mutualFriends.size() - 1) cout << ", ";
                }
                cout << "]\n";
            }
        }
    }
    cout << "=====================================\n\n";
}

int main() {
    SocialNetwork network;
    int choice = 0;

    // RUN THE TEST IMMEDIATELY WHEN THE APP STARTS
    runAutomatedTest(network);

    cout << "--- Welcome to the Social Network Builder ---\n";

    while (true) {
        cout << "\nMenu Options:\n";
        cout << "1. Add a New User\n";
        cout << "2. Make Two Users Friends\n";
        cout << "3. Get Smart Friend Suggestions (Interests & Mutual Friends)\n";
        cout << "4. Exit\n";
        cout << "Enter your choice: ";
        
        string input;
        getline(cin, input);

        try {
            size_t pos;
            choice = stoi(input, &pos); 
            for (size_t i = pos; i < input.length(); ++i) {
                if (!isspace(input[i])) throw invalid_argument("Extra characters");
            }
        } catch (...) { 
            cout << ">> INVALID INPUT: Please enter exactly a number from the menu.\n";
            continue; 
        }

        if (choice == 1) {
            string name, interest;
            unordered_set<string> interests;

            cout << "Enter User Name: ";
            getline(cin, name);

            if (network.getUser(name).name != "") {
                cout << ">> ALERT: A user named '" << name << "' is already in the system! Please use a different name.\n";
                continue; 
            }

            cout << "Enter interests one by one (type 'done' when finished):\n";
            while (true) {
                cout << " > ";
                getline(cin, interest);
                
                for(auto& c : interest) c = tolower(c); 

                if (interest == "done") break;
                if (!interest.empty()) interests.insert(interest);
            }

            bool success = network.addUser(name, interests);
            if (success) {
                cout << "User '" << name << "' added successfully!\n";
            }

        } else if (choice == 2) {
            string name1, name2;
            cout << "Enter first User's Name: ";
            getline(cin, name1);
            cout << "Enter second User's Name: ";
            getline(cin, name2);
            
            network.addFriendship(name1, name2);
            cout << "Action complete (if both users exist, they are now friends!).\n";

        } else if (choice == 3) {
            string targetName;
            cout << "Enter User's Name to get suggestions for: ";
            getline(cin, targetName);

            User u = network.getUser(targetName);
            if(u.name == "") {
                cout << "User '" << targetName << "' not found in the system.\n";
                continue;
            }

            cout << "\nFinding smart matches for " << u.name << "...\n";
            
            vector<ComprehensiveSuggestion> suggestions = RecommendationEngine::getSmartSuggestions(network, targetName);

            if (suggestions.empty()) {
                cout << "No suggestions found.\n";
            } else {
                for (const auto& match : suggestions) {
                    cout << " -> " << match.suggestedUserName << " (Match Score: " << match.getScore() << ")\n";
                    
                    if (!match.mutualFriends.empty()) {
                        cout << "    [Because you both know: ";
                        for (size_t i = 0; i < match.mutualFriends.size(); ++i) {
                            cout << match.mutualFriends[i];
                            if (i < match.mutualFriends.size() - 1) cout << ", ";
                        }
                        cout << "]\n";
                    }

                    if (!match.sharedInterests.empty()) {
                        cout << "    [Because you both like: ";
                        for (size_t i = 0; i < match.sharedInterests.size(); ++i) {
                            cout << match.sharedInterests[i];
                            if (i < match.sharedInterests.size() - 1) cout << ", ";
                        }
                        cout << "]\n";
                    }
                    cout << "\n";
                }
            }
        } else if (choice == 4) {
            cout << "Exiting program. Goodbye!\n";
            break;
        } else {
            cout << "Invalid choice. Please choose a number between 1 and 4.\n";
        }
    }

    return 0;
}