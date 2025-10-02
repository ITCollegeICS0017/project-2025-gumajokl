#pragma once

#include <string>
#include "utils.h"

class Cashier{ 
    private:
        std::string name;
    public:
        Cashier(const std::string& n);

        void exchange (const Transaction& tx) const;
        void printReceipt(const Transaction& tx) const;
};

class Manager { // Implementation of Manager class
    private:
        std::string managerName;
    public:
        Manager(const std::string& name);
};