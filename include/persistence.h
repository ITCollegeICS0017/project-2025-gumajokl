#pragma once

#include "employee.h"
#include "exchange_manager.h"

#include <filesystem>
#include <map>
#include <string>
#include <tuple>
#include <vector>

struct PersonEntry {
    int id;
    std::string role;
    std::string name;
};

class DataStore {
private:
    std::filesystem::path baseDirectory;
    std::filesystem::path reportsDirectory;

    std::map<std::string, PersonEntry> people;
    int nextPersonId;

    std::filesystem::path reserveFile() const;
    std::filesystem::path ratesFile() const;
    std::filesystem::path criticalFile() const;
    std::filesystem::path peopleFile() const;
    std::filesystem::path transactionsFile() const;

    void loadPeople();
    void persistPeople() const;

public:
    explicit DataStore(const std::string& baseDir = "data");

    void initialize();

    std::map<Currency, double> loadReserve(const std::map<Currency, double>& defaults) const;
    void saveReserve(const std::map<Currency, double>& balances) const;

    std::vector<std::tuple<Currency, Currency, double>> loadRates() const;
    void saveRates(const RateTable& table) const;

    std::map<Currency, double> loadCriticalMinimums() const;
    void saveCriticalMinimums(const std::map<Currency, double>& minima) const;

    int ensurePersonId(const std::string& role, const std::string& name);

    void appendTransaction(const Receipt& receipt) const;
    std::filesystem::path persistReport(const DailyReport& report, const Manager& manager) const;
};
