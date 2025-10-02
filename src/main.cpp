#include <stdio.h>
#include <string>
#include <ctime>
// using namespace std; // Using the standard namespace for simplicity


#include "employee.h"
#include "utils.h"


int main() {
    Cashier cashier("Gunther"); // Create a Cashier object named "Gunther"
    Transaction tx{100.0, Currency::USD, Currency::EUR}; // Create a Transaction object to exchange 100 USD to EUR
    cashier.exchange(tx); // Call the exchange method of the Cashier object with the Transaction object
    cashier.printReceipt(tx); // Call the printReceipt method of the Cashier object with the Transaction object
}
