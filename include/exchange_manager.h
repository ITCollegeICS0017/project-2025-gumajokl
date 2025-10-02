#pragma once

#include "utils.h"

#include <stdlib.h>

class Receipt {
    private:
        int transactionID;
    public:
        Receipt(int number);
        float exchangeRate; // Exchange rate for the transaction
        float CurrencyFrom;
        float CurrencyTo;
        float amountExchanged;
        Currency FromCurrency;
        Currency ToCurrency;
        time_t transactionTime; // Time of the transaction
};

class Reserve { // Implementation of Reserve class
    private: // Private members to hold total amounts of each currency
        float totalUSD;
        float totalEUR;
        float totalGBP;
    public: // Constructor to initialize the reserve amounts
        Reserve(float usd, float eur, float gbp);
};