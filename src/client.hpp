#pragma once

#include "connection.hpp"
#include <string>
#include <memory>

namespace netp {

class Client {
public:
    virtual ~Client() = default;

    // Connect to server
    virtual bool connect(const std::string& host, uint16_t port) = 0;

    // Disconnect from server
    virtual void disconnect() = 0;

    // Get the connection object
    virtual ConnectionPtr getConnection() = 0;

    // Client status
    virtual bool isConnected() const = 0;
};

// Factory function to create a client instance
std::unique_ptr<Client> createClient();

} // namespace netp 