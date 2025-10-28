#pragma once

#include "client.h"
#include "employee.h"
#include "persistence.h"

#include <functional>
#include <string>
#include <vector>

class ConsoleUI {
private:
    ExchangeOffice& office;
    DataStore& store;

    std::string readLine(const std::string& prompt) const;
    int readInt(const std::string& prompt, int minValue, int maxValue) const;
    double readDouble(const std::string& prompt, double minValue) const;
    Currency readCurrency(const std::string& prompt) const;
    bool readYesNo(const std::string& prompt) const;
    std::vector<int> readDenominations(const std::string& prompt) const;

    void employeeSession();
    void employeeExchangeFlow(Cashier& cashier);
    void employeeReserveCheck() const;

    void managerSession();
    void managerShowReport(Manager& manager);
    void managerAdjustRates(Manager& manager);
    void managerSetCriticalReserve(Manager& manager);

    void persistReserve() const;
    void persistRates() const;
    void persistCriticalMinimums() const;

public:
    ConsoleUI(ExchangeOffice& office, DataStore& persistence);

    void run();
};
