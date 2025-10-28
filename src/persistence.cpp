#include "persistence.h"

#include "utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <cstdio>

namespace {
    std::string canonicalKey(const std::string& role, const std::string& name) {
        std::string key = role;
        key.append("|");
        key.reserve(role.size() + 1 + name.size());
        for (char ch : name) {
            key.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
        return key;
    }

    Currency parseCurrency(const std::string& token) {
        return currency_from_string(token);
    }
}

DataStore::DataStore(const std::string& baseDir)
    : baseDirectory(baseDir),
      reportsDirectory(baseDirectory / "reports"),
      nextPersonId(1) {}

void DataStore::initialize() {
    std::filesystem::create_directories(baseDirectory);
    std::filesystem::create_directories(reportsDirectory);
    loadPeople();
}

std::filesystem::path DataStore::reserveFile() const {
    return baseDirectory / "reserve.csv";
}

std::filesystem::path DataStore::ratesFile() const {
    return baseDirectory / "rates.csv";
}

std::filesystem::path DataStore::criticalFile() const {
    return baseDirectory / "critical.csv";
}

std::filesystem::path DataStore::peopleFile() const {
    return baseDirectory / "people.csv";
}

std::filesystem::path DataStore::transactionsFile() const {
    return baseDirectory / "transactions.log";
}

void DataStore::loadPeople() {
    people.clear();
    nextPersonId = 1;
    auto filePath = peopleFile();
    if (!std::filesystem::exists(filePath)) {
        return;
    }

    std::ifstream input(filePath);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream stream(line);
        std::string role;
        std::string idToken;
        std::string name;

        if (!std::getline(stream, role, ';')) {
            continue;
        }
        if (!std::getline(stream, idToken, ';')) {
            continue;
        }
        if (!std::getline(stream, name)) {
            continue;
        }
        try {
            int id = std::stoi(idToken);
            PersonEntry entry{id, role, name};
            people[canonicalKey(role, name)] = entry;
            if (id >= nextPersonId) {
                nextPersonId = id + 1;
            }
        } catch (...) {
            // Skip malformed lines
        }
    }
}

void DataStore::persistPeople() const {
    std::ofstream output(peopleFile(), std::ios::trunc);
    std::vector<PersonEntry> entries;
    entries.reserve(people.size());
    for (const auto& [_, entry] : people) {
        entries.push_back(entry);
    }
    std::sort(entries.begin(), entries.end(), [](const PersonEntry& lhs, const PersonEntry& rhs) {
        return lhs.id < rhs.id;
    });

    for (const auto& entry : entries) {
        output << entry.role << ';' << entry.id << ';' << entry.name << '\n';
    }
}

std::map<Currency, double> DataStore::loadReserve(const std::map<Currency, double>& defaults) const {
    auto filePath = reserveFile();
    if (!std::filesystem::exists(filePath)) {
        const_cast<DataStore*>(this)->saveReserve(defaults);
        return defaults;
    }

    std::ifstream input(filePath);
    std::map<Currency, double> balances = defaults;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream stream(line);
        std::string currencyToken;
        std::string amountToken;
        if (!std::getline(stream, currencyToken, ',')) {
            continue;
        }
        if (!std::getline(stream, amountToken)) {
            continue;
        }
        try {
            Currency currency = parseCurrency(currencyToken);
            double amount = std::stod(amountToken);
            balances[currency] = amount;
        } catch (...) {
            // Ignore malformed entries
        }
    }
    return balances;
}

void DataStore::saveReserve(const std::map<Currency, double>& balances) const {
    std::ofstream output(reserveFile(), std::ios::trunc);
    for (const auto& [currency, amount] : balances) {
        output << to_string(currency) << ',' << std::fixed << std::setprecision(2) << amount << '\n';
    }
}

std::vector<std::tuple<Currency, Currency, double>> DataStore::loadRates() const {
    auto filePath = ratesFile();
    std::vector<std::tuple<Currency, Currency, double>> rates;
    if (!std::filesystem::exists(filePath)) {
        return rates;
    }

    std::ifstream input(filePath);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream stream(line);
        std::string fromToken;
        std::string toToken;
        std::string rateToken;
        if (!std::getline(stream, fromToken, ',')) {
            continue;
        }
        if (!std::getline(stream, toToken, ',')) {
            continue;
        }
        if (!std::getline(stream, rateToken)) {
            continue;
        }
        try {
            Currency from = parseCurrency(fromToken);
            Currency to = parseCurrency(toToken);
            double rate = std::stod(rateToken);
            rates.emplace_back(from, to, rate);
        } catch (...) {
            // Ignore malformed entries
        }
    }
    return rates;
}

void DataStore::saveRates(const RateTable& table) const {
    std::ofstream output(ratesFile(), std::ios::trunc);
    for (const auto& entry : table.serialize()) {
        Currency from;
        Currency to;
        double rate;
        std::tie(from, to, rate) = entry;
        output << to_string(from) << ',' << to_string(to) << ',' << std::setprecision(10) << rate << '\n';
    }
}

std::map<Currency, double> DataStore::loadCriticalMinimums() const {
    auto filePath = criticalFile();
    std::map<Currency, double> minima;
    if (!std::filesystem::exists(filePath)) {
        return minima;
    }

    std::ifstream input(filePath);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream stream(line);
        std::string currencyToken;
        std::string amountToken;
        if (!std::getline(stream, currencyToken, ',')) {
            continue;
        }
        if (!std::getline(stream, amountToken)) {
            continue;
        }
        try {
            Currency currency = parseCurrency(currencyToken);
            double amount = std::stod(amountToken);
            minima[currency] = amount;
        } catch (...) {
            // Ignore malformed entries
        }
    }
    return minima;
}

void DataStore::saveCriticalMinimums(const std::map<Currency, double>& minima) const {
    std::ofstream output(criticalFile(), std::ios::trunc);
    for (const auto& [currency, amount] : minima) {
        output << to_string(currency) << ',' << std::fixed << std::setprecision(2) << amount << '\n';
    }
}

int DataStore::ensurePersonId(const std::string& role, const std::string& name) {
    std::string trimmedName = name;
    trimmedName.erase(trimmedName.begin(), std::find_if(trimmedName.begin(), trimmedName.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    trimmedName.erase(std::find_if(trimmedName.rbegin(), trimmedName.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), trimmedName.end());

    std::string key = canonicalKey(role, trimmedName);
    auto iterator = people.find(key);
    if (iterator != people.end()) {
        return iterator->second.id;
    }

    PersonEntry entry{nextPersonId++, role, trimmedName};
    people[key] = entry;
    persistPeople();
    return entry.id;
}

void DataStore::appendTransaction(const Receipt& receipt) const {
    std::ofstream output(transactionsFile(), std::ios::app);
    output << receipt.timestamp() << '|'
           << receipt.id() << '|'
           << receipt.cashierIdentifier() << '|'
           << receipt.cashier() << '|'
           << receipt.clientIdentifier() << '|'
           << receipt.client() << '|'
           << to_string(receipt.source()) << '|'
           << std::fixed << std::setprecision(2) << receipt.sourceAmountValue() << '|'
           << std::fixed << std::setprecision(2) << receipt.profitInBase() << '|'
           << std::fixed << std::setprecision(2) << receipt.commissionInBase() << '\n';
}

std::filesystem::path DataStore::persistReport(const DailyReport& report, const Manager& manager) const {
    std::time_t timestamp = report.generatedOn();
    std::tm* timeInfo = std::localtime(&timestamp);
    char buffer[32];
    if (timeInfo != nullptr) {
        std::strftime(buffer, sizeof(buffer), "report-%Y%m%d-%H%M%S.txt", timeInfo);
    } else {
        std::snprintf(buffer, sizeof(buffer), "report-%lld.txt", static_cast<long long>(timestamp));
    }
    auto path = reportsDirectory / buffer;
    std::ofstream output(path, std::ios::trunc);
    output << "Manager: " << manager.getName() << " (ID " << manager.getId() << ")\n";
    std::tm* reportInfo = std::localtime(&timestamp);
    if (reportInfo) {
        output << "Generated: " << std::put_time(reportInfo, "%Y-%m-%d %H:%M:%S") << '\n';
    } else {
        output << "Generated (timestamp): " << static_cast<long long>(timestamp) << '\n';
    }
    output << "Profit (base currency): " << std::fixed << std::setprecision(2) << report.profitInBase() << "\n\n";
    output << "Ending reserves:\n";
    for (const auto& [currency, balance] : report.endBalances()) {
        output << "  " << to_string(currency) << ": " << std::fixed << std::setprecision(2) << balance;
        auto thresholdIt = report.criticalThresholds().find(currency);
        if (thresholdIt != report.criticalThresholds().end()) {
            output << " (critical min " << std::fixed << std::setprecision(2) << thresholdIt->second << ")";
        }
        output << '\n';
    }
    output << "\nTransactions:\n";
    for (const auto& record : report.history()) {
        output << "  Receipt #" << record.receiptId << " | Cashier "
               << record.cashierName << " (ID " << record.cashierId << ") | Client "
               << record.clientName << " (ID " << record.clientId << ") | Source "
               << to_string(record.sourceCurrency) << ' ' << std::fixed << std::setprecision(2) << record.sourceAmount
               << " | Profit base " << std::fixed << std::setprecision(2) << record.profitInBaseCurrency << '\n';
    }
    return path;
}
