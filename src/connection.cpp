#include <netpimpl.h>
#include <netpp.h>
#include <thread>
#include <cstring>
#include <cassert>

// std::min
#include <algorithm>

#ifdef _WIN32
#include <ws2tcpip.h>
#elif __linux__
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

namespace netp {
namespace impl {

// Static callbacks for libevent
static void readCallback(struct bufferevent* bev, void* ctx) {
    LOGV("[Connection] readCallback triggered");
    auto conn = static_cast<ConnectionImpl*>(ctx);
    
    // Check if connection is still valid
    if (!conn || conn->getState() == ConnectionState::Closed) {
        LOGV("[Connection] Read callback called on invalid/closed connection");
        return;
    }
    
    conn->onRead();
}

static void writeCallback(struct bufferevent* bev, void* ctx) {
    LOGV("[Connection] writeCallback triggered");
}

static void eventCallback(struct bufferevent* bev, short events, void* ctx) {
    LOGV("[Connection] eventCallback triggered with events: 0x%x", events);
    auto conn = static_cast<ConnectionImpl*>(ctx);
    
    // Check if connection is still valid
    if (!conn || conn->getState() == ConnectionState::Closed) {
        LOGV("[Connection] Event callback called on invalid/closed connection");
        return;
    }
    
    if (events & BEV_EVENT_EOF) {
        LOGI("[Connection] EOF received");
        conn->onClose();
    } else {
        conn->onError(events);
    }
}

ConnectionImpl::ConnectionImpl(event_base* base, bufferevent* bev)
    : base_(base)  // Store raw pointer - we don't own this
    , bev_(bev, bufferevent_free)
    , connected_(false)  // Initialize to false until connection is confirmed
    , remote_port_(0)
    , state_(ConnectionState::Closed)
{
    LOGI("[Connection] Creating new connection");

#ifdef _WIN32
    WinSockInitializer::ensureInitialized();
#elif __linux__
    LinuxThreadInitializer::ensureInitialized();
#endif

    if (!bev) {
        throw std::runtime_error("Invalid bufferevent");
    }
}

ConnectionImpl::~ConnectionImpl() {
    // Clear callbacks first to prevent any callbacks from being called during destruction
    clearCallbacks();
    
    // The bufferevent will be automatically freed by the unique_ptr
    // No need to manually call bufferevent_free
}

bool ConnectionImpl::sendPacket(const Packet& packet) {
    if (!isConnected()) {
        LOGE("[Connection] Cannot send packet: not connected");
        return false;
    }

    auto data = packet.serialize();
    LOGV("[Connection] Serialized packet data size: %zu bytes", data.size());
    
    auto framed_data = PacketFramer::framePacket(data);
    LOGV("[Connection] Framed packet size: %zu bytes", framed_data.size());
    
    return sendRawData(framed_data);
}

bool ConnectionImpl::sendRawData(const std::vector<uint8_t>& data) {
    if (!isConnected()) {
        LOGE("[Connection] Cannot send raw data: not connected");
        return false;
    }

    if (!bev_) {
        LOGE("[Connection] Cannot send raw data: invalid bufferevent");
        return false;
    }

    auto output = bufferevent_get_output(bev_.get());
    if (!output) {
        LOGE("[Connection] Failed to get output buffer");
        return false;
    }
    
    auto result = evbuffer_add(output, data.data(), data.size());
    if (result == 0) {
        LOGV("[Connection] Successfully queued %zu bytes", data.size());
        // Force a write attempt
        bufferevent_flush(bev_.get(), EV_WRITE, BEV_FLUSH);
    } else {
        LOGE("[Connection] Failed to queue data, error: %d", result);
    }
    return (result == 0);
}

void ConnectionImpl::setPacketHandler(PacketHandler handler) {
    packet_handler_ = std::move(handler);
}

void ConnectionImpl::setErrorHandler(ErrorHandler handler) {
    error_handler_ = std::move(handler);
}

void ConnectionImpl::setDisconnectHandler(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
}

void ConnectionImpl::setConnectedHandler(ConnectedHandler handler) {
    connected_handler_ = std::move(handler);
}

bool ConnectionImpl::isConnected() const {
    return connected_;
}

void ConnectionImpl::onConnect() {
    connected_ = true;
    state_ = ConnectionState::Connected;

    // Get peer info
    struct sockaddr_storage addr;
#ifdef _WIN32
    int addr_len = sizeof(addr);
#else
    socklen_t addr_len = sizeof(addr);
#endif

    evutil_socket_t fd = bufferevent_getfd(bev_.get());
    LOGV("[Connection] Socket FD: %d", fd);

    if (getpeername(fd, reinterpret_cast<struct sockaddr*>(&addr), &addr_len) == 0) {
        char host[NI_MAXHOST];
        char service[NI_MAXSERV];
        if (getnameinfo(reinterpret_cast<struct sockaddr*>(&addr), addr_len,
                    host, sizeof(host), service, sizeof(service),
                    NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
            remote_addr_ = host;
            remote_port_ = static_cast<uint16_t>(std::stoi(service));
            LOGI("[Connection] Peer info: %s:%d", remote_addr_.c_str(), remote_port_);
        } else {
            LOGE("[Connection] Failed to get peer name info");
            remote_addr_ = "unknown";
            remote_port_ = 0;
        }
    } else {
        LOGE("[Connection] Failed to get peer name");
        remote_addr_ = "unknown";
        remote_port_ = 0;
    }

    // Set up the callbacks now that we're connected
    if (bev_) {
        bufferevent_setcb(bev_.get(), readCallback, writeCallback, eventCallback, this);
        bufferevent_enable(bev_.get(), EV_READ | EV_WRITE);
        LOGI("[Connection] Callbacks set up and events enabled");
    }

    // Call the connected handler if set
    if (connected_handler_) {
        connected_handler_();
    }
}

void ConnectionImpl::onError(short events) {
    state_ = ConnectionState::Failed;
    connected_ = false;
    
    // Clear callbacks first to prevent re-entrancy
    clearCallbacks();
    
    if (error_handler_) {
        int err = EVUTIL_SOCKET_ERROR();
        std::string error_msg = "Connection error: ";
        
        if (err == 10061) {
            error_msg += "Connection refused (server not available)";
        } else {
            error_msg += std::to_string(err);
        }
        
        error_handler_(error_msg);
    }
    
    // After error, we should disconnect
    if (disconnect_handler_) {
        disconnect_handler_();
    }
}

void ConnectionImpl::onClose() {
    LOGI("[Connection] Connection closed");
    
    // Set state to closed first
    state_ = ConnectionState::Closed;
    connected_ = false;
    
    // Store disconnect handler before clearing callbacks
    auto disconnect_handler = disconnect_handler_;
    
    // Clear callbacks before calling handlers to prevent re-entrancy
    clearCallbacks();
    
    // Notify about disconnection after clearing callbacks
    if (disconnect_handler) {
        disconnect_handler();
    }
}

void ConnectionImpl::clearCallbacks() {
    if (bev_) {
        // Disable all events first
        bufferevent_disable(bev_.get(), EV_READ | EV_WRITE);
        // Clear all callbacks
        bufferevent_setcb(bev_.get(), nullptr, nullptr, nullptr, nullptr);
    }
    
    // Clear handler functions
    packet_handler_ = nullptr;
    error_handler_ = nullptr;
    disconnect_handler_ = nullptr;
    connected_handler_ = nullptr;
    connected_ = false;
    state_ = ConnectionState::Closed;  // Update state when clearing callbacks
}

void ConnectionImpl::disconnect() {
    if (connected_) {
        connected_ = false;
        state_ = ConnectionState::Closed;

        // Clear all callbacks first
        clearCallbacks();

        // Now it's safe to free the bufferevent
        if (bev_) {
            // Release the bufferevent without calling bufferevent_free
            // Let the unique_ptr handle the cleanup
            bev_.reset();
        }
    }
}

std::string ConnectionImpl::getRemoteAddress() const {
    return remote_addr_;
}

uint16_t ConnectionImpl::getRemotePort() const {
    return remote_port_;
}

void ConnectionImpl::onRead() {
    LOGV("[Connection] onRead called");
    
    // Additional safety check
    if (!bev_ || !isConnected()) {
        LOGV("[Connection] onRead called on invalid connection");
        return;
    }
    
    auto input = bufferevent_get_input(bev_.get());
    if (!input) {
        LOGE("[Connection] Failed to get input buffer");
        return;
    }
    
    size_t len = evbuffer_get_length(input);
    
    LOGV("[Connection] Available data: %zu bytes", len);
    
    if (len > 0) {
        std::vector<uint8_t> data(len);
        if (evbuffer_remove(input, data.data(), len) != static_cast<int>(len)) {
            LOGE("[Connection] Failed to read data from input buffer");
            return;
        }
        LOGV("[Connection] Read %zu bytes from buffer", len);
        
        auto packets = framer_.processData(data.data(), data.size());
        LOGV("[Connection] Processed %zu complete packets", packets.size());
        
        for (const auto& packet : packets) {
            LOGV("[Connection] Processing packet of size %zu bytes", packet.size());
            if (packet_handler_) {
                packet_handler_(packet);
            } else {
                LOGW("[Connection] Warning: No packet handler set");
            }
        }
    }
}

} // namespace impl
} // namespace netp 