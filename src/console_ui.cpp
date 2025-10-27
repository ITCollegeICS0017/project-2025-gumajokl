#include "console_ui.h"

#include "data_storage.h"
#include "employee.h"
#include "client.h"

#include <iostream>


ConsoleUI::ConsoleUI(DataStore& dataStore_in)
    : dataStore(dataStore_in) {}

// ### Util functions

std::string ConsoleUI::readLine(std::string prompt) const {
    std::string input;
    std::cout << prompt;
    std::cout.flush();
    if (!std::getline(std::cin, input)) {
        throw std::runtime_error("Input stream closed unexpectedly");
    }
    return input;
}

int ConsoleUI::readInt(const std::string& prompt, int minValue, int maxValue) const {
    while (true) {
        std::string text = readLine(prompt);
        try {
            int value = std::stoi(text);
            if (value < minValue || value > maxValue) {
                std::cout << "Please enter a number between " << minValue << " and " << maxValue << ".\n";
                continue;
            }
            return value;
        } catch (...) {
            std::cout << "Invalid number. Try again.\n";
        }
    }
}

double ConsoleUI::readDouble(const std::string& prompt, double minValue) const {
    while (true) {
        std::string text = readLine(prompt);
        try {
            double value = std::stod(text);
            if (value < minValue) {
                std::cout << "Value must be at least " << minValue << ".\n";
                continue;
            }
            return value;
        } catch (...) {
            std::cout << "Invalid number. Try again.\n";
        }
    }
}

Currency ConsoleUI::readCurrency(const std::string& prompt) const {
    while (true) {
        std::string text = readLine(prompt);
        if (currency_from_string(text) == Currency::UNKNOWN) {
            std::cout << "Unrecognized currency. Please try again.\n";
            continue;
        }
        return currency_from_string(text);
    }
}

bool ConsoleUI::readYesNo(const std::string& prompt) const {
    while (true) {
        std::string text = readLine(prompt);
        if (text == "y" || text == "yes") {
            return true;
        }
        if (text == "n" || text == "no") {
            return false;
        }
        std::cout << "Please enter 'y' or 'n'.\n";
    }
}


// ### Different screens

void ConsoleUI::mainMenu() {
    bool running = true;
    while (running && std::cin) {
        std::cout << "\n=== Currency Exchange System ===\n";
        std::cout << "1. Employee login\n";
        std::cout << "2. Manager login\n";
        std::cout << "3. Quit\n";

        int choice = readInt("Select option: ", 1, 3);

        switch (choice) {
            case 1:
                try {
                    cashierMenu();
                } catch (const std::exception& error) {
                    std::cout << "Cashier session ended: " << error.what() << '\n';
                }
                break;
            case 2:
                try {
                    // managerMenu();
                } catch (const std::exception& error) {
                    std::cout << "Manager session ended: " << error.what() << '\n';
                }
                break;
            case 3:
                running = false;
                break;
        }
    }

    std::cout << "Program ended\n";
}

void ConsoleUI::cashierMenu() {
    std::string cashierName = readLine("Enter cashier name: ");
    int cashierId = dataStore.getPersonId("cashier", cashierName);
    Cashier cashier(cashierId, cashierName);

    bool active = true;
    while (active) {
        std::cout << "\n-- Cashier Menu --\n";
        std::cout << "1. Perform exchange\n";
        std::cout << "2. Check reserve balances\n";
        std::cout << "3. Logout\n";
        int choice = readInt("Select option: ", 1, 3);

        switch (choice) {
            case 1:
                cashierExchange(cashier);
                break;
            case 2:
                ReserveCheck();
                break;
            case 3:
                active = false;
                break;
        }
    }
}

void ConsoleUI::cashierExchange(Cashier &cashier){
    // Get specific client
    // If does not exist, create new client
    std::string clientName = readLine("Enter client name: ");
    int clientId = dataStore.getPersonId("client", clientName);
    Client client(clientId, clientName);

    // Set exchange details
    Currency fromCurrency = readCurrency("Enter currency to exchange from (USD, EUR, GBP, LOCAL): ");
    double amount = readDouble("Enter amount to exchange: ", 0.01);
    Currency toCurrency = readCurrency("Enter currency to exchange to (USD, EUR, GBP, LOCAL): ");

}