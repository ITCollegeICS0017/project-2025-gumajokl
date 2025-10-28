#pragma once

#include "utils.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

class Reserve {
private:
    std::map<Currency, double> balances;

public:
    Reserve();
    explicit Reserve(const std::map<Currency, double>& initialBalances);

    double getBalance(Currency currency) const;
    void setBalance(Currency currency, double amount);
    void deposit(Currency currency, double amount);
    void withdraw(Currency currency, double amount);
    bool canWithdraw(Currency currency, double amount) const;
    const std::map<Currency, double>& allBalances() const;
};

class RateTable {
private:
    std::map<std::pair<Currency, Currency>, double> directRates;
    Currency baseCurrency;

public:
    explicit RateTable(Currency base = Currency::LOCAL);

    void setRate(Currency from, Currency to, double rate);
    double getRate(Currency from, Currency to) const;
    bool canConvert(Currency from, Currency to) const;
    double convert(double amount, Currency from, Currency to) const;
    Currency base() const;
    std::vector<std::tuple<Currency, Currency, double>> serialize() const;
};

struct TransactionRecord {
    int receiptId;
    int cashierId;
    std::string cashierName;
    int clientId;
    std::string clientName;
    Currency sourceCurrency;
    double sourceAmount;
    std::vector<PayoutDetail> payouts;
    double profitInBaseCurrency;
    time_t timestamp;
};

class Receipt {
private:
    int transactionID;
    int cashierId;
    std::string cashierName;
    int clientId;
    std::string clientName;
    Currency sourceCurrency;
    double sourceAmount;
    std::vector<PayoutDetail> payoutDetails;
    double profitBaseCurrency;
    double commissionBaseCurrency;
    time_t transactionTime;

public:
    Receipt(int id,
            int cashierIdentifier,
            std::string cashier,
            int clientIdentifier,
            std::string client,
            Currency fromCurrency,
            double fromAmount,
            std::vector<PayoutDetail> details,
            double profitBase,
            double commissionBase,
            time_t timestamp);

    int id() const;
    int cashierIdentifier() const;
    const std::string& cashier() const;
    int clientIdentifier() const;
    const std::string& client() const;
    Currency source() const;
    double sourceAmountValue() const;
    const std::vector<PayoutDetail>& payouts() const;
    double profitInBase() const;
    double commissionInBase() const;
    time_t timestamp() const;
};

class DailyReport {
private:
    std::map<Currency, double> startingBalances;
    std::map<Currency, double> endingBalances;
    std::map<Currency, double> minimumThresholds;
    std::vector<TransactionRecord> transactions;
    double totalProfitBase;
    time_t generatedAt;

public:
    DailyReport(std::map<Currency, double> start,
                std::map<Currency, double> end,
                std::map<Currency, double> thresholds,
                std::vector<TransactionRecord> records,
                double profit,
                time_t generated);

    const std::map<Currency, double>& startBalances() const;
    const std::map<Currency, double>& endBalances() const;
    const std::map<Currency, double>& criticalThresholds() const;
    const std::vector<TransactionRecord>& history() const;
    double profitInBase() const;
    time_t generatedOn() const;
};

class BonusPolicy {
public:
    virtual ~BonusPolicy() = default;
    virtual double calculateBonus(double profitBaseCurrency) const = 0;
};

class PercentageBonusPolicy : public BonusPolicy {
private:
    double percentage;

public:
    explicit PercentageBonusPolicy(double percent);
    double calculateBonus(double profitBaseCurrency) const override;
};

class ExchangeOffice {
private:
    RateTable rateTable;
    Reserve currentReserve;
    Reserve startingReserve;
    std::map<Currency, double> criticalMinimums;
    std::vector<TransactionRecord> dailyTransactions;
    double profitInBase;
    double commissionPercent;
    int nextReceiptId;

    double commissionFor(double amount) const;

public:
    ExchangeOffice(RateTable rates, Reserve reserve, double commission);

    Receipt executeTransaction(const ExchangeRequest& request, const std::string& cashierName, int cashierId);
    bool isBelowCritical(Currency currency) const;
    double criticalMinimum(Currency currency) const;
    void setCriticalMinimum(Currency currency, double amount);
    void initializeCriticalMinimums(const std::map<Currency, double>& minima);
    void topUpReserve(Currency currency, double amount);
    void reduceReserve(Currency currency, double amount);
    void updateRate(Currency from, Currency to, double rate);

    double currentProfitBase() const;
    const Reserve& reserve() const;
    RateTable& rateConfig();
    const RateTable& rateConfig() const;
    const std::map<Currency, double>& criticalMinimumsMap() const;

    DailyReport compileDailyReport() const;
    void resetDailyCycle();
};
