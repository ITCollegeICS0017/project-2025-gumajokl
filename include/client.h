#pragma once

#include <string>

class Client{
    private:
        int _clientID;
        std::string _clientName;

    public:
        Client(int clientID, std::string clientName);

        int get_id() const;
        const std::string& get_name() const;
};