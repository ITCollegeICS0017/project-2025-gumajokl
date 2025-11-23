#include "exchange_manager.h"

#include <algorithm>
#include <ctime>
#include <utility>

namespace {
    constexpr double kEpsilon = 1e-8;
}

Reserve::Reserve() = default;

Reserve::Reserve(const std::map<Currency, double>& initialBalances) : balances(initialBalances) {}

double Reserve::getBalance(Currency currency) const {
    auto iterator = balances.find(currency);
    return iterator != balances.end() ? iterator->second : 0.0;
}

void Reserve::setBalance(Currency currency, double amount) {
    if (amount < 0.0) {
        throw ReserveError("Balance cannot be negative");
    }
    balances[currency] = amount;
}

void Reserve::deposit(Currency currency, double amount) {
    if (amount < 0.0) {
        throw ReserveError("Cannot deposit a negative amount");
    }
    balances[currency] = getBalance(currency) + amount;
}

void Reserve::withdraw(Currency currency, double amount) {
    if (!canWithdraw(currency, amount)) {
        throw ReserveError("Insufficient reserve for withdrawal");
    }
    balances[currency] = getBalance(currency) - amount;
}

bool Reserve::canWithdraw(Currency currency, double amount) const {
    if (amount < 0.0) {
        return false;
    }
    return getBalance(currency) + kEpsilon >= amount;
}

const std::map<Currency, double>& Reserve::allBalances() const {
    return balances;
}

RateTable::RateTable(Currency base) : baseCurrency(base) {}

void RateTable::setRate(Currency from, Currency to, double rate) {
    if (rate <= 0.0) {
        throw ExchangeError("Exchange rate must be positive");
    }
    directRates[{from, to}] = rate;
    directRates[{to, from}] = 1.0 / rate;
}

double RateTable::getRate(Currency from, Currency to) const {
    if (from == to) {
        return 1.0;
    }
    auto iterator = directRates.find({from, to});
    if (iterator != directRates.end()) {
        return iterator->second;
    }
    throw RateNotFoundError("Rate not configured for conversion from " + to_string(from) + " to " + to_string(to));
}

bool RateTable::canConvert(Currency from, Currency to) const {
    if (from == to) {
        return true;
    }
    if (directRates.count({from, to}) > 0) {
        return true;
    }
    try {
        (void)getRate(from, to);
        return true;
    } catch (...) {
        // ignored
    }
    // Attempt via base currency
    auto toBase = directRates.find({from, baseCurrency});
    auto fromBase = directRates.find({baseCurrency, to});
    return toBase != directRates.end() && fromBase != directRates.end();
}

double RateTable::convert(double amount, Currency from, Currency to) const {
    if (from == to) {
        return amount;
    }
    auto iterator = directRates.find({from, to});
    if (iterator != directRates.end()) {
        return amount * iterator->second;
    }
    // Attempt conversion via base currency
    auto toBase = directRates.find({from, baseCurrency});
    auto fromBase = directRates.find({baseCurrency, to});
    if (toBase != directRates.end() && fromBase != directRates.end()) {
        double amountInBase = amount * toBase->second;
        return amountInBase * fromBase->second;
    }
    throw RateNotFoundError("Unable to convert from " + to_string(from) + " to " + to_string(to));
}

Currency RateTable::base() const {
    return baseCurrency;
}

std::vector<std::tuple<Currency, Currency, double>> RateTable::serialize() const {
    std::vector<std::tuple<Currency, Currency, double>> entries;
    entries.reserve(directRates.size());
    for (const auto& entry : directRates) {
        auto from = entry.first.first;
        auto to = entry.first.second;
        if (static_cast<int>(from) < static_cast<int>(to)) {
            entries.emplace_back(from, to, entry.second);
        }
    }
    return entries;
}

Receipt::Receipt(int id,
                 int cashierIdentifier,
                 std::string cashier,
                 int clientIdentifier,
                 std::string client,
                 Currency fromCurrency,
                 double fromAmount,
                 std::vector<PayoutDetail> details,
                 double profitBase,
                 double commissionBase,
                 time_t timestamp)
    : transactionID(id),
      cashierId(cashierIdentifier),
      cashierName(std::move(cashier)),
      clientId(clientIdentifier),
      clientName(std::move(client)),
      sourceCurrency(fromCurrency),
      sourceAmount(fromAmount),
      payoutDetails(std::move(details)),
      profitBaseCurrency(profitBase),
      commissionBaseCurrency(commissionBase),
      transactionTime(timestamp) {}

int Receipt::id() const {
    return transactionID;
}

int Receipt::cashierIdentifier() const {
    return cashierId;
}

const std::string& Receipt::cashier() const {
    return cashierName;
}

int Receipt::clientIdentifier() const {
    return clientId;
}

const std::string& Receipt::client() const {
    return clientName;
}

Currency Receipt::source() const {
    return sourceCurrency;
}

double Receipt::sourceAmountValue() const {
    return sourceAmount;
}

const std::vector<PayoutDetail>& Receipt::payouts() const {
    return payoutDetails;
}

double Receipt::profitInBase() const {
    return profitBaseCurrency;
}

double Receipt::commissionInBase() const {
    return commissionBaseCurrency;
}

time_t Receipt::timestamp() const {
    return transactionTime;
}

DailyReport::DailyReport(std::map<Currency, double> start,
                         std::map<Currency, double> end,
                         std::map<Currency, double> thresholds,
                         std::vector<TransactionRecord> records,
                         double profit,
                         time_t generated)
    : startingBalances(std::move(start)),
      endingBalances(std::move(end)),
      minimumThresholds(std::move(thresholds)),
      transactions(std::move(records)),
      totalProfitBase(profit),
      generatedAt(generated) {}

const std::map<Currency, double>& DailyReport::startBalances() const {
    return startingBalances;
}

const std::map<Currency, double>& DailyReport::endBalances() const {
    return endingBalances;
}

const std::map<Currency, double>& DailyReport::criticalThresholds() const {
    return minimumThresholds;
}

const std::vector<TransactionRecord>& DailyReport::history() const {
    return transactions;
}

double DailyReport::profitInBase() const {
    return totalProfitBase;
}

time_t DailyReport::generatedOn() const {
    return generatedAt;
}

PercentageBonusPolicy::PercentageBonusPolicy(double percent) : percentage(percent) {
    if (percentage < 0.0) {
        throw ExchangeError("Bonus percentage cannot be negative");
    }
}

double PercentageBonusPolicy::calculateBonus(double profitBaseCurrency) const {
    return profitBaseCurrency * percentage;
}

ExchangeOffice::ExchangeOffice(RateTable rates, Reserve reserve, double commission)
    : rateTable(std::move(rates)),
      currentReserve(std::move(reserve)),
      startingReserve(currentReserve),
      profitInBase(0.0),
      commissionPercent(commission),
      nextReceiptId(1) {
    if (commissionPercent < 0.0 || commissionPercent >= 1.0) {
        throw ExchangeError("Commission percentage must be between 0 and 1");
    }
}

double ExchangeOffice::commissionFor(double amount) const {
    return amount * commissionPercent;
}

Receipt ExchangeOffice::executeTransaction(const ExchangeRequest& request, const std::string& cashierName, int cashierId) {
    if (request.totalAllocatedSource() - request.totalAmount > kEpsilon) {
        throw ExchangeError("Requested source allocation exceeds available amount");
    }

    double remainingSource = request.totalAmount;
    std::vector<PayoutDetail> payoutDetails;
    double profitBase = 0.0;
    double commissionBase = 0.0;

    for (const auto& portion : request.portions) {
        double sourceSlice = portion.useRemainder ? remainingSource : portion.sourceAmount;
        if (sourceSlice < 0.0) {
            throw ExchangeError("Source slice cannot be negative");
        }
        if (sourceSlice > remainingSource + kEpsilon) {
            throw ExchangeError("Portion exceeds remaining source amount");
        }
        if (sourceSlice < kEpsilon) {
            continue;
        }

        if (!rateTable.canConvert(request.sourceCurrency, portion.targetCurrency)) {
            throw RateNotFoundError("No rate for converting from " + to_string(request.sourceCurrency) + " to " + to_string(portion.targetCurrency));
        }

        double convertedAmount = rateTable.convert(sourceSlice, request.sourceCurrency, portion.targetCurrency);
        double commission = commissionFor(convertedAmount);
        double payout = convertedAmount - commission;

        if (!currentReserve.canWithdraw(portion.targetCurrency, convertedAmount)) {
            throw ReserveError("Insufficient reserve for " + to_string(portion.targetCurrency));
        }

        currentReserve.withdraw(portion.targetCurrency, convertedAmount);
        currentReserve.deposit(portion.targetCurrency, commission);
        currentReserve.deposit(request.sourceCurrency, sourceSlice);

        double commissionInBase = rateTable.convert(commission, portion.targetCurrency, rateTable.base());
        profitBase += commissionInBase;
        commissionBase += commissionInBase;

        payoutDetails.push_back(PayoutDetail{
            portion.targetCurrency,
            payout,
            commission,
            portion.denominations
        });

        remainingSource -= sourceSlice;
    }

    // Any remainder is returned to the client in the original currency, so no reserve change.
    double usedSource = request.totalAmount - remainingSource;
    time_t now = std::time(nullptr);
    int receiptId = nextReceiptId++;

    profitInBase += profitBase;

    TransactionRecord record{
        receiptId,
        cashierId,
        cashierName,
        request.clientId,
        request.clientName,
        request.sourceCurrency,
        usedSource,
        payoutDetails,
        profitBase,
        now
    };
    dailyTransactions.push_back(record);

    return Receipt(receiptId,
                   cashierId,
                   cashierName,
                   request.clientId,
                   request.clientName,
                   request.sourceCurrency,
                   usedSource,
                   payoutDetails,
                   profitBase,
                   commissionBase,
                   now);
}

bool ExchangeOffice::isBelowCritical(Currency currency) const {
    auto threshold = criticalMinimums.find(currency);
    if (threshold == criticalMinimums.end()) {
        return false;
    }
    return currentReserve.getBalance(currency) < threshold->second;
}

double ExchangeOffice::criticalMinimum(Currency currency) const {
    auto iterator = criticalMinimums.find(currency);
    return iterator != criticalMinimums.end() ? iterator->second : 0.0;
}

void ExchangeOffice::setCriticalMinimum(Currency currency, double amount) {
    if (amount < 0.0) {
        throw ExchangeError("Critical minimum cannot be negative");
    }
    criticalMinimums[currency] = amount;
}

void ExchangeOffice::initializeCriticalMinimums(const std::map<Currency, double>& minima) {
    for (const auto& [currency, amount] : minima) {
        if (amount < 0.0) {
            throw ExchangeError("Critical minimum cannot be negative");
        }
    }
    criticalMinimums = minima;
}

void ExchangeOffice::topUpReserve(Currency currency, double amount) {
    currentReserve.deposit(currency, amount);
}

void ExchangeOffice::reduceReserve(Currency currency, double amount) {
    currentReserve.withdraw(currency, amount);
}

void ExchangeOffice::updateRate(Currency from, Currency to, double rate) {
    rateTable.setRate(from, to, rate);
}

double ExchangeOffice::currentProfitBase() const {
    return profitInBase;
}

const Reserve& ExchangeOffice::reserve() const {
    return currentReserve;
}

RateTable& ExchangeOffice::rateConfig() {
    return rateTable;
}

const RateTable& ExchangeOffice::rateConfig() const {
    return rateTable;
}

const std::map<Currency, double>& ExchangeOffice::criticalMinimumsMap() const {
    return criticalMinimums;
}

DailyReport ExchangeOffice::compileDailyReport() const {
    return DailyReport(
        startingReserve.allBalances(),
        currentReserve.allBalances(),
        criticalMinimums,
        dailyTransactions,
        profitInBase,
        std::time(nullptr)
    );
}

void ExchangeOffice::resetDailyCycle() {
    startingReserve = currentReserve;
    dailyTransactions.clear();
    profitInBase = 0.0;
}
