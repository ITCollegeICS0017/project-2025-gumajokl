#include "employee.h"

#include <iostream>

Cashier::Cashier(const std::string& n) : name(n){}

void Cashier::exchange(const Transaction& tx) const {
    std::cout << name << " exchanges " << tx.amount << " from " << (int)tx.from << " to " << (int)tx.to << std::endl;
}

void Cashier::printReceipt(const Transaction& tx) const {
    std::cout << "Receipt: " << name << " exchanged " 
            << tx.amount << " from " 
            << (int)tx.from << " to " 
            << (int)tx.to << std::endl;
}


Manager::Manager(const std::string& name) : managerName(name) {}