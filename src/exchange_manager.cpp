#include "exchange_manager.h"

Receipt::Receipt(int number) : transactionID(number) {}

Reserve::Reserve(float usd, float eur, float gbp) : totalUSD(usd), totalEUR(eur), totalGBP(gbp) {}