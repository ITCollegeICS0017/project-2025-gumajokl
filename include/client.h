#pragma once

#include "utils.h"

#include <string>
#include <vector>

class Client {
private:
    int clientId;
    std::string clientName;

public:
    Client(int id, std::string name);

    int id() const;
    const std::string& name() const;
    ExchangeRequest createSimpleRequest(Currency sourceCurrency, double amount, Currency targetCurrency) const;
    ExchangeRequest createCustomRequest(Currency sourceCurrency, double amount, std::vector<ExchangePortion> portions) const;
};
