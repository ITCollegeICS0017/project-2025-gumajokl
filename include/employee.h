#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <string>

class Employee {
public:
    explicit Employee(const std::string &name);

    std::string getName() const;

private:
    std::string name_;
};

#endif
