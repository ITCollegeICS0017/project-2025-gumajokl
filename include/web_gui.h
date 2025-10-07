#pragma once

#include "exchange_manager.h"
#include "persistence.h"

#include <string>

class WebGuiServer {
private:
    ExchangeOffice& office;
    DataStore& store;
    int port;
    bool running;

public:
    WebGuiServer(ExchangeOffice& office, DataStore& store, int port = 8080);

    void start();
};
