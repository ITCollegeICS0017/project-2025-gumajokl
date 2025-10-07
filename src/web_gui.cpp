#include "web_gui.h"

#include "client.h"
#include "employee.h"
#include "utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {
    struct HttpRequest {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> query;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        std::unordered_map<std::string, std::string> form;
    };

    std::string htmlEscape(const std::string& input) {
        std::string escaped;
        escaped.reserve(input.size());
        for (char ch : input) {
            switch (ch) {
                case '&': escaped.append("&amp;"); break;
                case '<': escaped.append("&lt;"); break;
                case '>': escaped.append("&gt;"); break;
                case '"': escaped.append("&quot;"); break;
                case '\'': escaped.append("&#39;"); break;
                default: escaped.push_back(ch); break;
            }
        }
        return escaped;
    }

    std::string trim(const std::string& value) {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    std::string toUpper(const std::string& value) {
        std::string result = value;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        });
        return result;
    }

    std::string urlDecode(const std::string& encoded) {
        std::string result;
        result.reserve(encoded.size());
        for (std::size_t i = 0; i < encoded.size(); ++i) {
            char ch = encoded[i];
            if (ch == '+') {
                result.push_back(' ');
            } else if (ch == '%' && i + 2 < encoded.size()) {
                char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
                char decoded = static_cast<char>(std::strtol(hex, nullptr, 16));
                result.push_back(decoded);
                i += 2;
            } else {
                result.push_back(ch);
            }
        }
        return result;
    }

    std::unordered_map<std::string, std::string> parseFormEncoded(const std::string& data) {
        std::unordered_map<std::string, std::string> result;
        std::size_t start = 0;
        while (start < data.size()) {
            std::size_t end = data.find('&', start);
            if (end == std::string::npos) {
                end = data.size();
            }
            std::string pair = data.substr(start, end - start);
            std::size_t eq = pair.find('=');
            std::string key = eq == std::string::npos ? pair : pair.substr(0, eq);
            std::string value = eq == std::string::npos ? "" : pair.substr(eq + 1);
            result[urlDecode(key)] = urlDecode(value);
            start = end + 1;
        }
        return result;
    }

    std::vector<int> parseDenominations(const std::string& text) {
        std::vector<int> denominations;
        std::istringstream stream(text);
        int value = 0;
        while (stream >> value) {
            if (value > 0) {
                denominations.push_back(value);
            }
        }
        return denominations;
    }

    std::string joinDenominations(const std::vector<int>& denominations) {
        if (denominations.empty()) {
            return "-";
        }
        std::ostringstream stream;
        for (std::size_t i = 0; i < denominations.size(); ++i) {
            if (i > 0) {
                stream << ", ";
            }
            stream << denominations[i];
        }
        return stream.str();
    }

    double parseDoubleField(const std::unordered_map<std::string, std::string>& form,
                            const std::string& key,
                            bool required,
                            double defaultValue = 0.0) {
        auto iterator = form.find(key);
        if (iterator == form.end() || iterator->second.empty()) {
            if (required) {
                throw ExchangeError("Missing required field: " + key);
            }
            return defaultValue;
        }
        try {
            return std::stod(iterator->second);
        } catch (...) {
            throw ExchangeError("Invalid numeric value for field: " + key);
        }
    }

    std::string getField(const std::unordered_map<std::string, std::string>& form,
                         const std::string& key,
                         bool required) {
        auto iterator = form.find(key);
        if (iterator == form.end() || iterator->second.empty()) {
            if (required) {
                throw ExchangeError("Missing required field: " + key);
            }
            return "";
        }
        return iterator->second;
    }

    Currency parseCurrencyField(const std::unordered_map<std::string, std::string>& form,
                                const std::string& key) {
        std::string value = getField(form, key, true);
        return currency_from_string(value);
    }

    std::string renderCurrencySelect(const std::string& name, const std::string& selected = "") {
        static const std::vector<std::string> options = {"USD", "EUR", "GBP", "LOCAL"};
        std::ostringstream stream;
        stream << "<select name=\"" << name << "\">";
        for (const auto& option : options) {
            stream << "<option value=\"" << option << "\"";
            if (!selected.empty() && toUpper(option) == toUpper(selected)) {
                stream << " selected";
            }
            stream << ">" << option << "</option>";
        }
        stream << "</select>";
        return stream.str();
    }

    std::string makeResponse(int statusCode, const std::string& statusText, const std::string& body) {
        std::ostringstream stream;
        stream << "HTTP/1.1 " << statusCode << ' ' << statusText << "\r\n";
        stream << "Content-Type: text/html; charset=utf-8\r\n";
        stream << "Content-Length: " << body.size() << "\r\n";
        stream << "Connection: close\r\n\r\n";
        stream << body;
        return stream.str();
    }

    std::string makeErrorResponse(const std::string& message) {
        std::ostringstream body;
        body << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Error</title></head><body>";
        body << "<h1>Error</h1><p>" << htmlEscape(message) << "</p>";
        body << "<p><a href=\"/\">Return to application</a></p></body></html>";
        return makeResponse(500, "Internal Server Error", body.str());
    }
}

WebGuiServer::WebGuiServer(ExchangeOffice& officeRef, DataStore& dataStore, int listenPort)
    : office(officeRef),
      store(dataStore),
      port(listenPort),
      running(false) {}

namespace {
    std::string renderMainPage(ExchangeOffice& office, const std::string& message) {
        DailyReport report = office.compileDailyReport();
        std::ostringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
        html << "<title>Currency Exchange Office</title>";
        html << "<style>body{font-family:sans-serif;margin:24px;background:#f5f5f5;}"
                "header{margin-bottom:20px;}section{background:#fff;padding:16px;margin-bottom:20px;border-radius:8px;"
                "box-shadow:0 1px 3px rgba(0,0,0,0.1);}table{border-collapse:collapse;width:100%;}"
                "th,td{border:1px solid #ddd;padding:8px;text-align:left;}"
                "th{background:#333;color:#fff;} .status{padding:12px;border-radius:6px;margin-bottom:16px;}"
                ".status.ok{background:#e0f6e9;border:1px solid #9bd3aa;}"
                ".status.error{background:#fdecea;border:1px solid #f5a097;}"
                "form > div{margin-bottom:12px;}"
                "label{display:block;margin-bottom:4px;font-weight:bold;}"
                "input[type=text],input[type=number]{width:100%;padding:8px;box-sizing:border-box;}"
                "button{padding:8px 16px;border:none;border-radius:4px;background:#333;color:#fff;cursor:pointer;}"
                "button:hover{background:#111;}details summary{cursor:pointer;font-weight:bold;}"
                "</style></head><body>";
        html << "<header><h1>Currency Exchange Office</h1>"
             << "<p>Web interface running locally. Open multiple tabs to simulate different employees.</p></header>";

        if (!message.empty()) {
            bool error = message.rfind("Error:", 0) == 0;
            html << "<div class=\"status " << (error ? "error" : "ok") << "\">"
                 << htmlEscape(message) << "</div>";
        }

        html << "<section><h2>Reserve Balances</h2><table><thead><tr><th>Currency</th><th>Balance</th>"
                "<th>Critical Minimum</th><th>Status</th></tr></thead><tbody>";
        for (const auto& entry : office.reserve().allBalances()) {
            Currency currency = entry.first;
            double balance = entry.second;
            double minimum = office.criticalMinimum(currency);
            bool below = office.isBelowCritical(currency);
            html << "<tr><td>" << htmlEscape(to_string(currency)) << "</td><td>"
                 << std::fixed << std::setprecision(2) << balance << "</td><td>"
                 << std::fixed << std::setprecision(2) << minimum << "</td><td>"
                 << (below ? "<strong style=\"color:#b00020;\">Below minimum</strong>" : "OK")
                 << "</td></tr>";
        }
        html << "</tbody></table></section>";

        html << "<section><h2>Exchange Rates (relative to one unit of source currency)</h2><table>"
                "<thead><tr><th>From</th><th>To</th><th>Rate</th></tr></thead><tbody>";
        for (const auto& entry : office.rateConfig().serialize()) {
            Currency from;
            Currency to;
            double rate;
            std::tie(from, to, rate) = entry;
            html << "<tr><td>" << htmlEscape(to_string(from)) << "</td><td>"
                 << htmlEscape(to_string(to)) << "</td><td>"
                 << std::fixed << std::setprecision(6) << rate << "</td></tr>";
        }
        html << "</tbody></table></section>";

        html << "<section><h2>Cashier Exchange</h2>"
                "<form method=\"POST\" action=\"/exchange\">"
                "<div><label>Cashier name</label><input type=\"text\" name=\"cashier_name\" required></div>"
                "<div><label>Client name</label><input type=\"text\" name=\"client_name\" required></div>"
                "<div><label>Source currency</label>" << renderCurrencySelect("source_currency") << "</div>"
                "<div><label>Amount in source currency</label><input type=\"number\" step=\"0.01\" min=\"0.01\" name=\"amount\" required></div>"
                "<div><label>Primary payout currency</label>" << renderCurrencySelect("target_currency") << "</div>"
                "<div><label>Primary payout amount in source currency (leave blank to use remainder)</label>"
                "<input type=\"number\" step=\"0.01\" min=\"0\" name=\"target_amount\"></div>"
                "<div><label>Preferred denominations for primary payout (space separated, optional)</label>"
                "<input type=\"text\" name=\"target_denominations\" placeholder=\"50 20 10\"></div>"
                "<details><summary>Optional second payout portion</summary>"
                "<div><label>Secondary payout currency</label>" << renderCurrencySelect("target_currency2") << "</div>"
                "<div><label>Amount in source currency for secondary payout (leave blank to use remainder)</label>"
                "<input type=\"number\" step=\"0.01\" min=\"0\" name=\"target_amount2\"></div>"
                "<div><label>Preferred denominations for secondary payout</label>"
                "<input type=\"text\" name=\"target_denominations2\" placeholder=\"20 10\"></div>"
                "</details>"
                "<button type=\"submit\">Process exchange</button></form></section>";

        html << "<section><h2>Manager Actions</h2>"
                "<details open><summary>Generate daily report</summary>"
                "<form method=\"POST\" action=\"/manager/report\">"
                "<div><label>Manager name</label><input type=\"text\" name=\"manager_name\" required></div>"
                "<button type=\"submit\">Create report</button>"
                "</form></details>"
                "<details><summary>Adjust exchange rate</summary>"
                "<form method=\"POST\" action=\"/manager/rate\">"
                "<div><label>Manager name</label><input type=\"text\" name=\"manager_name\" required></div>"
                "<div><label>From currency</label>" << renderCurrencySelect("from_currency") << "</div>"
                "<div><label>To currency</label>" << renderCurrencySelect("to_currency") << "</div>"
                "<div><label>New rate</label><input type=\"number\" step=\"0.0001\" min=\"0.0001\" name=\"rate\" required></div>"
                "<button type=\"submit\">Update rate</button>"
                "</form></details>"
                "<details><summary>Set critical reserve level</summary>"
                "<form method=\"POST\" action=\"/manager/critical\">"
                "<div><label>Manager name</label><input type=\"text\" name=\"manager_name\" required></div>"
                "<div><label>Currency</label>" << renderCurrencySelect("currency") << "</div>"
                "<div><label>Critical minimum amount</label><input type=\"number\" step=\"0.01\" min=\"0\" name=\"amount\" required></div>"
                "<button type=\"submit\">Save threshold</button>"
                "</form></details>"
                "<details><summary>Reset daily cycle</summary>"
                "<form method=\"POST\" action=\"/manager/reset\">"
                "<div><label>Manager name</label><input type=\"text\" name=\"manager_name\" required></div>"
                "<button type=\"submit\">Reset cycle</button>"
                "</form></details>"
                "<details><summary>Top up reserve</summary>"
                "<form method=\"POST\" action=\"/manager/topup\">"
                "<div><label>Manager name</label><input type=\"text\" name=\"manager_name\" required></div>"
                "<div><label>Currency</label>" << renderCurrencySelect("currency") << "</div>"
                "<div><label>Amount to add</label><input type=\"number\" step=\"0.01\" min=\"0.01\" name=\"amount\" required></div>"
                "<button type=\"submit\">Add funds</button>"
                "</form></details>"
                "</section>";

        html << "<section><h2>Recent Transactions</h2>";
        const auto& history = report.history();
        if (history.empty()) {
            html << "<p>No transactions recorded for today.</p>";
        } else {
            html << "<p>Total profit today: " << std::fixed << std::setprecision(2) << report.profitInBase()
                 << " (base currency)</p>";
            html << "<table><thead><tr><th>Receipt</th><th>Cashier</th><th>Client</th>"
                    "<th>Source</th><th>Payouts</th><th>Profit (base)</th><th>Timestamp</th></tr></thead><tbody>";
            std::size_t displayCount = std::min<std::size_t>(history.size(), 10);
            for (std::size_t idx = history.size() - displayCount; idx < history.size(); ++idx) {
                const auto& record = history[idx];
                std::ostringstream payouts;
                for (std::size_t i = 0; i < record.payouts.size(); ++i) {
                    const auto& payout = record.payouts[i];
                    if (i > 0) {
                        payouts << "<br>";
                    }
                    payouts << htmlEscape(to_string(payout.currency)) << ": "
                            << std::fixed << std::setprecision(2) << payout.amountPaid
                            << " (commission " << std::fixed << std::setprecision(2) << payout.commissionTaken
                            << ", notes " << htmlEscape(joinDenominations(payout.denominations)) << ")";
                }
                std::tm* timestamp = std::localtime(&record.timestamp);
                std::ostringstream timeBuilder;
                if (timestamp != nullptr) {
                    timeBuilder << std::put_time(timestamp, "%Y-%m-%d %H:%M:%S");
                } else {
                    timeBuilder << record.timestamp;
                }

                html << "<tr><td>#"<< record.receiptId << "</td><td>"
                     << htmlEscape(record.cashierName) << " (ID " << record.cashierId << ")</td><td>"
                     << htmlEscape(record.clientName) << " (ID " << record.clientId << ")</td><td>"
                     << htmlEscape(to_string(record.sourceCurrency)) << " "
                     << std::fixed << std::setprecision(2) << record.sourceAmount << "</td><td>"
                     << payouts.str() << "</td><td>"
                     << std::fixed << std::setprecision(2) << record.profitInBaseCurrency << "</td><td>"
                     << timeBuilder.str() << "</td></tr>";
            }
            html << "</tbody></table>";
        }
        html << "</section>";

        html << "<footer><p>Tip: open another tab to act as a manager while keeping an employee session active.</p></footer>";

        html << "</body></html>";
        return html.str();
    }

    HttpRequest parseHttpRequest(const std::string& rawRequest) {
        HttpRequest request;

        std::size_t headerEndPos = rawRequest.find("\r\n\r\n");
        if (headerEndPos == std::string::npos) {
            throw ExchangeError("Invalid HTTP request: header terminator not found");
        }

        std::string headerSection = rawRequest.substr(0, headerEndPos);
        std::istringstream headerStream(headerSection);

        std::string requestLine;
        if (!std::getline(headerStream, requestLine)) {
            throw ExchangeError("Invalid HTTP request: empty request line");
        }
        if (!requestLine.empty() && requestLine.back() == '\r') {
            requestLine.pop_back();
        }
        std::istringstream requestLineStream(requestLine);
        std::string target;
        requestLineStream >> request.method >> target;

        std::size_t querySeparator = target.find('?');
        if (querySeparator != std::string::npos) {
            request.path = target.substr(0, querySeparator);
            request.query = parseFormEncoded(target.substr(querySeparator + 1));
        } else {
            request.path = target;
        }

        std::string headerLine;
        while (std::getline(headerStream, headerLine)) {
            if (!headerLine.empty() && headerLine.back() == '\r') {
                headerLine.pop_back();
            }
            if (headerLine.empty()) {
                continue;
            }
            std::size_t colon = headerLine.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            std::string key = headerLine.substr(0, colon);
            std::string value = trim(headerLine.substr(colon + 1));
            request.headers[toUpper(key)] = value;
        }

        request.body = rawRequest.substr(headerEndPos + 4);
        std::string contentType;
        auto ctIterator = request.headers.find("CONTENT-TYPE");
        if (ctIterator != request.headers.end()) {
            contentType = toUpper(ctIterator->second);
        }
        if (request.method == "POST" && contentType.find("APPLICATION/X-WWW-FORM-URLENCODED") != std::string::npos) {
            request.form = parseFormEncoded(request.body);
        }
        return request;
    }

    std::string readHttpRequest(int clientFd) {
        std::string data;
        data.reserve(4096);
        char buffer[4096];
        bool headersComplete = false;
        std::size_t contentLength = 0;

        while (true) {
            ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) {
                break;
            }
            data.append(buffer, static_cast<std::size_t>(bytesRead));

            if (!headersComplete) {
                std::size_t headerEnd = data.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    headersComplete = true;
                    std::string headerPart = data.substr(0, headerEnd);
                    std::istringstream headerStream(headerPart);
                    std::string line;
                    std::getline(headerStream, line); // skip request line
                    while (std::getline(headerStream, line)) {
                        if (!line.empty() && line.back() == '\r') {
                            line.pop_back();
                        }
                        if (line.empty()) {
                            continue;
                        }
                        std::size_t colon = line.find(':');
                        if (colon == std::string::npos) {
                            continue;
                        }
                        std::string key = line.substr(0, colon);
                        std::string value = trim(line.substr(colon + 1));
                        if (toUpper(key) == "CONTENT-LENGTH") {
                            try {
                                contentLength = static_cast<std::size_t>(std::stoul(value));
                            } catch (...) {
                                throw ExchangeError("Invalid Content-Length header");
                            }
                        }
                    }
                    if (contentLength == 0) {
                        break;
                    }
                }
            }

            if (headersComplete) {
                std::size_t headerEnd = data.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    std::size_t totalLength = headerEnd + 4 + contentLength;
                    if (data.size() >= totalLength) {
                        break;
                    }
                }
            }

            if (data.size() > 1'000'000) {
                throw ExchangeError("HTTP request too large");
            }
        }

        return data;
    }

    std::string handleExchange(ExchangeOffice& office,
                               DataStore& store,
                               const std::unordered_map<std::string, std::string>& form) {
        std::string cashierName = trim(getField(form, "cashier_name", true));
        std::string clientName = trim(getField(form, "client_name", true));
        if (cashierName.empty() || clientName.empty()) {
            throw ExchangeError("Names cannot be empty");
        }

        int cashierId = store.ensurePersonId("cashier", cashierName);
        Cashier cashier(cashierId, cashierName, office);

        int clientId = store.ensurePersonId("client", clientName);
        Client client(clientId, clientName);

        Currency sourceCurrency = parseCurrencyField(form, "source_currency");
        double totalAmount = parseDoubleField(form, "amount", true);
        if (totalAmount <= 0.0) {
            throw ExchangeError("Amount must be greater than zero");
        }

        std::vector<ExchangePortion> portions;

        Currency primaryTarget = parseCurrencyField(form, "target_currency");
        std::string primaryAmountText = getField(form, "target_amount", false);
        ExchangePortion primaryPortion = primaryAmountText.empty()
            ? ExchangePortion::remainder(primaryTarget)
            : ExchangePortion(primaryTarget, parseDoubleField(form, "target_amount", false));
        if (!primaryAmountText.empty() && primaryPortion.sourceAmount <= 0.0) {
            throw ExchangeError("Primary portion amount must be greater than zero");
        }
        primaryPortion.denominations = parseDenominations(getField(form, "target_denominations", false));
        portions.push_back(primaryPortion);

        std::string secondaryCurrencyText = getField(form, "target_currency2", false);
        if (!secondaryCurrencyText.empty()) {
            Currency secondaryTarget = currency_from_string(secondaryCurrencyText);
            std::string secondaryAmountText = getField(form, "target_amount2", false);
            ExchangePortion secondaryPortion = secondaryAmountText.empty()
                ? ExchangePortion::remainder(secondaryTarget)
                : ExchangePortion(secondaryTarget, parseDoubleField(form, "target_amount2", false));
            if (!secondaryAmountText.empty() && secondaryPortion.sourceAmount <= 0.0) {
                throw ExchangeError("Secondary portion amount must be greater than zero");
            }
            secondaryPortion.denominations = parseDenominations(getField(form, "target_denominations2", false));
            portions.push_back(secondaryPortion);
        }

        ExchangeRequest request(client.id(), client.name(), sourceCurrency, totalAmount, portions);
        Receipt receipt = cashier.handleRequest(request);
        store.appendTransaction(receipt);
        store.saveReserve(office.reserve().allBalances());

        std::ostringstream message;
        message << "Exchange completed. Receipt #" << receipt.id()
                << " for client " << client.name() << " (ID " << client.id() << ")";
        return message.str();
    }

    std::string handleReport(ExchangeOffice& office,
                             DataStore& store,
                             const std::unordered_map<std::string, std::string>& form) {
        std::string managerName = trim(getField(form, "manager_name", true));
        if (managerName.empty()) {
            throw ExchangeError("Manager name cannot be empty");
        }
        int managerId = store.ensurePersonId("manager", managerName);
        Manager manager(managerId, managerName, office);
        DailyReport report = manager.compileDailyReport();
        auto path = store.persistReport(report, manager);
        double bonus = manager.calculateBonus(report.profitInBase());

        std::ostringstream message;
        message << "Report generated and saved to " << path
                << ". Profit: " << std::fixed << std::setprecision(2) << report.profitInBase()
                << ". Suggested bonus (5%): " << std::fixed << std::setprecision(2) << bonus;
        return message.str();
    }

    std::string handleRateChange(ExchangeOffice& office,
                                 DataStore& store,
                                 const std::unordered_map<std::string, std::string>& form) {
        std::string managerName = trim(getField(form, "manager_name", true));
        int managerId = store.ensurePersonId("manager", managerName);
        Manager manager(managerId, managerName, office);
        Currency from = parseCurrencyField(form, "from_currency");
        Currency to = parseCurrencyField(form, "to_currency");
        double rate = parseDoubleField(form, "rate", true);
        manager.setExchangeRate(from, to, rate);
        store.saveRates(office.rateConfig());
        std::ostringstream message;
        message << "Exchange rate updated: 1 " << to_string(from) << " = "
                << std::fixed << std::setprecision(6) << rate << " " << to_string(to);
        return message.str();
    }

    std::string handleCriticalMinimum(ExchangeOffice& office,
                                      DataStore& store,
                                      const std::unordered_map<std::string, std::string>& form) {
        std::string managerName = trim(getField(form, "manager_name", true));
        int managerId = store.ensurePersonId("manager", managerName);
        Manager manager(managerId, managerName, office);
        Currency currency = parseCurrencyField(form, "currency");
        double amount = parseDoubleField(form, "amount", true);
        manager.setCriticalReserve(currency, amount);
        store.saveCriticalMinimums(office.criticalMinimumsMap());
        std::ostringstream message;
        message << "Critical reserve for " << to_string(currency) << " set to "
                << std::fixed << std::setprecision(2) << amount;
        return message.str();
    }

    std::string handleResetCycle(ExchangeOffice& office,
                                 DataStore& store,
                                 const std::unordered_map<std::string, std::string>& form) {
        std::string managerName = trim(getField(form, "manager_name", true));
        int managerId = store.ensurePersonId("manager", managerName);
        Manager manager(managerId, managerName, office);
        office.resetDailyCycle();
        store.saveReserve(office.reserve().allBalances());
        store.saveCriticalMinimums(office.criticalMinimumsMap());
        store.saveRates(office.rateConfig());
        return "Daily cycle reset. Starting balances updated.";
    }

    std::string handleTopUp(ExchangeOffice& office,
                             DataStore& store,
                             const std::unordered_map<std::string, std::string>& form) {
        std::string managerName = trim(getField(form, "manager_name", true));
        int managerId = store.ensurePersonId("manager", managerName);
        Manager manager(managerId, managerName, office);
        Currency currency = parseCurrencyField(form, "currency");
        double amount = parseDoubleField(form, "amount", true);
        if (amount <= 0.0) {
            throw ExchangeError("Top-up amount must be positive");
        }
        manager.topUpReserve(currency, amount);
        store.saveReserve(office.reserve().allBalances());
        std::ostringstream message;
        message << "Added " << std::fixed << std::setprecision(2) << amount
                << " to reserve of " << to_string(currency);
        return message.str();
    }
}

void WebGuiServer::start() {
    std::signal(SIGPIPE, SIG_IGN);
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw ExchangeError("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverFd);
        throw ExchangeError("Failed to set socket options");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(serverFd);
        throw ExchangeError("Failed to bind socket to port " + std::to_string(port));
    }

    if (listen(serverFd, 16) < 0) {
        close(serverFd);
        throw ExchangeError("Failed to listen on port");
    }

    std::cout << "Web GUI running. Open http://127.0.0.1:" << port << "/ in your browser." << std::endl;
    std::cout << "Press Ctrl+C to stop the server." << std::endl;

    running = true;
    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        try {
            std::string rawRequest = readHttpRequest(clientFd);
            if (!rawRequest.empty()) {
                HttpRequest request = parseHttpRequest(rawRequest);
                std::string body;
                try {
                    if (request.method == "GET" && request.path == "/") {
                        body = makeResponse(200, "OK", renderMainPage(office, ""));
                    } else if (request.method == "POST" && request.path == "/exchange") {
                        std::string message = handleExchange(office, store, request.form);
                        body = makeResponse(200, "OK", renderMainPage(office, message));
                    } else if (request.method == "POST" && request.path == "/manager/report") {
                        std::string message = handleReport(office, store, request.form);
                        body = makeResponse(200, "OK", renderMainPage(office, message));
                    } else if (request.method == "POST" && request.path == "/manager/rate") {
                        std::string message = handleRateChange(office, store, request.form);
                        body = makeResponse(200, "OK", renderMainPage(office, message));
                    } else if (request.method == "POST" && request.path == "/manager/critical") {
                        std::string message = handleCriticalMinimum(office, store, request.form);
                        body = makeResponse(200, "OK", renderMainPage(office, message));
                    } else if (request.method == "POST" && request.path == "/manager/reset") {
                        std::string message = handleResetCycle(office, store, request.form);
                        body = makeResponse(200, "OK", renderMainPage(office, message));
                    } else if (request.method == "POST" && request.path == "/manager/topup") {
                        std::string message = handleTopUp(office, store, request.form);
                        body = makeResponse(200, "OK", renderMainPage(office, message));
                    } else {
                        body = makeResponse(404, "Not Found",
                                            "<!DOCTYPE html><html><body><h1>404 Not Found</h1>"
                                            "<p>Unknown path.</p><p><a href=\"/\">Return home</a></p></body></html>");
                    }
                } catch (const std::exception& error) {
                    std::string message = std::string("Error: ") + error.what();
                    body = makeResponse(200, "OK", renderMainPage(office, message));
                }
                send(clientFd, body.c_str(), body.size(), 0);
            }
        } catch (const std::exception& error) {
            std::string response = makeErrorResponse(error.what());
            send(clientFd, response.c_str(), response.size(), 0);
        }

        close(clientFd);
    }

    close(serverFd);
}
