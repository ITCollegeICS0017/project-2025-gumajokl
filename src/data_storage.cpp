#include "data_storage.h"



int DataStore::addPerson(PersonRole role, const std::string& name) {
    Person newPerson{nextPersonId++, name, role};
    peopleDB[std::make_tuple(role, name)] = newPerson;
    return newPerson.id;
}

int DataStore::getPersonId(PersonRole role, const std::string& name) {
    std::tuple<PersonRole, std::string> key = std::make_tuple(role, name);
    auto it = peopleDB.find(key);
    if (it != peopleDB.end()) {
        return it->second.id;
    }
    return addPerson(role, name);
}

