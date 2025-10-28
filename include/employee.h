#pragma once

#include "utils.h"
#include "exchange_manager.h"

#include <string>


class Employee {
    protected:
        int id;
        std::string name;
        PersonRole role;

        ExchangeOffice exchangeOffice;

    public:
        Employee(int employeeId, std::string employeeName, ExchangeOffice& exchangeOffice_in);

        int get_id() const { return id; }
        const std::string& get_name() const { return name; }
};


class Cashier : public Employee {
    public:
        Cashier(int cashierId, std::string cashierName);

        Receipt perform_exchange(const ExchangeRequest& request);
};

class Manager : public Employee {
    public:
        Manager(int managerId, std::string managerName);
};