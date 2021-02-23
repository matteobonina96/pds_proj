//
// Created by matteo on 11/02/21.
//

#ifndef PROJE_USERS_H
#define PROJE_USERS_H
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>

class users {
std::string username;
std::string password;
std::string user_path;
public:
    users(){};
    users(std::string username,std::string password,std::string user_path):
            user_path(user_path),
            username(username),
            password(password){};


    void setUsername(std::string u) {
        this->username=u;
    }
    void setPassword(std::string p) {
        this->password=p;
    }
    void setUserPath(std::string up) {
        this->user_path=up;
    }
    std::string getUsername() {
        return this->username;
    }
    std::string getPassword() {
        return this->password;
    }
    std::string getUserPath() {
        return this->user_path;
    }
};


#endif //PROJE_USERS_H
