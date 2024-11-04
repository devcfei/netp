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
    ConnectionImpl(struct bufferevent *bev)
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
        pRecvBuf_  = new BYTE[4096];
        lambdaClose_=OnClose;
        return hr;      
    }
    virtual HRESULT SetConnectionCallback(IConnectionCallback* piConnectionCb);
    virtual IConnectionCallback* GetConnectionCallback() { return piConnectionCb_; }
    virtual HRESULT SendData(BYTE* Packet, SIZE_T Length );
    // virtual HRESULT GetRemoteIP(char *ipaddr, SIZE_T size);


    BYTE *getRecvBuffer(){return pRecvBuf_;}

    // virtual HRESULT OnEvent(CONNECTION_EVENT event);

private:
    struct bufferevent* bev_;
    IConnectionCallback* piConnectionCb_;
    BYTE *pRecvBuf_;

protected:
    std::function<void()> lambdaClose_;

};

class ConnectImplServer: public ConnectionImpl
{
public:
    ConnectImplServer(struct bufferevent *bev, char *ipaddr)
        :ConnectionImpl(bev)
    {
        StringCchCopyA(ipaddr_, 16, ipaddr);
    }
    ~ConnectImplServer()
    {

    }

    virtual HRESULT GetRemoteIP(char *ipaddr, SIZE_T size)
    {
        HRESULT hr = S_OK;

        StringCchCopyA(ipaddr, size, ipaddr_);

        return hr;
    }

    virtual HRESULT OnEvent(CONNECTION_EVENT event)
    {
        HRESULT hr = S_OK;
        if (event == CONNECTION_CLOSE)
        {
        }
        else if (event == CONNECTION_ERROR)
        {
        }
        lambdaClose_();
        return hr;
    }

private:
    CHAR ipaddr_[16];

};



class ConnectImplClient: public ConnectionImpl
{
public:
    ConnectImplClient(struct bufferevent *bev)
        :ConnectionImpl(bev)
    {

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

    virtual HRESULT GetRemoteIP(char *ipaddr, SIZE_T size)
    {
        HRESULT hr = S_OK;
        return hr;
    }

    virtual HRESULT OnEvent(CONNECTION_EVENT event)
    {
        HRESULT hr = S_OK;

        if (event == CONNECTION_CLOSE)
        {
            lambdaClose_();
        }
        else if (event == CONNECTION_ERROR)
        {
            lambdaClose_();
        }
        else if (event == CONNECTION_ESTABLISHED)
        {
            lambdaOnConnect_();
        }
        return hr;
    }
private:
    std::function<void()> lambdaOnConnect_;

};


class ServerImpl : public IServer
{
public:
    ServerImpl();
    ~ServerImpl();


    HRESULT Initialize(WORD Port, IEventHandler *piEventHander);
    HRESULT Start();

private:
    // tcp Port number
    WORD portnum_;

    // win32 thread
    HANDLE hThreadWorker_;
    DWORD dwThreadID_;

    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    DWORD WorkerThread();


    // libevent backend
    struct event_base* base;
    struct evconnlistener* listener;
    struct event* signal_event;


    static void listener_cb(struct evconnlistener*, evutil_socket_t,
        struct sockaddr*, int socklen, void*);
    static void signal_cb(evutil_socket_t, short, void*);

	static void conn_writecb(struct bufferevent* bev, void* user_data);
    static void conn_readcb(struct bufferevent* bev, void* user_data);
    static void conn_eventcb(struct bufferevent* bev, short events, void* user_data);


    IEventHandler *piEventCb_;

    

private:
    std::set<IConnection*> vecConnection_;
    HRESULT OnNewConnection(IConnection* piConn)
    {
        HRESULT hr = S_OK;
        vecConnection_.insert(piConn);

        ConnectionImpl *pConn = reinterpret_cast<ConnectionImpl *>(piConn);
        piEventCb_->OnEvent(EVENT_NEW_CONNECTION, ULONG_PTR(piConn));

        pConn->Initialize([this, pConn]()
                          { OnConnectionClose(pConn); });

        return hr;
    }

    virtual void OnConnectionClose(IConnection* pConn)
    {
        vecConnection_.erase(pConn);

        piEventCb_->OnEvent(EVENT_CONNECTION_CLOSE, ULONG_PTR(pConn) );
    }

};



class ClientImpl: public IClient
{
public:
    ClientImpl();
    ~ClientImpl();


    HRESULT Initialize(const CHAR* lpszIPAddr, WORD Port, IEventHandler *piEventHander);
    HRESULT Start();
    HRESULT Stop();


private:
    CHAR ipaddr_[16];
    // tcp Port number
    WORD portnum_;

    // win32 thread
    HANDLE hThreadWorker_;
    DWORD dwThreadID_;

    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    DWORD WorkerThread();


    // libevent backend
    struct event_base* base;
	struct bufferevent* bev = NULL;


	static void conn_writecb(struct bufferevent* bev, void* user_data);
    static void conn_readcb(struct bufferevent* bev, void* user_data);
    static void conn_eventcb(struct bufferevent* bev, short events, void* user_data);


    IEventHandler *piEventCb_;

private:
    void OnConnection(IConnection* pConn)
    {
        piEventCb_->OnEvent(EVENT_NEW_CONNECTION, ULONG_PTR(pConn) );
    }

    void OnConnectionClose(IConnection* pConn)
    {
        piEventCb_->OnEvent(EVENT_CONNECTION_CLOSE, ULONG_PTR(pConn) );
    }
};


#endif// _NETP_IMPL_H_