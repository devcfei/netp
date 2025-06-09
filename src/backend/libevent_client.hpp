#pragma once

#include "../client.hpp"
#include "libevent_common.hpp"
#include "libevent_connection.hpp"
#include <memory>
#include <string>

namespace netp {
namespace backend {

class LibeventClient : public Client {
public:
    LibeventClient();
    ~LibeventClient() override;

    // Client interface implementation
    bool connect(const std::string& host, uint16_t port) override;
    void disconnect() override;
    ConnectionPtr getConnection() override;
    bool isConnected() const override;

private:
    static void connectCallback(struct bufferevent* bev, short events, void* ctx);

    EventBasePtr base_;
    std::shared_ptr<LibeventConnection> connection_;
    std::string host_;
    uint16_t port_;
};

} // namespace backend
} // namespace netp 