#include "employee.h"
using namespace std;

Employee::Employee(const string &name):
    name_(name) {}

string Employee::getName() const { return name_; }
// int Employee::getId() const { return id_; }
