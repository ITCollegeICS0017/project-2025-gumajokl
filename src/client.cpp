#include "client.h"

#include <utility>

Client::Client(int id, std::string name)
    : clientId(id),
      clientName(std::move(name)) {}

int Client::id() const {
    return clientId;
}

const std::string& Client::name() const {
    return clientName;
}

ExchangeRequest Client::createSimpleRequest(Currency sourceCurrency, double amount, Currency targetCurrency) const {
    std::vector<ExchangePortion> portions;
    auto portion = ExchangePortion::remainder(targetCurrency);
    portions.push_back(portion);
    return ExchangeRequest(clientId, clientName, sourceCurrency, amount, portions);
}

ExchangeRequest Client::createCustomRequest(Currency sourceCurrency, double amount, std::vector<ExchangePortion> portions) const {
    return ExchangeRequest(clientId, clientName, sourceCurrency, amount, std::move(portions));
}
