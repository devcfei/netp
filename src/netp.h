#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace netp {

// Forward declarations
class Connection;
class Server;
class Client;
using ConnectionPtr = std::shared_ptr<Connection>;

//
// Packet System
//

// Packet header with 32-bit length field
struct PacketHeader {
    uint32_t data_length;  // Network byte order
};

// Base packet interface
class Packet {
public:
    virtual ~Packet() = default;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual bool deserialize(const uint8_t* data, size_t length) = 0;
    virtual size_t getDataSize() const = 0;
};

// Helper template for packet implementation
template<typename T>
class BasicPacket : public Packet {
public:
    std::vector<uint8_t> serialize() const override {
        return static_cast<const T*>(this)->serializeImpl();
    }
    
    bool deserialize(const uint8_t* data, size_t length) override {
        return static_cast<T*>(this)->deserializeImpl(data, length);
    }
    
    size_t getDataSize() const override {
        return static_cast<const T*>(this)->getDataSizeImpl();
    }
};

//
// Connection Interface
//

class Connection {
public:
    // Handler types
    using PacketHandler = std::function<void(const std::vector<uint8_t>&)>;
    using ErrorHandler = std::function<void(const std::string&)>;
    using DisconnectHandler = std::function<void()>;

    virtual ~Connection() = default;

    // Packet operations
    virtual bool sendPacket(const Packet& packet) = 0;
    virtual bool sendRawData(const std::vector<uint8_t>& data) = 0;

    // Event handlers
    virtual void setPacketHandler(PacketHandler handler) = 0;
    virtual void setErrorHandler(ErrorHandler handler) = 0;
    virtual void setDisconnectHandler(DisconnectHandler handler) = 0;

    // Connection management
    virtual bool isConnected() const = 0;
    virtual void disconnect() = 0;

    // Connection info
    virtual std::string getRemoteAddress() const = 0;
    virtual uint16_t getRemotePort() const = 0;
};

//
// Server Interface
//

class Server {
public:
    // Handler types
    using ConnectionHandler = std::function<void(ConnectionPtr)>;
    using ErrorHandler = std::function<void(const std::string&)>;

    virtual ~Server() = default;

    // Server operations
    virtual bool start(uint16_t port) = 0;
    virtual void stop() = 0;

    // Event handlers
    virtual void setConnectionHandler(ConnectionHandler handler) = 0;
    virtual void setErrorHandler(ErrorHandler handler) = 0;

    // Server status
    virtual bool isRunning() const = 0;
    virtual uint16_t getPort() const = 0;
};

//
// Client Interface
//

class Client {
public:
    virtual ~Client() = default;

    // Client operations
    virtual bool connect(const std::string& host, uint16_t port) = 0;
    virtual void disconnect() = 0;

    // Connection access
    virtual ConnectionPtr getConnection() = 0;

    // Client status
    virtual bool isConnected() const = 0;
};

//
// Factory Functions
//

// Create a new server instance
std::unique_ptr<Server> createServer();

// Create a new client instance
std::unique_ptr<Client> createClient();

} // namespace netp