#include <string>
#include <iostream>

#include "employee.h"
#include "utils.h"




// Cashier class that has an encapsulated name and an exchange method
class Cashier{ 
    private:
        // Constructor that initializes the name of the cashier when created
        std::string name;
    public:
        Cashier(std::string n) : name(n){}

        void exchange (const Transaction& tx) { // Method to perform currency exchange (Uses tx as input and calls cout)
            std::cout << name << " exchanges " 
            << tx.amount << " from " 
            << (int)tx.from << " to " 
            << (int)tx.to << std::endl;
        }

        void printReceipt(const Transaction& tx) { // Method to print a receipt of the transaction (Uses tx as input and calls cout)
            std::cout << "Receipt: " << name << " exchanged " 
            << tx.amount << " from " 
            << (int)tx.from << " to " 
            << (int)tx.to << std::endl;
        }
};

class Manager { // Implementation of Manager class
    private:
        std::string managerName;
    public:
        Manager(std::string name) : managerName(name) {} // Constructor to initialize manager name
};