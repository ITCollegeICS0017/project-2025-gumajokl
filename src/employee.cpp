#include "employee.h"

Employee::Employee(int employeeId, std::string employeeName, ExchangeOffice& exchangeOffice_in) : id(employeeId), name(std::move(employeeName)), exchangeOffice(exchangeOffice_in){}

int Employee::get_id() const { return id; }
const std::string &Employee::get_name() const { return name; }


Cashier::Cashier(int cashierId, std::string cashierName) : Employee(cashierId, std::move(cashierName)) {
    role = PersonRole::CASHIER;
}

Cashier::perform_exchange(const ExchangeRequest& request) {
    return exchangeOffice.process_exchange(request);
}


Manager::Manager(int managerId, std::string managerName) : Employee(managerId, std::move(managerName)) {
    role = PersonRole::MANAGER;
}




