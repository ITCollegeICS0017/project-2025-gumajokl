#include "employee.h"

Employee::Employee(int employeeId, std::string employeeName) : id(employeeId), name(std::move(employeeName)) {}

int Employee::get_id() const { return id; }
const std::string &Employee::get_name() const { return name; }




Cashier::Cashier(int cashierId, std::string cashierName) : Employee(cashierId, std::move(cashierName)) {}


Manager::Manager(int managerId, std::string managerName) : Employee(managerId, std::move(managerName)) {}




