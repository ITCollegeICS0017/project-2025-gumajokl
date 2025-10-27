#pragma once

#include <string>

class Employee {
    protected:
        int id;
        std::string name;


    public:
        Employee(int employeeId, std::string employeeName);

        int get_id() const { return id; }
        const std::string& get_name() const { return name; }
};


class Cashier : public Employee {
    private:

    public:
        Cashier(int cashierId, std::string cashierName);
};

class Manager : public Employee {
    private:

    public:
        Manager(int managerId, std::string managerName);
};