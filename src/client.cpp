#include <netpimpl.h>
#include <thread>
#include <cstring>
#include "connection.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#elif __linux__
#include <arpa/inet.h>
#else 
#error "unsupported OS!"
#endif

namespace netp {
namespace impl {

ClientImpl::ClientImpl()
    : base_(nullptr, event_base_free)  // Initialize with null ptr but specify deleter
    , port_(0)
{
#ifdef _WIN32
    // Initialize WinSock first
    WinSockInitializer::ensureInitialized();
    
    // Initialize libevent for Windows threads
    if (evthread_use_windows_threads() < 0) {
        std::cerr << "[Client] Failed to initialize libevent thread support" << std::endl;
        throw std::runtime_error("Failed to initialize libevent thread support");
    }
#endif

    // Now create the event base after WinSock is initialized
    base_.reset(event_base_new());
    if (!base_) {
        throw std::runtime_error("Failed to create event base");
    }
}

ClientImpl::~ClientImpl() {
    disconnect();
    
    // Add a small delay to ensure event loop has exited
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool ClientImpl::connect(const std::string& host, uint16_t port) {
    // Only allow connection if we don't have a connection or if previous connection failed
    if (connection_ && connection_->getState() != ConnectionState::Failed) {
        return false;
    }

    // Clean up any existing failed connection
    if (connection_) {
        connection_->clearCallbacks();
        connection_.reset();
    }

    host_ = host;
    port_ = port;

    // Create bufferevent
    auto bev = bufferevent_socket_new(
        base_.get(),
        -1,
        BEV_OPT_CLOSE_ON_FREE
    );

    if (!bev) {
        return false;
    }

    // Set up the connection
    connection_ = std::make_shared<ConnectionImpl>(base_.get(), bev);

    // Set up the callbacks for the bufferevent
    bufferevent_setcb(bev, nullptr, nullptr, connectCallback, this);

    // Start connection
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &sin.sin_addr) <= 0) {
        connection_.reset();
        return false;
    }

    if (bufferevent_socket_connect(bev,
                                 reinterpret_cast<struct sockaddr*>(&sin),
                                 sizeof(sin)) < 0) {
        connection_.reset();
        return false;
    }

    // Start the event loop in a separate thread
    event_thread_ = std::thread([this]() {
        event_base_dispatch(base_.get());
    });
    event_thread_.detach();  // Detach the thread to let it clean up on its own

    return true;
}

void ClientImpl::disconnect() {
    if (connection_) {
        // First disable and clear all bufferevent callbacks
        connection_->clearCallbacks();
        
        // Then stop the event loop
        event_base_loopbreak(base_.get());

        // Clean up connection
        connection_.reset();
    }
}

ConnectionPtr ClientImpl::getConnection() {
    return std::static_pointer_cast<Connection>(connection_);
}

bool ClientImpl::isConnected() const {
    return connection_ && connection_->isConnected();
}

void ClientImpl::connectCallback(struct bufferevent* bev, short events, void* ctx) {
    auto client = static_cast<ClientImpl*>(ctx);

    if (events & BEV_EVENT_CONNECTED) {
        // Connection successful - set up the normal callbacks
        bufferevent_setcb(bev, nullptr, nullptr, nullptr, client->connection_.get());
        bufferevent_enable(bev, EV_READ | EV_WRITE);
        
        // Now that we're actually connected, mark the connection as established
        if (client->connection_) {
            client->connection_->onConnect();
        }
    } else if (events & BEV_EVENT_ERROR) {
        int err = EVUTIL_SOCKET_ERROR();
        if (client->connection_) {
            // Report the error through the connection's error handler
            client->connection_->onError(events);
            
            // Set internal state but keep the object alive
            client->connection_->setState(ConnectionState::Failed);
            
            // Break the event loop
            if (client->base_) {
                event_base_loopbreak(client->base_.get());
            }
        }
    } else {
        // Other events like EOF
        if (client->connection_) {
            client->connection_->setState(ConnectionState::Failed);
            
            // Break the event loop
            if (client->base_) {
                event_base_loopbreak(client->base_.get());
            }
        }
    }
}

} // namespace impl
} // namespace netp 