#include "console_ui.h"

#include <cctype>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
    std::string trim(const std::string& text) {
        const auto first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return "";
        }
        const auto last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    std::string to_upper(std::string value) {
        for (auto& ch : value) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        return value;
    }
}

ConsoleUI::ConsoleUI(ExchangeOffice& officeRef, DataStore& persistence)
    : office(officeRef),
      store(persistence) {}

std::string ConsoleUI::readLine(const std::string& prompt) const {
    std::cout << prompt;
    std::cout.flush();
    std::string input;
    if (!std::getline(std::cin, input)) {
        throw ExchangeError("Input stream closed unexpectedly");
    }
    return trim(input);
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
        try {
            return currency_from_string(text);
        } catch (const std::exception& error) {
            std::cout << error.what() << ". Please try again.\n";
        }
    }
}

bool ConsoleUI::readYesNo(const std::string& prompt) const {
    while (true) {
        std::string text = to_upper(readLine(prompt));
        if (text == "Y" || text == "YES") {
            return true;
        }
        if (text == "N" || text == "NO") {
            return false;
        }
        std::cout << "Please enter 'y' or 'n'.\n";
    }
}

std::vector<int> ConsoleUI::readDenominations(const std::string& prompt) const {
    std::string text = readLine(prompt);
    std::vector<int> denominations;
    if (text.empty()) {
        return denominations;
    }

    std::istringstream stream(text);
    int value = 0;
    while (stream >> value) {
        if (value > 0) {
            denominations.push_back(value);
        }
    }
    return denominations;
}

void ConsoleUI::persistReserve() const {
    store.saveReserve(office.reserve().allBalances());
}

void ConsoleUI::persistRates() const {
    store.saveRates(office.rateConfig());
}

void ConsoleUI::persistCriticalMinimums() const {
    store.saveCriticalMinimums(office.criticalMinimumsMap());
}

void ConsoleUI::run() {
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
                    employeeSession();
                } catch (const std::exception& error) {
                    std::cout << "Employee session ended: " << error.what() << '\n';
                }
                break;
            case 2:
                try {
                    managerSession();
                } catch (const std::exception& error) {
                    std::cout << "Manager session ended: " << error.what() << '\n';
                }
                break;
            case 3:
                running = false;
                break;
        }
    }

    std::cout << "Shutting down. Goodbye!\n";
}

void ConsoleUI::employeeSession() {
    std::string employeeName = readLine("Enter employee name: ");
    int employeeId = store.ensurePersonId("cashier", employeeName);
    Cashier cashier(employeeId, employeeName, office);

    bool active = true;
    while (active) {
        std::cout << "\n-- Cashier Menu --\n";
        std::cout << "1. Perform exchange\n";
        std::cout << "2. Check reserve balances\n";
        std::cout << "3. Logout\n";
        int choice = readInt("Select option: ", 1, 3);

        switch (choice) {
            case 1:
                employeeExchangeFlow(cashier);
                break;
            case 2:
                employeeReserveCheck();
                break;
            case 3:
                active = false;
                break;
        }
    }
}

void ConsoleUI::employeeExchangeFlow(Cashier& cashier) {
    try {
        std::string clientName = readLine("Client name: ");
        int clientId = store.ensurePersonId("client", clientName);
        Client client(clientId, clientName);

        Currency sourceCurrency = readCurrency("Source currency (USD/EUR/GBP/LOCAL): ");
        double totalAmount = readDouble("Amount to exchange (in source currency): ", 0.01);

        std::vector<ExchangePortion> portions;
        bool split = readYesNo("Split payout into multiple currencies? (y/n): ");
        bool remainderDeclared = false;

        auto collectDenominations = [this](int index) {
            return readDenominations("Preferred denominations for portion " + std::to_string(index) + " (space separated, blank for any): ");
        };

        if (!split) {
            Currency target = readCurrency("Target currency: ");
            ExchangePortion portion = ExchangePortion::remainder(target);
            portion.denominations = readDenominations("Preferred denominations (space separated, blank for any): ");
            portions.push_back(portion);
            remainderDeclared = true;
        } else {
            int portionCount = readInt("How many payout portions? (1-5): ", 1, 5);
            for (int i = 0; i < portionCount; ++i) {
                Currency target = readCurrency("Portion " + std::to_string(i + 1) + " target currency: ");
                std::string amountText = to_upper(readLine("Portion " + std::to_string(i + 1) + " amount in source currency or 'ALL': "));

                ExchangePortion portion = ExchangePortion::remainder(target);
                if (amountText != "ALL") {
                    try {
                        double sourceSlice = std::stod(amountText);
                        if (sourceSlice <= 0.0) {
                            throw ExchangeError("Portion amount must be positive");
                        }
                        portion = ExchangePortion(target, sourceSlice);
                    } catch (const std::exception&) {
                        throw ExchangeError("Invalid portion amount provided");
                    }
                } else {
                    if (remainderDeclared) {
                        throw ExchangeError("Only one portion may use the remainder");
                    }
                    portion = ExchangePortion::remainder(target);
                    remainderDeclared = true;
                }
                portion.denominations = collectDenominations(i + 1);
                portions.push_back(portion);
            }
        }

        if (!remainderDeclared) {
            std::cout << "No remainder specified. Any leftover source currency will be returned to the client.\n";
        }

        ExchangeRequest request(client.id(), client.name(), sourceCurrency, totalAmount, portions);
        Receipt receipt = cashier.handleRequest(request);
        cashier.printReceipt(receipt);
        store.appendTransaction(receipt);
        persistReserve();
    } catch (const std::exception& error) {
        std::cout << "Exchange failed: " << error.what() << '\n';
    }
}

void ConsoleUI::employeeReserveCheck() const {
    std::cout << "\nCurrent reserve balances:\n";
    for (const auto& [currency, balance] : office.reserve().allBalances()) {
        std::cout << "  " << to_string(currency) << ": " << std::fixed << std::setprecision(2) << balance;
        if (office.isBelowCritical(currency)) {
            std::cout << " (below critical minimum of "
                      << std::fixed << std::setprecision(2) << office.criticalMinimum(currency) << ")";
        }
        std::cout << '\n';
    }
}

void ConsoleUI::managerSession() {
    std::string managerName = readLine("Enter manager name: ");
    int managerId = store.ensurePersonId("manager", managerName);
    Manager manager(managerId, managerName, office);

    bool active = true;
    while (active) {
        std::cout << "\n-- Manager Menu --\n";
        std::cout << "1. Generate end-of-day report\n";
        std::cout << "2. Adjust exchange rate\n";
        std::cout << "3. Set critical reserve level\n";
        std::cout << "4. View reserve balances\n";
        std::cout << "5. Reset daily cycle\n";
        std::cout << "6. Logout\n";

        int choice = readInt("Select option: ", 1, 6);
        switch (choice) {
            case 1:
                managerShowReport(manager);
                break;
            case 2:
                managerAdjustRates(manager);
                break;
            case 3:
                managerSetCriticalReserve(manager);
                break;
            case 4:
                employeeReserveCheck();
                break;
            case 5:
                office.resetDailyCycle();
                persistReserve();
                std::cout << "Daily cycle reset. Starting balances updated.\n";
                break;
            case 6:
                active = false;
                break;
        }
    }
}

void ConsoleUI::managerShowReport(Manager& manager) {
    DailyReport report = manager.compileDailyReport();
    std::cout << "\n=== Daily Report ===\n";
    std::time_t generated = report.generatedOn();
    std::tm* generatedInfo = std::localtime(&generated);
    if (generatedInfo) {
        std::cout << "Generated at: " << std::put_time(generatedInfo, "%Y-%m-%d %H:%M:%S") << '\n';
    } else {
        std::cout << "Generated at timestamp: " << static_cast<long long>(generated) << '\n';
    }
    std::cout << "Profit (base currency): " << std::fixed << std::setprecision(2) << report.profitInBase() << '\n';

    std::cout << "\nEnding reserves:\n";
    for (const auto& [currency, balance] : report.endBalances()) {
        std::cout << "  " << to_string(currency) << ": " << std::fixed << std::setprecision(2) << balance;
        auto thresholdIt = report.criticalThresholds().find(currency);
        if (thresholdIt != report.criticalThresholds().end()) {
            std::cout << " (critical min " << std::fixed << std::setprecision(2) << thresholdIt->second << ")";
        }
        std::cout << '\n';
    }

    std::cout << "\nTransactions (" << report.history().size() << "):\n";
    for (const auto& record : report.history()) {
        std::cout << "  Receipt #" << record.receiptId << " | Cashier "
                  << record.cashierName << " (ID " << record.cashierId << ") | Client "
                  << record.clientName << " (ID " << record.clientId << ") | Source "
                  << to_string(record.sourceCurrency) << " "
                  << std::fixed << std::setprecision(2) << record.sourceAmount
                  << " | Profit base " << std::fixed << std::setprecision(2) << record.profitInBaseCurrency << '\n';
    }

    double bonus = manager.calculateBonus(report.profitInBase());
    std::cout << "Calculated cashier bonus (5% of profit): "
              << std::fixed << std::setprecision(2) << bonus << '\n';

    auto reportPath = store.persistReport(report, manager);
    std::cout << "Report saved to " << reportPath << '\n';
}

void ConsoleUI::managerAdjustRates(Manager& manager) {
    try {
        Currency from = readCurrency("From currency: ");
        Currency to = readCurrency("To currency: ");
        if (from == to) {
            throw ExchangeError("From and to currencies must be different");
        }
        double rate = readDouble("New rate (target per unit from-currency): ", 0.0001);

        manager.setExchangeRate(from, to, rate);
        persistRates();
        std::cout << "Exchange rate updated for " << to_string(from) << " -> " << to_string(to) << ".\n";
    } catch (const std::exception& error) {
        std::cout << "Rate update failed: " << error.what() << '\n';
    }
}

void ConsoleUI::managerSetCriticalReserve(Manager& manager) {
    Currency currency = readCurrency("Currency: ");
    double amount = readDouble("Critical minimum amount: ", 0.0);
    manager.setCriticalReserve(currency, amount);
    persistCriticalMinimums();
    std::cout << "Critical minimum updated for " << to_string(currency) << ".\n";
}
