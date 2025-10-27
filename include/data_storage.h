#pragma once

#include "utils.h"

#include <map>
#include <tuple>
#include <string>

struct Person {
    int id;
    std::string name;
    PersonRole role;
};

class DataStore{
    private:
        std::map<std::tuple<std::string, std::string>, Person> peopleDB;
        int nextPersonId = 1;

        int addPerson(const std::string& roleStr, const std::string& name);

    public:
        int getPersonId(const std::string& roleStr, const std::string& name);

};