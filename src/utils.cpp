#include "utils.h"



std::string string_from_currency(Currency currency) {
    switch (currency) {
        case Currency::USD:
            return "USD";
        case Currency::EUR:
            return "EUR";
        case Currency::GBP:
            return "GBP";
        case Currency::LOCAL:
            return "LOCAL";
        default:
            return "UNKNOWN";
    }
}

Currency currency_from_string(const std::string& curr_str) {
    if (curr_str == "USD") {
        return Currency::USD;
    } else if (curr_str == "EUR") {
        return Currency::EUR;
    } else if (curr_str == "GBP") {
        return Currency::GBP;
    } else if (curr_str == "LOCAL") {
        return Currency::LOCAL;
    }
    return Currency::UNKNOWN;
}



