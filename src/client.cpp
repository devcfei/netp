#include <netpimpl.h>
#include <thread>
#include <cstring>
#include "connection.h"
#include "netpp.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#elif __linux__
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else 
#error "unsupported OS!"
#endif

namespace netp {
namespace impl {

ClientImpl::ClientImpl()
    : base_(nullptr, event_base_free)  // Initialize with null ptr but specify deleter
    , port_(0)
    , event_thread_running_(false)
{
    LOGI("[Client] Initializing client...");
    
#ifdef _WIN32
    // Initialize WinSock first
    WinSockInitializer::ensureInitialized();
    
    // Initialize libevent for Windows threads
    if (evthread_use_windows_threads() < 0) {
        LOGE("[Client] Failed to initialize libevent thread support");
        throw std::runtime_error("Failed to initialize libevent thread support");
    }
#elif __linux__
    // Initialize libevent for pthreads on Linux
    try {
        LinuxThreadInitializer::ensureInitialized();
        LOGI("[Client] Linux thread initialization successful");
    } catch (const std::exception& e) {
        LOGE("[Client] Linux thread initialization failed: %s", e.what());
        throw;
    }
#endif

    // Now create the event base after initialization
    LOGI("[Client] Creating event base...");
    base_.reset(event_base_new());
    if (!base_) {
        LOGE("[Client] Failed to create event base");
        throw std::runtime_error("Failed to create event base");
    }
    LOGI("[Client] Event base created successfully");
}

ClientImpl::~ClientImpl() {
    disconnect();
    
    // Wait for event thread to finish
    if (event_thread_.joinable()) {
        event_thread_.join();
    }
}

bool ClientImpl::connect(const std::string& host, uint16_t port) {
    // Disconnect any existing connection first
    disconnect();
    
    // Wait for event thread to finish if it's running
    if (event_thread_.joinable()) {
        event_thread_.join();
    }

    host_ = host;
    port_ = port;

    LOGI("[Client] Starting connection to %s:%d", host.c_str(), port);

    // Create bufferevent first
    LOGI("[Client] Creating bufferevent...");
    auto bev = bufferevent_socket_new(
        base_.get(),
        -1,
        BEV_OPT_CLOSE_ON_FREE
    );

    if (!bev) {
        LOGE("[Client] Failed to create bufferevent");
        return false;
    }

    LOGI("[Client] Created bufferevent successfully");

    // Set up the connection
    try {
        connection_ = std::make_shared<ConnectionImpl>(base_.get(), bev);
        LOGI("[Client] Connection object created successfully");
    } catch (const std::exception& e) {
        LOGE("[Client] Failed to create connection object: %s", e.what());
        bufferevent_free(bev);
        return false;
    } catch (...) {
        LOGE("[Client] Unknown exception creating connection object");
        bufferevent_free(bev);
        return false;
    }

    // Set up the callbacks for the bufferevent BEFORE starting connection
    bufferevent_setcb(bev, nullptr, nullptr, connectCallback, this);
    LOGI("[Client] Set up connection callbacks");

    // Start connection
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &sin.sin_addr) <= 0) {
        LOGE("[Client] Failed to parse host address: %s", host.c_str());
        connection_.reset();
        return false;
    }

    LOGI("[Client] Initiating connection...");
    if (bufferevent_socket_connect(bev,
                                 reinterpret_cast<struct sockaddr*>(&sin),
                                 sizeof(sin)) < 0) {
        LOGE("[Client] Failed to initiate connection to %s:%d", host.c_str(), port);
        connection_.reset();
        return false;
    }

    LOGI("[Client] Connection initiated, starting event loop");

    // Start the event loop in a separate thread AFTER setting up connection
    event_thread_running_ = true;
    
    // Start event loop thread
    event_thread_ = std::thread([this]() {
        LOGI("[Client] Starting event loop");
        event_base_dispatch(base_.get());
        LOGI("[Client] Event loop finished");
        event_thread_running_ = false;
    });

    LOGI("[Client] Event loop thread started");
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
    
    // Wait for event thread to finish
    if (event_thread_.joinable()) {
        event_thread_.join();
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
    
    LOGI("[Client] connectCallback called with events: 0x%x", events);
    
    // Safety check
    if (!client) {
        LOGE("[Client] connectCallback called with null client");
        return;
    }

    if (events & BEV_EVENT_CONNECTED) {
        LOGI("[Client] Connection established to %s:%d", client->host_.c_str(), client->port_);
        // Now that we're actually connected, mark the connection as established
        if (client->connection_) {
            try {
                // Let the connection set up its callbacks and mark itself as connected
                client->connection_->onConnect();
                LOGI("[Client] Connection setup completed successfully");
            } catch (const std::exception& e) {
                LOGE("[Client] Exception in onConnect: %s", e.what());
            }
        } else {
            LOGE("[Client] Connection object is null in connectCallback");
        }
    } else if (events & BEV_EVENT_ERROR) {
        int err = EVUTIL_SOCKET_ERROR();
        LOGE("[Client] Connection error to %s:%d: %d", client->host_.c_str(), client->port_, err);
        
        if (client->connection_) {
            try {
                // Report the error through the connection's error handler
                client->connection_->onError(events);
                
                // Set internal state but keep the object alive
                client->connection_->setState(ConnectionState::Failed);
            } catch (const std::exception& e) {
                LOGE("[Client] Exception in onError: %s", e.what());
            }
        }
        
        // Break the event loop
        if (client->base_) {
            event_base_loopbreak(client->base_.get());
        }
    } else {
        // Other events like EOF
        LOGI("[Client] Connection event: 0x%x", events);
        if (client->connection_) {
            try {
                client->connection_->setState(ConnectionState::Failed);
            } catch (const std::exception& e) {
                LOGE("[Client] Exception in event handling: %s", e.what());
            }
        }
        
        // Break the event loop
        if (client->base_) {
            event_base_loopbreak(client->base_.get());
        }
    }
}

} // namespace impl
} // namespace netp 