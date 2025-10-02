#include "employee.h"  // just the header is needed for compilation
#include <iostream>
using namespace std;

int main() {
    Employee emp("Alice");
    cout << "Name: " << emp.getName() << endl;
    return 0;
}
