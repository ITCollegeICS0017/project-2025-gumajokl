#include "employee.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

namespace {
    void printPayout(std::ostream& stream, const PayoutDetail& payout) {
        stream << "  -> " << to_string(payout.currency)
               << " amount: " << std::fixed << std::setprecision(2) << payout.amountPaid;
        if (!payout.denominations.empty()) {
            stream << " (denominations: ";
            for (std::size_t i = 0; i < payout.denominations.size(); ++i) {
                stream << payout.denominations[i];
                if (i + 1 != payout.denominations.size()) {
                    stream << ", ";
                }
            }
            stream << ")";
        }
        stream << '\n';
    }
}

Employee::Employee(int employeeId, std::string employeeName)
    : id(employeeId),
      name(std::move(employeeName)) {}

int Employee::getId() const {
    return id;
}

const std::string& Employee::getName() const {
    return name;
}

Cashier::Cashier(int cashierId, std::string cashierName, ExchangeOffice& exchangeOffice)
    : Employee(cashierId, std::move(cashierName)),
      office(exchangeOffice) {}

Receipt Cashier::handleRequest(const ExchangeRequest& request) {
    return office.executeTransaction(request, name, id);
}

void Cashier::printReceipt(const Receipt& receipt) const {
    std::ostringstream builder;
    builder << "Receipt #" << receipt.id() << " for client " << receipt.client()
            << " (ID " << receipt.clientIdentifier() << ") handled by "
            << receipt.cashier() << " (ID " << receipt.cashierIdentifier() << ")\n";
    builder << "Source: " << to_string(receipt.source()) << " amount "
            << std::fixed << std::setprecision(2) << receipt.sourceAmountValue() << '\n';
    for (const auto& payout : receipt.payouts()) {
        printPayout(builder, payout);
    }
    builder << "Profit (base): " << std::fixed << std::setprecision(2) << receipt.profitInBase() << '\n';
    std::cout << builder.str();
}

bool Cashier::collectLowReserveAlerts(std::vector<Currency>& lowCurrencies) const {
    bool any = false;
    for (const auto& balance : office.reserve().allBalances()) {
        if (office.isBelowCritical(balance.first)) {
            lowCurrencies.push_back(balance.first);
            any = true;
        }
    }
    return any;
}

std::string Cashier::role() const {
    return "Cashier";
}

void Cashier::performDailyDuties() {
    std::vector<Currency> lowCurrencies;
    if (collectLowReserveAlerts(lowCurrencies)) {
        std::cout << "Cashier " << name << " (ID " << id << ") alerts: low reserves for ";
        for (std::size_t i = 0; i < lowCurrencies.size(); ++i) {
            std::cout << to_string(lowCurrencies[i]);
            if (i + 1 != lowCurrencies.size()) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "Cashier " << name << " (ID " << id << "): all reserves above critical thresholds.\n";
    }
}

Manager::Manager(int managerId, std::string managerName, ExchangeOffice& exchangeOffice, std::unique_ptr<BonusPolicy> policy)
    : Employee(managerId, std::move(managerName)),
      office(exchangeOffice),
      bonusPolicy(std::move(policy)) {}

std::string Manager::role() const {
    return "Manager";
}

void Manager::performDailyDuties() {
    auto report = compileDailyReport();
    std::cout << "Manager " << name << " (ID " << id << ") daily report (profit base: "
              << std::fixed << std::setprecision(2) << report.profitInBase() << ")\n";
    for (const auto& [currency, balance] : report.endBalances()) {
        std::cout << "  Reserve " << to_string(currency) << ": " << std::fixed << std::setprecision(2) << balance;
        auto thresholdIt = report.criticalThresholds().find(currency);
        if (thresholdIt != report.criticalThresholds().end()) {
            std::cout << " (critical min " << std::fixed << std::setprecision(2) << thresholdIt->second << ")";
        }
        std::cout << '\n';
    }
}

void Manager::setExchangeRate(Currency from, Currency to, double rate) {
    office.updateRate(from, to, rate);
}

void Manager::setCriticalReserve(Currency currency, double amount) {
    office.setCriticalMinimum(currency, amount);
}

void Manager::topUpReserve(Currency currency, double amount) {
    office.topUpReserve(currency, amount);
}

double Manager::calculateBonus(double profitBaseCurrency) const {
    return bonusPolicy ? bonusPolicy->calculateBonus(profitBaseCurrency) : 0.0;
}

DailyReport Manager::compileDailyReport() const {
    return office.compileDailyReport();
}
