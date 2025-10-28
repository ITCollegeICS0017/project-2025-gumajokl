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
        std::map<std::tuple<PersonRole, std::string>, Person> peopleDB;
        int nextPersonId = 1;

        int addPerson(PersonRole role, const std::string& name);

    public:
        DataStore();
        int getPersonId(PersonRole role, const std::string& name);

};