#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <ctime>

enum class Currency {
    USD,
    EUR,
    GBP,
    LOCAL
};

std::string to_string(Currency currency);
Currency currency_from_string(const std::string& symbol);

class ExchangeError : public std::runtime_error {
public:
    explicit ExchangeError(const std::string& message);
};

class RateNotFoundError : public ExchangeError {
public:
    explicit RateNotFoundError(const std::string& message);
};

class ReserveError : public ExchangeError {
public:
    explicit ReserveError(const std::string& message);
};

struct ExchangePortion {
    Currency targetCurrency;
    double sourceAmount;          // How much of the source currency should be converted
    bool useRemainder;            // If true, consume the remaining unallocated source amount
    std::vector<int> denominations; // Preferred denominations for payout

    ExchangePortion(Currency target, double amount);
    static ExchangePortion remainder(Currency target);
};

struct ExchangeRequest {
    int clientId;
    std::string clientName;
    Currency sourceCurrency;
    double totalAmount;
    std::vector<ExchangePortion> portions;

    ExchangeRequest(int clientIdentifier, std::string client, Currency source, double amount, std::vector<ExchangePortion> parts = {});
    double totalAllocatedSource() const;
    bool hasRemainderPortion() const;
};

struct PayoutDetail {
    Currency currency;
    double amountPaid;
    double commissionTaken;
    std::vector<int> denominations;
};
