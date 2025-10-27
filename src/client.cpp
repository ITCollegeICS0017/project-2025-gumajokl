
#include "client.h"

Client::Client(int clientID, std::string clientName) : _clientID(clientID), _clientName(clientName) {}

int Client::get_id() const { return _clientID; }
const std::string& Client::get_name() const { return _clientName; }