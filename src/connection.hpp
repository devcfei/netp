#pragma once

#include "packet.hpp"
#include <functional>
#include <string>
#include <memory>

namespace netp {

class Connection {
public:
    using PacketHandler = std::function<void(const std::vector<uint8_t>&)>;
    using ErrorHandler = std::function<void(const std::string&)>;
    using DisconnectHandler = std::function<void()>;

    virtual ~Connection() = default;

    // Send a packet
    virtual bool sendPacket(const Packet& packet) = 0;

    // Send raw data
    virtual bool sendRawData(const std::vector<uint8_t>& data) = 0;

    // Set handlers
    virtual void setPacketHandler(PacketHandler handler) = 0;
    virtual void setErrorHandler(ErrorHandler handler) = 0;
    virtual void setDisconnectHandler(DisconnectHandler handler) = 0;

    // Connection management
    virtual bool isConnected() const = 0;
    virtual void disconnect() = 0;

    // Get connection info
    virtual std::string getRemoteAddress() const = 0;
    virtual uint16_t getRemotePort() const = 0;
};

// Smart pointer type for connections
using ConnectionPtr = std::shared_ptr<Connection>;

} // namespace netp 