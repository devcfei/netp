#pragma once

#include <netp.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/thread.h>
#include <stdexcept>
#include <iostream>
#include <thread>
#include "connection.h"  // For ConnectionState enum

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace netp {
namespace impl {

#ifdef _WIN32
// Windows socket initialization helper
class WinSockInitializer {
public:
    static WinSockInitializer& instance() {
        static WinSockInitializer inst;
        return inst;
    }

    // Call this before any network operations
    static void ensureInitialized() {
        instance();
    }

private:
    WinSockInitializer() {
        std::cout << "[WinSockInitializer] Initializing WSA..." << std::endl;
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "[WinSockInitializer] WSAStartup failed with error: " << result << std::endl;
            throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
        }
        std::cout << "[WinSockInitializer] WSA initialized successfully" << std::endl;
    }

    ~WinSockInitializer() {
        std::cout << "[WinSockInitializer] Cleaning up WSA..." << std::endl;
        WSACleanup();
        std::cout << "[WinSockInitializer] WSA cleanup completed" << std::endl;
    }

    // Prevent copying
    WinSockInitializer(const WinSockInitializer&) = delete;
    WinSockInitializer& operator=(const WinSockInitializer&) = delete;
};
#endif

// Helper class for packet framing
class PacketFramer {
public:
    static constexpr size_t HEADER_SIZE = sizeof(PacketHeader);

    // Process incoming data and extract complete packets
    std::vector<std::vector<uint8_t>> processData(const uint8_t* data, size_t length);

    // Frame a packet for sending
    static std::vector<uint8_t> framePacket(const std::vector<uint8_t>& data);

private:
    std::vector<uint8_t> buffer_;
};

// Smart pointer types for libevent resources
using EventBasePtr = std::unique_ptr<event_base, decltype(&event_base_free)>;
using BufferEventPtr = std::unique_ptr<bufferevent, decltype(&bufferevent_free)>;
using ListenerPtr = std::unique_ptr<evconnlistener, decltype(&evconnlistener_free)>;

// Connection implementation
class ConnectionImpl : public Connection {
public:
    ConnectionImpl(event_base* base, bufferevent* bev);
    ~ConnectionImpl() override;

    bool sendPacket(const Packet& packet) override;
    bool sendRawData(const std::vector<uint8_t>& data) override;
    void setPacketHandler(PacketHandler handler) override;
    void setErrorHandler(ErrorHandler handler) override;
    void setDisconnectHandler(DisconnectHandler handler) override;
    bool isConnected() const override;
    void disconnect() override;
    std::string getRemoteAddress() const override;
    uint16_t getRemotePort() const override;

    void onRead();
    void onError(short events);
    void onConnect();
    void clearCallbacks();
    void setState(ConnectionState state) { state_ = state; }
    ConnectionState getState() const { return state_; }

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
    ConnectionState state_ = ConnectionState::Initial;
};

// Server implementation
class ServerImpl : public Server {
public:
    ServerImpl();
    ~ServerImpl() override;

    bool start(uint16_t port) override;
    void stop() override;
    void setConnectionHandler(ConnectionHandler handler) override;
    void setErrorHandler(ErrorHandler handler) override;
    bool isRunning() const override;
    uint16_t getPort() const override;

private:
    static void acceptCallback(struct evconnlistener* listener,
                             evutil_socket_t fd,
                             struct sockaddr* addr,
                             int socklen,
                             void* ctx);
    
    static void acceptErrorCallback(struct evconnlistener* listener,
                                  void* ctx);

    EventBasePtr base_;
    ListenerPtr listener_;
    ConnectionHandler connection_handler_;
    ErrorHandler error_handler_;
    bool running_;
    uint16_t port_;
    std::thread event_thread_;  // Thread for running the event loop
};

// Client implementation
class ClientImpl : public Client {
public:
    ClientImpl();
    ~ClientImpl() override;

    bool connect(const std::string& host, uint16_t port) override;
    void disconnect() override;
    ConnectionPtr getConnection() override;
    bool isConnected() const override;

private:
    static void connectCallback(struct bufferevent* bev, short events, void* ctx);

    EventBasePtr base_;
    std::shared_ptr<ConnectionImpl> connection_;
    std::string host_;
    uint16_t port_;
    std::thread event_thread_;  // Add thread member
};

} // namespace impl
} // namespace netp 