#include "data_storage.h"



int DataStore::addPerson(const std::string& roleStr, const std::string& name) {
    PersonRole role = PersonRole::UNKNOWN;
    if (roleStr == "cashier") {
        role = PersonRole::CASHIER;
    } else if (roleStr == "manager") {
        role = PersonRole::MANAGER;
    } else if (roleStr == "client") {
        role = PersonRole::CLIENT;
    }

    Person newPerson{nextPersonId++, name, role};
    peopleDB[std::make_tuple(roleStr, name)] = newPerson;
    return newPerson.id;
}

int DataStore::getPersonId(const std::string& roleStr, const std::string& name) {
    std::tuple<std::string, std::string> key = std::make_tuple(roleStr, name);
    auto it = peopleDB.find(key);
    if (it != peopleDB.end()) {
        return it->second.id;
    }
    return addPerson(roleStr, name);
}

