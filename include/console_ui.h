#pragma once

#include "utils.h"
#include "employee.h"
#include "data_storage.h"
#include "exchange_manager.h"

#include <string>


class ConsoleUI {
    private:
        DataStore& dataStore;    
        ExchangeOffice& exchangeOffice;

        std::string readLine(std::string prompt) const;
        int readInt(const std::string& prompt, int minValue, int maxValue) const;
        double readDouble(const std::string& prompt, double minValue) const;
        Currency readCurrency(const std::string& prompt) const;
        bool readYesNo(const std::string& prompt) const;

        void printReceipt(const Receipt& receipt);


        void cashierMenu();
        void cashierPerformExchange(Cashier &cashier);

        void employeeReserveCheck() const;

        void managerSession();
        void managerShowReport();
        void managerAdjustRates();
        void managerSetCriticalReserve();


    public:
        ConsoleUI(DataStore& dataStore_in, ExchangeOffice& exchangeOffice_in);
        void mainMenu();
};