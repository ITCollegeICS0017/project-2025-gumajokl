#include "utils.h"

#include <algorithm>
#include <cctype>
#include <map>

namespace {
    constexpr double kCurrencyEpsilon = 1e-8;
}

std::string to_string(Currency currency) {
    switch (currency) {
        case Currency::USD: return "USD";
        case Currency::EUR: return "EUR";
        case Currency::GBP: return "GBP";
        case Currency::LOCAL: return "LOCAL";
        default: return "UNKNOWN";
    }
}

Currency currency_from_string(const std::string& symbol) {
    std::string normalized;
    normalized.reserve(symbol.size());
    std::transform(symbol.begin(), symbol.end(), std::back_inserter(normalized), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    if (normalized == "USD") return Currency::USD;
    if (normalized == "EUR") return Currency::EUR;
    if (normalized == "GBP") return Currency::GBP;
    if (normalized == "LOCAL" || normalized == "LC" || normalized == "LOC") return Currency::LOCAL;
    throw ExchangeError("Unsupported currency symbol: " + symbol);
}

ExchangeError::ExchangeError(const std::string& message) : std::runtime_error(message) {}

RateNotFoundError::RateNotFoundError(const std::string& message) : ExchangeError(message) {}

ReserveError::ReserveError(const std::string& message) : ExchangeError(message) {}

ExchangePortion::ExchangePortion(Currency target, double amount)
    : targetCurrency(target),
      sourceAmount(amount),
      useRemainder(false),
      denominations{} {
    if (amount < 0.0) {
        throw ExchangeError("ExchangePortion amount cannot be negative");
    }
}

ExchangePortion ExchangePortion::remainder(Currency target) {
    ExchangePortion portion(target, 0.0);
    portion.useRemainder = true;
    return portion;
}

ExchangeRequest::ExchangeRequest(int clientIdentifier, std::string client, Currency source, double amount, std::vector<ExchangePortion> parts)
    : clientId(clientIdentifier),
      clientName(std::move(client)),
      sourceCurrency(source),
      totalAmount(amount),
      portions(std::move(parts)) {
    if (totalAmount <= 0.0) {
        throw ExchangeError("Requested exchange amount must be positive");
    }
    if (hasRemainderPortion()) {
        int remainderCount = 0;
        for (const auto& portion : portions) {
            if (portion.useRemainder) {
                remainderCount++;
            }
        }
        if (remainderCount > 1) {
            throw ExchangeError("Only one remainder portion is allowed per request");
        }
    }
    if (totalAllocatedSource() - totalAmount > kCurrencyEpsilon) {
        throw ExchangeError("Allocated source amount exceeds total available amount");
    }
}

double ExchangeRequest::totalAllocatedSource() const {
    double allocated = 0.0;
    for (const auto& portion : portions) {
        if (!portion.useRemainder) {
            allocated += portion.sourceAmount;
        }
    }
    return allocated;
}

bool ExchangeRequest::hasRemainderPortion() const {
    return std::any_of(portions.begin(), portions.end(), [](const ExchangePortion& portion) {
        return portion.useRemainder;
    });
}
