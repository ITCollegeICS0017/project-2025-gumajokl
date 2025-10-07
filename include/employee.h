#pragma once

#include "exchange_manager.h"
#include "utils.h"

#include <memory>
#include <string>
#include <vector>

class Employee {
protected:
    int id;
    std::string name;

public:
    Employee(int employeeId, std::string employeeName);
    virtual ~Employee() = default;

    int getId() const;
    const std::string& getName() const;
    virtual std::string role() const = 0;
    virtual void performDailyDuties() = 0;
};

class Cashier : public Employee {
private:
    ExchangeOffice& office;

public:
    Cashier(int cashierId, std::string cashierName, ExchangeOffice& exchangeOffice);

    Receipt handleRequest(const ExchangeRequest& request);
    void printReceipt(const Receipt& receipt) const;
    bool collectLowReserveAlerts(std::vector<Currency>& lowCurrencies) const;

    std::string role() const override;
    void performDailyDuties() override;
};

class Manager : public Employee {
private:
    ExchangeOffice& office;
    std::unique_ptr<BonusPolicy> bonusPolicy;

public:
    Manager(int managerId,
            std::string managerName,
            ExchangeOffice& exchangeOffice,
            std::unique_ptr<BonusPolicy> policy = std::make_unique<PercentageBonusPolicy>(0.05));

    std::string role() const override;
    void performDailyDuties() override;

    void setExchangeRate(Currency from, Currency to, double rate);
    void setCriticalReserve(Currency currency, double amount);
    void topUpReserve(Currency currency, double amount);
    double calculateBonus(double profitBaseCurrency) const;
    DailyReport compileDailyReport() const;
};
