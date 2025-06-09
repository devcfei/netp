#include <netpimpl.h>
#include <ws2tcpip.h>
#include <thread>
#include <iostream>
#include <cstring>
#include <cassert>

// std::min
#include <algorithm>

namespace netp {
namespace impl {

// Static callbacks for libevent
static void readCallback(struct bufferevent* bev, void* ctx) {
    std::cout << "[Connection] readCallback triggered" << std::endl;
    auto conn = static_cast<ConnectionImpl*>(ctx);
    conn->onRead();
}

static void writeCallback(struct bufferevent* bev, void* ctx) {
    std::cout << "[Connection] writeCallback triggered" << std::endl;
}

static void eventCallback(struct bufferevent* bev, short events, void* ctx) {
    std::cout << "[Connection] eventCallback triggered with events: 0x" << std::hex << events << std::dec << std::endl;
    auto conn = static_cast<ConnectionImpl*>(ctx);
    conn->onError(events);
}

ConnectionImpl::ConnectionImpl(event_base* base, bufferevent* bev)
    : base_(base, event_base_free)
    , bev_(bev, bufferevent_free)
    , connected_(true)
    , remote_port_(0)
{
    std::cout << "[Connection] Creating new connection" << std::endl;

#ifdef _WIN32
    WinSockInitializer::ensureInitialized();
#endif

    // Get peer info
    struct sockaddr_storage addr;
#ifdef _WIN32
    int addr_len = sizeof(addr);
#else
    socklen_t addr_len = sizeof(addr);
#endif

    evutil_socket_t fd = bufferevent_getfd(bev);
    std::cout << "[Connection] Socket FD: " << fd << std::endl;

    if (getpeername(fd, reinterpret_cast<struct sockaddr*>(&addr), &addr_len) == 0) {
        char host[NI_MAXHOST];
        char service[NI_MAXSERV];
        if (getnameinfo(reinterpret_cast<struct sockaddr*>(&addr), addr_len,
                    host, sizeof(host), service, sizeof(service),
                    NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
            remote_addr_ = host;
            remote_port_ = static_cast<uint16_t>(std::stoi(service));
            std::cout << "[Connection] Peer info: " << remote_addr_ << ":" << remote_port_ << std::endl;
        } else {
            std::cerr << "[Connection] Failed to get peer name info" << std::endl;
        }
    } else {
        std::cerr << "[Connection] Failed to get peer name" << std::endl;
    }

    // First disable all events while setting up
    bufferevent_disable(bev, EV_READ | EV_WRITE);

    // Set callbacks
    bufferevent_setcb(bev, readCallback, writeCallback, eventCallback, this);
    std::cout << "[Connection] Callbacks set" << std::endl;
    
    // Enable both reading and writing
    if (bufferevent_enable(bev, EV_READ | EV_WRITE) < 0) {
        std::cerr << "[Connection] Failed to enable bufferevent" << std::endl;
    } else {
        std::cout << "[Connection] Successfully enabled bufferevent for reading and writing" << std::endl;
    }
    
    // Set watermarks to ensure we don't buffer too much data
    bufferevent_setwatermark(bev, EV_READ, 0, 0);  // No read limits
    bufferevent_setwatermark(bev, EV_WRITE, 0, 0); // No write limits
    
    // Verify event states
    auto enabled = bufferevent_get_enabled(bev);
    std::cout << "[Connection] Enabled events: " 
              << "READ=" << ((enabled & EV_READ) ? "yes" : "no") 
              << " WRITE=" << ((enabled & EV_WRITE) ? "yes" : "no") << std::endl;
}

ConnectionImpl::~ConnectionImpl() {
    disconnect();
}

bool ConnectionImpl::sendPacket(const Packet& packet) {
    if (!isConnected()) {
        std::cerr << "[Connection] Cannot send packet: not connected" << std::endl;
        return false;
    }

    auto data = packet.serialize();
    std::cout << "[Connection] Serialized packet data size: " << data.size() << " bytes" << std::endl;
    
    auto framed_data = PacketFramer::framePacket(data);
    std::cout << "[Connection] Framed packet size: " << framed_data.size() << " bytes" << std::endl;
    
    return sendRawData(framed_data);
}

bool ConnectionImpl::sendRawData(const std::vector<uint8_t>& data) {
    if (!isConnected()) {
        std::cerr << "[Connection] Cannot send raw data: not connected" << std::endl;
        return false;
    }

    auto output = bufferevent_get_output(bev_.get());
    auto result = evbuffer_add(output, data.data(), data.size());
    if (result == 0) {
        std::cout << "[Connection] Successfully queued " << data.size() << " bytes" << std::endl;
        // Force a write attempt
        bufferevent_flush(bev_.get(), EV_WRITE, BEV_FLUSH);
    } else {
        std::cerr << "[Connection] Failed to queue data, error: " << result << std::endl;
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

bool ConnectionImpl::isConnected() const {
    return connected_;
}

void ConnectionImpl::disconnect() {
    if (connected_) {
        connected_ = false;

        // Disable all events and callbacks first
        if (bev_) {
            bufferevent_disable(bev_.get(), EV_READ | EV_WRITE);
            bufferevent_setcb(bev_.get(), nullptr, nullptr, nullptr, nullptr);
        }

        // Notify disconnect handler
        if (disconnect_handler_) {
            disconnect_handler_();
        }

        // Now it's safe to free the bufferevent
        if (bev_) {
            bufferevent_free(bev_.release());
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
    std::cout << "[Connection] onRead called" << std::endl;
    auto input = bufferevent_get_input(bev_.get());
    size_t len = evbuffer_get_length(input);
    
    std::cout << "[Connection] Available data: " << len << " bytes" << std::endl;
    
    if (len > 0) {
        std::vector<uint8_t> data(len);
        if (evbuffer_remove(input, data.data(), len) != static_cast<int>(len)) {
            std::cerr << "[Connection] Failed to read data from input buffer" << std::endl;
            return;
        }
        std::cout << "[Connection] Read " << len << " bytes from buffer" << std::endl;
        
        auto packets = framer_.processData(data.data(), data.size());
        std::cout << "[Connection] Processed " << packets.size() << " complete packets" << std::endl;
        
        for (const auto& packet : packets) {
            std::cout << "[Connection] Processing packet of size " << packet.size() << " bytes" << std::endl;
            if (packet_handler_) {
                packet_handler_(packet);
            } else {
                std::cerr << "[Connection] Warning: No packet handler set" << std::endl;
            }
        }
    }
}

void ConnectionImpl::onError(short events) {
    std::cout << "[Connection] onError called with events: " << events << std::endl;
    
    if (events & BEV_EVENT_ERROR) {
        int err = EVUTIL_SOCKET_ERROR();
        std::cerr << "[Connection] Bufferevent error: " << err << std::endl;
        if (error_handler_) {
            error_handler_("Connection error: " + std::to_string(err));
        }
    }
    
    if (events & BEV_EVENT_EOF) {
        std::cout << "[Connection] Peer closed connection" << std::endl;
    }
    
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        // Schedule disconnect to avoid immediate cleanup during callback
        if (base_) {
            struct timeval tv = { 0, 0 };  // immediate execution
            event_base_once(base_.get(), -1, EV_TIMEOUT, 
                [](evutil_socket_t, short, void* arg) {
                    auto conn = static_cast<ConnectionImpl*>(arg);
                    conn->disconnect();
                }, this, &tv);
        }
    }
}

} // namespace impl
} // namespace netp 