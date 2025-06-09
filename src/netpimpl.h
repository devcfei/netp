#ifndef _NETP_IMPL_H_
#define _NETP_IMPL_H_


using namespace netp;



typedef enum _CONNECTION_EVENT
{
    CONNECTION_CLOSE,
    CONNECTION_ERROR,
    CONNECTION_ESTABLISHED,
}CONNECTION_EVENT;




class ConnectionImpl: public IConnection
{
public:
    ConnectionImpl(struct bufferevent* bev)
        : bev_(bev), pRecvBuf_(nullptr), piConnectionCb_(nullptr)
    {
    }

    virtual ~ConnectionImpl()
    {
        if (pRecvBuf_)
        {
            delete[] pRecvBuf_;
        }
    }

    HRESULT Initialize(std::function<void()> OnClose)
    {
        HRESULT hr = S_OK;
        pRecvBuf_ = new BYTE[4096];
        lambdaClose_ = OnClose;
        return hr;      
    }

    // IConnection interface implementation
    virtual HRESULT SetSession(ISession* piSession);
    virtual ISession* GetSession() { return piConnectionCb_; }
    virtual HRESULT SendPacket(const BYTE* Packet, SIZE_T Length);
    virtual HRESULT GetRemoteIP(char* ipaddr, SIZE_T size) = 0;
    virtual HRESULT GetLocalIP(char* ipaddr, SIZE_T size) { return E_NOTIMPL; }
    virtual HRESULT IsConnected(BOOL* pConnected) { return E_NOTIMPL; }
    virtual HRESULT SetSendBufferSize(SIZE_T size) { return E_NOTIMPL; }
    virtual HRESULT SetReceiveBufferSize(SIZE_T size) { return E_NOTIMPL; }
    virtual HRESULT GetBufferStatus(NETP_BUFFER_STATUS* pStatus) { return E_NOTIMPL; }
    virtual HRESULT Pause() { return E_NOTIMPL; }
    virtual HRESULT Resume() { return E_NOTIMPL; }
    virtual HRESULT IsPaused(BOOL* pPaused) { return E_NOTIMPL; }
    virtual HRESULT GetStatistics(NETP_STATISTICS* pStats) { return E_NOTIMPL; }
    virtual HRESULT GetLastError(NETP_ERROR_CODE* pErrorCode, char* errorMsg, SIZE_T msgSize) { return E_NOTIMPL; }

    BYTE* getRecvBuffer() { return pRecvBuf_; }

protected:
    struct bufferevent* bev_;
    ISession* piConnectionCb_;
    BYTE* pRecvBuf_;
    std::function<void()> lambdaClose_;
};

class ConnectImplServer: public ConnectionImpl
{
public:
    ConnectImplServer(struct bufferevent* bev, char* ipaddr)
        : ConnectionImpl(bev)
    {
        StringCchCopyA(ipaddr_, 16, ipaddr);
    }

    ~ConnectImplServer()
    {
    }

    virtual HRESULT GetRemoteIP(char* ipaddr, SIZE_T size)
    {
        HRESULT hr = S_OK;
        StringCchCopyA(ipaddr, size, ipaddr_);
        return hr;
    }

    virtual HRESULT GetLocalIP(char* ipaddr, SIZE_T size)
    {
        if (!ipaddr || size < 16)
            return E_INVALIDARG;
        StringCchCopyA(ipaddr, size, "0.0.0.0");
        return S_OK;
    }

    virtual HRESULT IsConnected(BOOL* pConnected)
    {
        if (!pConnected)
            return E_INVALIDARG;
        *pConnected = (bev_ != nullptr);
        return S_OK;
    }

    HRESULT OnEvent(CONNECTION_EVENT event)
    {
        HRESULT hr = S_OK;
        if (event == CONNECTION_CLOSE || event == CONNECTION_ERROR)
        {
            lambdaClose_();
        }
        return hr;
    }

private:
    CHAR ipaddr_[16];
};



class ConnectImplClient: public ConnectionImpl
{
public:
    ConnectImplClient(struct bufferevent* bev)
        : ConnectionImpl(bev), connected_(FALSE)
    {
        ipaddr_[0] = '\0';
    }

    ~ConnectImplClient()
    {
    }

    HRESULT Initialize(std::function<void()> OnConnect, std::function<void()> OnClose)
    {
        HRESULT hr = S_OK;
        hr = ConnectionImpl::Initialize(OnClose);
        lambdaOnConnect_ = OnConnect;
        return hr;      
    }

    virtual HRESULT GetRemoteIP(char* ipaddr, SIZE_T size)
    {
        if (!ipaddr || size < 16)
            return E_INVALIDARG;
        
        if (ipaddr_[0] == '\0')
        {
            struct sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            evutil_socket_t fd = bufferevent_getfd(bev_);
            if (fd != -1 && getpeername(fd, (struct sockaddr*)&addr, &addr_len) == 0)
            {
                if (addr.ss_family == AF_INET)
                {
                    struct sockaddr_in* s = (struct sockaddr_in*)&addr;
                    inet_ntop(AF_INET, &s->sin_addr, ipaddr_, sizeof(ipaddr_));
                }
                else if (addr.ss_family == AF_INET6)
                {
                    struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
                    inet_ntop(AF_INET6, &s->sin6_addr, ipaddr_, sizeof(ipaddr_));
                }
            }
        }

        StringCchCopyA(ipaddr, size, ipaddr_);
        return S_OK;
    }

    virtual HRESULT GetLocalIP(char* ipaddr, SIZE_T size)
    {
        if (!ipaddr || size < 16)
            return E_INVALIDARG;
        
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);
        evutil_socket_t fd = bufferevent_getfd(bev_);
        if (fd != -1 && getsockname(fd, (struct sockaddr*)&addr, &addr_len) == 0)
        {
            if (addr.ss_family == AF_INET)
            {
                struct sockaddr_in* s = (struct sockaddr_in*)&addr;
                inet_ntop(AF_INET, &s->sin_addr, ipaddr, size);
                return S_OK;
            }
            else if (addr.ss_family == AF_INET6)
            {
                struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
                inet_ntop(AF_INET6, &s->sin6_addr, ipaddr, size);
                return S_OK;
            }
        }
        return E_FAIL;
    }

    virtual HRESULT IsConnected(BOOL* pConnected)
    {
        if (!pConnected)
            return E_INVALIDARG;
        *pConnected = connected_;
        return S_OK;
    }

    HRESULT OnEvent(CONNECTION_EVENT event)
    {
        HRESULT hr = S_OK;

        switch (event)
        {
        case CONNECTION_CLOSE:
            connected_ = FALSE;
            lambdaClose_();
            break;

        case CONNECTION_ERROR:
            connected_ = FALSE;
            lambdaClose_();
            break;

        case CONNECTION_ESTABLISHED:
            connected_ = TRUE;
            lambdaOnConnect_();
            break;
        }
        return hr;
    }

private:
    std::function<void()> lambdaOnConnect_;
    CHAR ipaddr_[16];
    BOOL connected_;
};


class ServerImpl : public IServer
{
public:
    ServerImpl();
    ~ServerImpl();

    // IServer interface implementation
    virtual HRESULT Initialize(const NETP_SERVER_CONFIG* pConfig, IEventHandler* piEventHandler);
    virtual HRESULT Start();
    virtual HRESULT Stop();
    virtual HRESULT SetMaxConnections(SIZE_T maxConnections) { return E_NOTIMPL; }
    virtual HRESULT GetActiveConnections(SIZE_T* pCount) { return E_NOTIMPL; }
    virtual HRESULT SetKeepAlive(BOOL enable, ULONG idleTime, ULONG interval) { return E_NOTIMPL; }
    virtual HRESULT SetTcpNoDelay(BOOL enable) { return E_NOTIMPL; }
    virtual HRESULT SetReuseAddr(BOOL enable) { return E_NOTIMPL; }

private:
    // tcp Port number
    WORD portnum_;

    // win32 thread
    HANDLE hThreadWorker_;
    DWORD dwThreadID_;

    // libevent backend
    struct event_base* base;
    struct evconnlistener* listener;
    struct event* signal_event;

    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    DWORD WorkerThread();

    static void listener_cb(struct evconnlistener*, evutil_socket_t,
        struct sockaddr*, int socklen, void*);
    static void signal_cb(evutil_socket_t, short, void*);

    static void conn_writecb(struct bufferevent* bev, void* user_data);
    static void conn_readcb(struct bufferevent* bev, void* user_data);
    static void conn_eventcb(struct bufferevent* bev, short events, void* user_data);

    IEventHandler* piEventCb_;

private:
    std::set<IConnection*> vecConnection_;
    HRESULT OnNewConnection(IConnection* piConn)
    {
        HRESULT hr = S_OK;
        vecConnection_.insert(piConn);

        ConnectionImpl* pConn = reinterpret_cast<ConnectionImpl*>(piConn);
        piEventCb_->OnEvent(EVENT_NEW_CONNECTION, ULONG_PTR(piConn), NETP_E_SUCCESS, nullptr);

        pConn->Initialize([this, pConn]()
                          { OnConnectionClose(pConn); });

        return hr;
    }

    virtual void OnConnectionClose(IConnection* pConn)
    {
        vecConnection_.erase(pConn);
        piEventCb_->OnEvent(EVENT_CONNECTION_CLOSE, ULONG_PTR(pConn), NETP_E_SUCCESS, nullptr);
    }
};



class ClientImpl: public IClient
{
public:
    ClientImpl();
    ~ClientImpl();

    // IClient interface implementation
    virtual HRESULT Initialize(const NETP_CLIENT_CONFIG* pConfig, IEventHandler* piEventHandler);
    virtual HRESULT Start();
    virtual HRESULT Stop();
    virtual HRESULT Reconnect() { return E_NOTIMPL; }
    virtual HRESULT IsConnected(BOOL* pConnected) { return E_NOTIMPL; }
    virtual HRESULT SetKeepAlive(BOOL enable, ULONG idleTime, ULONG interval) { return E_NOTIMPL; }
    virtual HRESULT SetTcpNoDelay(BOOL enable) { return E_NOTIMPL; }

private:
    CHAR ipaddr_[46];  // Increased size to match NETP_CLIENT_CONFIG
    WORD portnum_;

    // win32 thread
    HANDLE hThreadWorker_;
    DWORD dwThreadID_;

    // libevent backend
    struct event_base* base;
    struct bufferevent* bev;

    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    DWORD WorkerThread();

    static void conn_writecb(struct bufferevent* bev, void* user_data);
    static void conn_readcb(struct bufferevent* bev, void* user_data);
    static void conn_eventcb(struct bufferevent* bev, short events, void* user_data);

    IEventHandler* piEventCb_;

private:
    void OnConnection(IConnection* pConn)
    {
        piEventCb_->OnEvent(EVENT_NEW_CONNECTION, ULONG_PTR(pConn), NETP_E_SUCCESS, nullptr);
    }

    void OnConnectionClose(IConnection* pConn)
    {
        piEventCb_->OnEvent(EVENT_CONNECTION_CLOSE, ULONG_PTR(pConn), NETP_E_SUCCESS, nullptr);
    }
};


#endif// _NETP_IMPL_H_