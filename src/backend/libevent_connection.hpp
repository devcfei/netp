#pragma once

#include "../connection.hpp"
#include "libevent_common.hpp"
#include <string>
#include <memory>

namespace netp {
namespace backend {

class LibeventConnection : public Connection {
public:
    LibeventConnection(event_base* base, bufferevent* bev);
    ~LibeventConnection() override;

    // Connection interface implementation
    bool sendPacket(const Packet& packet) override;
    bool sendRawData(const std::vector<uint8_t>& data) override;
    void setPacketHandler(PacketHandler handler) override;
    void setErrorHandler(ErrorHandler handler) override;
    void setDisconnectHandler(DisconnectHandler handler) override;
    bool isConnected() const override;
    void disconnect() override;
    std::string getRemoteAddress() const override;
    uint16_t getRemotePort() const override;

    // Internal event handlers
    void onRead();
    void onError(short events);

private:
    EventBasePtr base_;
    BufferEventPtr bev_;
    PacketFramer framer_;
    PacketHandler packet_handler_;
    ErrorHandler error_handler_;
    DisconnectHandler disconnect_handler_;
    bool connected_;
    std::string remote_addr_;
    uint16_t remote_port_;
};

} // namespace backend
} // namespace netp 