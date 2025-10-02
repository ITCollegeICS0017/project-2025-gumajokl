
#include "utils.h"

enum class Currency {
    USD,
    EUR,
    GBP
};

struct Transaction {
    double amount;
    Currency from;
    Currency to;
};