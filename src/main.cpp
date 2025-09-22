#include <stdio.h>
#include <string>
#include <iostream>
using namespace std; // Using the standard namespace for simplicity

// Currency types that we allow
enum class Currency { 
    USD,
    EUR,
    GBP
};

// A struct for a currency exchange transaction
struct Transaction { 
    double amount;
    Currency from;
    Currency to;
};

// Cashier class that has an encapsulated name and an exchange method
class Cashier{ 
    private:
        // Constructor that initializes the name of the cashier when created
        string name;
    public:
        Cashier(string n) : name(n){}
            void exchange (const Transaction& tx) { // Method to perform currency exchange (Uses tx as input and calls cout)
                cout << name << " exchanges " << tx.amount << " from " << (int)tx.from << " to " << (int)tx.to << endl;
            }
};

int main() {
    Cashier cashier("Gunther");
    Transaction tx{100.0, Currency::USD, Currency::EUR};
    cashier.exchange(tx);
}
