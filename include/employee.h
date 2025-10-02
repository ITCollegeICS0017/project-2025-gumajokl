#pragma once

#include <string>

class Cashier{ 
    private:
        std::string name;
    public:
        Cashier(std::string n);

        void exchange (const Transaction& tx);

        void printReceipt(const Transaction& tx);
};