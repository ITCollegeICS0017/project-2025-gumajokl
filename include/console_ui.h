#pragma once

#include "utils.h"

#include <string>


class ConsoleUI {
    private:
        DataStore& dataStore;    

        std::string readLine(std::string prompt) const;
        int readInt(const std::string& prompt, int minValue, int maxValue) const;
        double readDouble(const std::string& prompt, double minValue) const;
        Currency readCurrency(const std::string& prompt) const;
        bool readYesNo(const std::string& prompt) const;

        void mainMenu();

        void cashierMenu();
        void cashierExchange(Cashier &cashier);

        void ReserveCheck() const;

        void managerSession();
        void managerShowReport();
        void managerAdjustRates();
        void managerSetCriticalReserve();


    public:
        ConsoleUI(DataStore& dataStore_in);
        void start();
};