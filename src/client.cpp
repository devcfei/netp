#include <netpimpl.h>
#include <ws2tcpip.h>
#include <thread>

namespace netp {
namespace impl {

ClientImpl::ClientImpl()
    : base_(event_base_new(), event_base_free)
    , port_(0)
{
#ifdef _WIN32
    // Initialize WinSock
    WinSockInitializer::ensureInitialized();
    
    // Initialize libevent for Windows threads
    if (evthread_use_windows_threads() < 0) {
        std::cerr << "[Client] Failed to initialize libevent thread support" << std::endl;
        throw std::runtime_error("Failed to initialize libevent thread support");
    }
    std::cout << "[Client] Initialized libevent thread support" << std::endl;
#endif
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

    // Set up connection error handler
    connection_->setErrorHandler([this](const std::string& error) {
        // Ensure clean shutdown on connection error
        disconnect();
    });

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
    std::thread([this]() {
        event_base_dispatch(base_.get());
    }).detach();

    return true;
}

void ClientImpl::disconnect() {
    if (connection_) {
        // First stop the event loop
        if (base_) {
            event_base_loopexit(base_.get(), nullptr);
        }

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
        // Connection successful
    } else {
        // Connection failed
        client->disconnect();
    }
}

} // namespace impl
} // namespace netp 