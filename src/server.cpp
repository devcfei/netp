#include <netpimpl.h>
#include <netpp.h>
#include <thread>
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
        LOGE("[Server] Failed to initialize libevent thread support");
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
        LOGE("[Server] Already running on port %d", port_);
        return false;
    }

    // Create event base if not already created
    if (!base_) {
        base_.reset(event_base_new());
        if (!base_) {
            LOGE("[Server] Failed to create event base");
            return false;
        }
        LOGI("[Server] Created event base");
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    LOGI("[Server] Starting on port %d", port);

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
        LOGE("[Server] Failed to create listener on port %d", port);
        if (error_handler_) {
            error_handler_("Failed to create listener");
        }
        return false;
    }

    LOGI("[Server] Successfully created listener");
    evconnlistener_set_error_cb(listener_.get(), acceptErrorCallback);
    
    running_ = true;
    port_ = port;
    
    // Start the event loop in a separate thread
    event_thread_ = std::thread([this]() {
        LOGI("[Server] Starting event loop");
        
        // Run the event loop
        int res = event_base_dispatch(base_.get());
        if (res < 0) {
            LOGE("[Server] Error in event dispatch");
        } else if (res == 1) {
            LOGI("[Server] Event loop exited normally");
        }
        
        LOGI("[Server] Event loop ended");
    });

    return true;
}

void ServerImpl::stop() {
    if (running_) {
        LOGI("[Server] Stopping server...");
        running_ = false;
        
        // Break the event loop
        event_base_loopexit(base_.get(), nullptr);
        
        // Stop accepting new connections
        listener_.reset();
        
        // Wait for event loop to finish
        if (event_thread_.joinable()) {
            event_thread_.join();
        }
        
        LOGI("[Server] Server stopped");
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

    LOGI("[Server] New connection accepted on socket %d", fd);

    // Get peer information before creating connection
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int gni_ret = getnameinfo(addr, socklen,
                             host, sizeof(host),
                             service, sizeof(service),
                             NI_NUMERICHOST | NI_NUMERICSERV);
    
    if (gni_ret != 0) {
        LOGE("[Server] Failed to get peer info: %s", gai_strerror(gni_ret));
        evutil_closesocket(fd);
        return;
    }

    LOGI("[Server] Accepted connection from %s:%s", host, service);

    // Set socket to non-blocking mode
    evutil_make_socket_nonblocking(fd);
    LOGI("[Server] Set socket to non-blocking mode");

    // Create bufferevent for the new connection
    auto bev = bufferevent_socket_new(
        base,
        fd,
        BEV_OPT_CLOSE_ON_FREE
    );

    if (!bev) {
        LOGE("[Server] Failed to create bufferevent for new connection");
        evutil_closesocket(fd);
        if (server->error_handler_) {
            server->error_handler_("Failed to create bufferevent for new connection");
        }
        return;
    }

    LOGI("[Server] Successfully created bufferevent");

    // Create connection object
    auto conn = std::make_shared<ConnectionImpl>(base, bev);
    
    // Set up the connection
    conn->onConnect();  // This will set up callbacks and mark as connected
    
    LOGI("[Server] Created new connection from %s:%d", 
         conn->getRemoteAddress().c_str(), conn->getRemotePort());

    // Verify bufferevent state
    auto enabled = bufferevent_get_enabled(bev);
    LOGI("[Server] Bufferevent enabled events after connection creation: READ=%s WRITE=%s",
         ((enabled & EV_READ) ? "yes" : "no"),
         ((enabled & EV_WRITE) ? "yes" : "no"));

    // Notify handler
    if (server->connection_handler_) {
        server->connection_handler_(conn);
        LOGI("[Server] Connection handler notified");
    } else {
        LOGW("[Server] Warning: No connection handler set");
    }

    // Final verification of bufferevent state
    enabled = bufferevent_get_enabled(bev);
    LOGI("[Server] Final bufferevent enabled events: READ=%s WRITE=%s",
         ((enabled & EV_READ) ? "yes" : "no"),
         ((enabled & EV_WRITE) ? "yes" : "no"));
}

void ServerImpl::acceptErrorCallback(struct evconnlistener* listener, void* ctx) {
    auto server = static_cast<ServerImpl*>(ctx);
    auto base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();

    LOGE("[Server] Accept error: %d", err);
    if (server->error_handler_) {
        server->error_handler_("Accept error: " + std::to_string(err));
    }

    // Try to restart accepting after a brief pause
    struct timeval delay = { 1, 0 };
    event_base_loopexit(base, &delay);
}

} // namespace impl
} // namespace netp 