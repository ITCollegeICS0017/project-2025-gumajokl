#ifndef TYPES_H
#define TYPES_H

enum class Currency{
    USD,
    EUR,
    GBP
};

struct Transaction{
    double amount;
    Currency from;
    Currency to;
};

#endif