#include "netpp.h"







ServerImpl::ServerImpl()
{

}

ServerImpl::~ServerImpl()
{
    
}



HRESULT ServerImpl::Initialize(WORD Port, IEventHandler *piEventHander)
{
    HRESULT hr = S_OK;

    // WSAStartup
    WSADATA wsaData;
    if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {

        HRESULT hr;
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOGF(_T("WSAStartup failed! hr = %08X\n"), hr);

        hr = E_FAIL;
        return hr;
    }

    
    evthread_use_windows_threads();

    portnum_ = Port;
    piEventCb_ = piEventHander;

    return hr;
}


HRESULT ServerImpl::Start()
{
    HRESULT hr = S_OK;


    hThreadWorker_ = CreateThread(NULL, 0, WorkerThreadProc, this, 0, &dwThreadID_);
    if (hThreadWorker_ == NULL) {
        HRESULT hr;
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOGF(_T("CreateThread failed! hr = %08X\n"), hr);
        hr = E_FAIL;
    }

    return hr;
}


HRESULT ServerImpl::Stop()
{
    HRESULT hr = S_OK;
    event_base_loopbreak(base);
    return hr;
}



DWORD WINAPI ServerImpl::WorkerThreadProc(LPVOID lpParam)
{
    ServerImpl* pThis = (ServerImpl*)lpParam;
    return pThis->WorkerThread();
}

DWORD ServerImpl::WorkerThread()
{
    base = event_base_new();
    if (!base) {
        LOGF(_T("Could not initialize libevent!\n"));
        return 1;
    }

    struct sockaddr_in sin = { 0 };

    sin.sin_family = AF_INET;
    sin.sin_port = htons(portnum_);

    listener = evconnlistener_new_bind(base, listener_cb, (void*)this,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin,
        sizeof(sin));

    if (!listener) {
        LOGF(_T("Could not create a listener!\n"));
        piEventCb_->OnEvent(EVENT_BIND_ERROR, 0);
        return 1;
    }


    event_base_dispatch(base);
    evconnlistener_free(listener);
    event_base_free(base);

    return 0;
}





void ServerImpl::listener_cb(struct evconnlistener* listener, evutil_socket_t fd,
    struct sockaddr* sa, int socklen, void* user_data)
{
    ServerImpl* pServer = (ServerImpl*)user_data;
    struct event_base* base = pServer->base;
    struct bufferevent* bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        LOGF(_T("Error constructing bufferevent!\n"));
        event_base_loopbreak(base);
        return;
    }

    LOGI(_T("new Connection! bev = %p\n"), bev);
    struct sockaddr_in* client_addr = (struct sockaddr_in*)sa;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    LOGI(_T("Client IP: %s\n"), client_ip);

    ConnectImplServer* pConn = new ConnectImplServer(bev,client_ip);

    pServer->OnNewConnection(pConn);

    bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, pConn);
    bufferevent_enable(bev, EV_WRITE);
    bufferevent_enable(bev, EV_READ);
}


void ServerImpl::signal_cb(evutil_socket_t sig, short events, void* user_data)
{
    ServerImpl* pThis = (ServerImpl*)user_data;

    struct event_base* base = pThis->base;
    struct timeval delay = { 2, 0 };

    LOGF(_T("Caught an interrupt signal; exiting cleanly in two seconds.\n"));

    event_base_loopexit(base, &delay);
}


void ServerImpl::conn_writecb(struct bufferevent* bev, void* user_data)
{
    ConnectImplServer* connection = static_cast<ConnectImplServer*>(user_data);
}

void ServerImpl::conn_readcb(struct bufferevent* bev, void* user_data)
{
    ConnectImplServer* connection = static_cast<ConnectImplServer*>(user_data);


    struct evbuffer* input = bufferevent_get_input(bev);
    size_t sz = evbuffer_get_length(input);
    size_t Length = 0;
    size_t count = 0;
    DWORD *lenPacket=0;
    void* buff=connection->getRecvBuffer();
    ISession* piSesion=connection->GetSession();

    size_t last =sz;
    size_t plen =0;
    size_t plast =0;
    BYTE* pbuff=static_cast<BYTE*>(buff);

    while(last>4)
    {
        plen = bufferevent_read(bev, static_cast<BYTE*>(buff) + count, 4);
        lenPacket = reinterpret_cast<DWORD*>(pbuff);
        Length = *lenPacket;
        last-=plen;
        count+=plen;
        plast=Length-plen;
        // LOGI(_T("packet len %d, last %d\n"), Length, last);
        if ( plast <= last)
        {
            plen = bufferevent_read(bev, static_cast<BYTE*>(buff) + count, plast);
            piSesion->OnPacket(pbuff, Length);   
            last-=plen;
            count+=plen;
            pbuff+=Length;
        }
        else
        {
            // LOGW(_T("packet payload not ready!, totol len=%d\n"), Length);
            break;
        }
    }
}

void ServerImpl::conn_eventcb(struct bufferevent* bev, short events, void* user_data)
{
    ConnectImplServer* pConn = static_cast<ConnectImplServer*>(user_data);


    if (events & BEV_EVENT_EOF) {
        LOGI(_T("Connection closed! bev = %p\n"), bev);
        pConn->OnEvent(CONNECTION_CLOSE);


    }
    else if (events & BEV_EVENT_ERROR) {
        //printf("Got an error on the connection: %s\n",
        //    strerror(errno));/*XXX win32*/

        LOGI(_T("Got an error on the connection\n"));/*XXX win32*/
        pConn->OnEvent(CONNECTION_ERROR);

    }
    /* None of the other events can happen here, since we haven't enabled
     * timeouts */


    bufferevent_free(bev);
    delete pConn;
}
