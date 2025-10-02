#include "employee.h"
using namespace std;

Employee::Employee(const string &name, int id):
    name_(name), 
    id_(id) {}

string Employee::getName() const { return name_; }
// int Employee::getId() const { return id_; }
