#pragma once

#include "connection.hpp"
#include <functional>
#include <string>

namespace netp {

class Server {
public:
    using ConnectionHandler = std::function<void(ConnectionPtr)>;
    using ErrorHandler = std::function<void(const std::string&)>;

    virtual ~Server() = default;

    // Start the server
    virtual bool start(uint16_t port) = 0;

    // Stop the server
    virtual void stop() = 0;

    // Set handlers
    virtual void setConnectionHandler(ConnectionHandler handler) = 0;
    virtual void setErrorHandler(ErrorHandler handler) = 0;

    // Server status
    virtual bool isRunning() const = 0;
    virtual uint16_t getPort() const = 0;
};

// Factory function to create a server instance
std::unique_ptr<Server> createServer();

} // namespace netp 