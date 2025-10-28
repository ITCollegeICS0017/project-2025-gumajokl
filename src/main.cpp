#include "console_ui.h"
#include "data_storage.h"
#include "exchange_manager.h"




int main(int argc, char const *argv[])
{
    DataStore dataStore;
    ExchangeOffice exchangeOffice;
    

    ConsoleUI ui(dataStore, exchangeOffice);

    ui.mainMenu();

    return 0;
}
