#pragma once

#include "../server.hpp"
#include "libevent_common.hpp"
#include <event2/listener.h>
#include <memory>

namespace netp {
namespace backend {

class LibeventServer : public Server {
public:
    LibeventServer();
    ~LibeventServer() override;

    // Server interface implementation
    bool start(uint16_t port) override;
    void stop() override;
    void setConnectionHandler(ConnectionHandler handler) override;
    void setErrorHandler(ErrorHandler handler) override;
    bool isRunning() const override;
    uint16_t getPort() const override;

private:
    // Internal event handlers
    static void acceptCallback(struct evconnlistener* listener,
                             evutil_socket_t fd,
                             struct sockaddr* addr,
                             int socklen,
                             void* ctx);
    
    static void acceptErrorCallback(struct evconnlistener* listener,
                                  void* ctx);

    EventBasePtr base_;
    std::unique_ptr<evconnlistener, decltype(&evconnlistener_free)> listener_;
    ConnectionHandler connection_handler_;
    ErrorHandler error_handler_;
    bool running_;
    uint16_t port_;
};

} // namespace backend
} // namespace netp 