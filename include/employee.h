#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <string>

class Employee {
public:
    Employee(const std::string &name, int id);

    std::string getName() const;

private:
    std::string name_;
    int id_;
};

#endif
