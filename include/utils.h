#pragma once

#include <string>


enum class Currency {
    USD,
    EUR,
    GBP,
    LOCAL,
    UNKNOWN
};

enum class PersonRole {
    CASHIER,
    MANAGER,
    CLIENT,
    UNKNOWN
};

std::string string_from_currency(Currency currency);
Currency currency_from_string(const std::string& curr_str);


