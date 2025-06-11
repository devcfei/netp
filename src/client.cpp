#include <netpimpl.h>
#include <thread>
#include <cstring>


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
}

bool ClientImpl::connect(const std::string& host, uint16_t port) {
    if (connection_) {
        return false;
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
        // First stop the event loop
        event_base_loopbreak(base_.get());

        // Then disconnect the connection
        connection_->disconnect();
        connection_.reset();
    }
}

ConnectionPtr ClientImpl::getConnection() {
    return connection_;
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
    } else {
        // Connection failed
        if (events & BEV_EVENT_ERROR) {
            int err = EVUTIL_SOCKET_ERROR();
            if (client->connection_) {
                client->connection_->onError(events);
            }
        }
        client->disconnect();
    }
}

} // namespace impl
} // namespace netp 