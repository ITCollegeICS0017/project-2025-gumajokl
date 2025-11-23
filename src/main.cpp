#include "console_ui.h"
#include "exchange_manager.h"
#include "persistence.h"
#include "utils.h"

#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <tuple>

namespace {
    std::map<Currency, double> defaultReserveBalances() {
        return {
            {Currency::USD, 10'000.0},
            {Currency::EUR, 8'000.0},
            {Currency::GBP, 6'500.0},
            {Currency::LOCAL, 12'000.0}
        };
    }

    void ensureDefaultRates(RateTable& table) {
        table.setRate(Currency::USD, Currency::LOCAL, 1.08);
        table.setRate(Currency::EUR, Currency::LOCAL, 1.00);
        table.setRate(Currency::GBP, Currency::LOCAL, 1.22);
    }

    std::map<Currency, double> defaultCriticalMinimums() {
        return {
            {Currency::USD, 1'000.0},
            {Currency::EUR, 900.0},
            {Currency::GBP, 800.0},
            {Currency::LOCAL, 1'500.0}
        };
    }
}

int main(int argc, char* argv[]) {
    try {
        for (int index = 1; index < argc; ++index) {
            std::string argument = argv[index];
            if (argument == "--console") {
                continue; // Console mode is now the only supported interface.
            } else if (argument == "--gui" || argument == "--port" || argument == "-p") {
                throw ExchangeError("Web GUI support has been removed. Please use the terminal interface.");
            } else {
                throw ExchangeError("Unknown argument: " + argument);
            }
        }

        DataStore store("data");
        store.initialize();

        RateTable rateTable(Currency::LOCAL);
        auto storedRates = store.loadRates();
        if (storedRates.empty()) {
            ensureDefaultRates(rateTable);
            store.saveRates(rateTable);
        } else {
            for (const auto& entry : storedRates) {
                Currency from;
                Currency to;
                double rate;
                std::tie(from, to, rate) = entry;
                rateTable.setRate(from, to, rate);
            }
        }

        std::map<Currency, double> reserveBalances = store.loadReserve(defaultReserveBalances());
        Reserve reserve(reserveBalances);

        ExchangeOffice office(rateTable, reserve, 0.03);

        auto criticalMinima = store.loadCriticalMinimums();
        if (criticalMinima.empty()) {
            criticalMinima = defaultCriticalMinimums();
            store.saveCriticalMinimums(criticalMinima);
        }
        office.initializeCriticalMinimums(criticalMinima);

        ConsoleUI ui(office, store);
        ui.run();

        store.saveReserve(office.reserve().allBalances());
        store.saveRates(office.rateConfig());
        store.saveCriticalMinimums(office.criticalMinimumsMap());
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
