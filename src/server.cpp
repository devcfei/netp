#include <netpimpl.h>
#include <thread>
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <ws2tcpip.h>
#endif

namespace netp {
namespace impl {

ServerImpl::ServerImpl()
    : base_(nullptr, event_base_free)
    , listener_(nullptr, evconnlistener_free)
    , running_(false)
    , port_(0)
{
#ifdef _WIN32
    // Initialize WinSock first
    WinSockInitializer::ensureInitialized();
    
    // Initialize libevent for Windows threads
    if (evthread_use_windows_threads() < 0) {
        std::cerr << "[Server] Failed to initialize libevent thread support" << std::endl;
        throw std::runtime_error("Failed to initialize libevent thread support");
    }
#endif

    // Now create the event base after WinSock is initialized
    base_.reset(event_base_new());
    if (!base_) {
        throw std::runtime_error("Failed to create event base");
    }
}

ServerImpl::~ServerImpl() {
    stop();
}

bool ServerImpl::start(uint16_t port) {
    if (running_) {
        std::cerr << "[Server] Already running on port " << port_ << std::endl;
        return false;
    }

    // Create event base if not already created
    if (!base_) {
        base_.reset(event_base_new());
        if (!base_) {
            std::cerr << "[Server] Failed to create event base" << std::endl;
            return false;
        }
        std::cout << "[Server] Created event base" << std::endl;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    std::cout << "[Server] Starting on port " << port << std::endl;

    // Create the listener
    listener_.reset(evconnlistener_new_bind(
        base_.get(),
        acceptCallback,
        this,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        -1,  // backlog
        reinterpret_cast<struct sockaddr*>(&sin),
        sizeof(sin)
    ));

    if (!listener_) {
        std::cerr << "[Server] Failed to create listener on port " << port << std::endl;
        if (error_handler_) {
            error_handler_("Failed to create listener");
        }
        return false;
    }

    std::cout << "[Server] Successfully created listener" << std::endl;
    evconnlistener_set_error_cb(listener_.get(), acceptErrorCallback);
    
    running_ = true;
    port_ = port;
    
    // Start the event loop in a separate thread
    event_thread_ = std::thread([this]() {
        std::cout << "[Server] Starting event loop" << std::endl;
        
        // Run the event loop
        int res = event_base_dispatch(base_.get());
        if (res < 0) {
            std::cerr << "[Server] Error in event dispatch" << std::endl;
        } else if (res == 1) {
            std::cout << "[Server] Event loop exited normally" << std::endl;
        }
        
        std::cout << "[Server] Event loop ended" << std::endl;
    });

    return true;
}

void ServerImpl::stop() {
    if (running_) {
        std::cout << "[Server] Stopping server..." << std::endl;
        running_ = false;
        
        // Break the event loop
        event_base_loopexit(base_.get(), nullptr);
        
        // Stop accepting new connections
        listener_.reset();
        
        // Wait for event loop to finish
        if (event_thread_.joinable()) {
            event_thread_.join();
        }
        
        std::cout << "[Server] Server stopped" << std::endl;
    }
}

void ServerImpl::setConnectionHandler(ConnectionHandler handler) {
    connection_handler_ = std::move(handler);
}

void ServerImpl::setErrorHandler(ErrorHandler handler) {
    error_handler_ = std::move(handler);
}

bool ServerImpl::isRunning() const {
    return running_;
}

uint16_t ServerImpl::getPort() const {
    return port_;
}

void ServerImpl::acceptCallback(struct evconnlistener* listener,
                              evutil_socket_t fd,
                              struct sockaddr* addr,
                              int socklen,
                              void* ctx)
{
    auto server = static_cast<ServerImpl*>(ctx);
    auto base = evconnlistener_get_base(listener);

    std::cout << "[Server] New connection accepted on socket " << fd << std::endl;

    // Get peer information before creating connection
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int gni_ret = getnameinfo(addr, socklen,
                             host, sizeof(host),
                             service, sizeof(service),
                             NI_NUMERICHOST | NI_NUMERICSERV);
    
    if (gni_ret != 0) {
        std::cerr << "[Server] Failed to get peer info: " << gai_strerror(gni_ret) << std::endl;
        evutil_closesocket(fd);
        return;
    }

    std::cout << "[Server] Accepted connection from " << host << ":" << service << std::endl;

    // Set socket to non-blocking mode
    evutil_make_socket_nonblocking(fd);
    std::cout << "[Server] Set socket to non-blocking mode" << std::endl;

    // Create bufferevent for the new connection
    auto bev = bufferevent_socket_new(
        base,
        fd,
        BEV_OPT_CLOSE_ON_FREE
    );

    if (!bev) {
        std::cerr << "[Server] Failed to create bufferevent for new connection" << std::endl;
        evutil_closesocket(fd);
        if (server->error_handler_) {
            server->error_handler_("Failed to create bufferevent for new connection");
        }
        return;
    }

    std::cout << "[Server] Successfully created bufferevent" << std::endl;

    // Create connection object
    auto conn = std::make_shared<ConnectionImpl>(base, bev);
    
    // Set up the connection
    conn->onConnect();  // This will set up callbacks and mark as connected
    
    std::cout << "[Server] Created new connection from " << conn->getRemoteAddress() 
              << ":" << conn->getRemotePort() << std::endl;

    // Verify bufferevent state
    auto enabled = bufferevent_get_enabled(bev);
    std::cout << "[Server] Bufferevent enabled events after connection creation: " 
              << "READ=" << ((enabled & EV_READ) ? "yes" : "no") 
              << " WRITE=" << ((enabled & EV_WRITE) ? "yes" : "no") << std::endl;

    // Notify handler
    if (server->connection_handler_) {
        server->connection_handler_(conn);
        std::cout << "[Server] Connection handler notified" << std::endl;
    } else {
        std::cerr << "[Server] Warning: No connection handler set" << std::endl;
    }

    // Final verification of bufferevent state
    enabled = bufferevent_get_enabled(bev);
    std::cout << "[Server] Final bufferevent enabled events: " 
              << "READ=" << ((enabled & EV_READ) ? "yes" : "no") 
              << " WRITE=" << ((enabled & EV_WRITE) ? "yes" : "no") << std::endl;
}

void ServerImpl::acceptErrorCallback(struct evconnlistener* listener, void* ctx) {
    auto server = static_cast<ServerImpl*>(ctx);
    auto base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();

    std::cerr << "[Server] Accept error: " << err << std::endl;
    if (server->error_handler_) {
        server->error_handler_("Accept error: " + std::to_string(err));
    }

    // Try to restart accepting after a brief pause
    struct timeval delay = { 1, 0 };
    event_base_loopexit(base, &delay);
}

} // namespace impl
} // namespace netp 