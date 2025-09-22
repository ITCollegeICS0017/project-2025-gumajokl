#include <stdio.h>
#include <string>
#include <iostream>
#include <ctime>
#include <chrono>
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
                cout << name << " exchanges " 
                << tx.amount << " from " 
                << (int)tx.from << " to " 
                << (int)tx.to << endl;
            }
};

// Implementation of Client class
class Client {
    private:
        string clientName;
    public:
        Client(string name) : clientName(name) {} // Constructor to initialize client name

};

// Implementation of Receipt class
class Receipt {
    private:
        int transactionID;
    public:
        Receipt(int number) : transactionID(number) {}
        float exchangeRate; // Exchange rate for the transaction
        float CurrencyFrom;
        float CurrencyTo;
        float amountExchanged;
        Currency FromCurrency;
        Currency ToCurrency;
        time_t transactionTime; // Time of the transaction
};

class Reserve { // Implementation of Reserve class
    private: // Private members to hold total amounts of each currency
        float totalUSD;
        float totalEUR;
        float totalGBP;
    public: // Constructor to initialize the reserve amounts
        Reserve(float usd, float eur, float gbp) 
        : totalUSD(usd), totalEUR(eur), totalGBP(gbp) {}
};

int main() {
    Cashier cashier("Gunther"); // Create a Cashier object named "Gunther"
    Transaction tx{100.0, Currency::USD, Currency::EUR}; // Create a Transaction object to exchange 100 USD to EUR
    cashier.exchange(tx); // Call the exchange method of the Cashier object with the Transaction object
}
