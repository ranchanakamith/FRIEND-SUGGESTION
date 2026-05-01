#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace std; 

struct User {
    string name;
    unordered_set<string> interests;

    User() : name("") {}
    
    User(string userName, unordered_set<string> userInterests) 
        : name(userName), interests(userInterests) {}
};

class SocialNetwork {
private:
    unordered_map<string, User> users;
    unordered_map<string, unordered_set<string>> adjacencyList;

public:
    bool addUser(const string& name, const unordered_set<string>& interests) {
        if (users.find(name) != users.end()) {
            return false; 
        }
        users[name] = User(name, interests);
        adjacencyList[name] = unordered_set<string>(); 
        return true; 
    }

    void addFriendship(const string& name1, const string& name2) {
        if (users.find(name1) != users.end() && users.find(name2) != users.end()) {
            adjacencyList[name1].insert(name2);
            adjacencyList[name2].insert(name1); 
        } else {
            cerr << "Error: One or both user names do not exist in the system.\n";
        }
    }

    User getUser(const string& name) {
        if (users.find(name) != users.end()) return users[name];
        return User("", {}); 
    }

    unordered_set<string> getFriends(const string& name) {
        if (adjacencyList.find(name) != adjacencyList.end()) return adjacencyList[name];
        return unordered_set<string>(); 
    }
    
    const unordered_map<string, User>& getAllUsers() const {
        return users;
    }
};