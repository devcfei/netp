#include "netpp.h"





ClientImpl::ClientImpl()
{

}

ClientImpl::~ClientImpl()
{
    
}


HRESULT ClientImpl::Initialize(const CHAR* lpszIPAddr, WORD Port, IEventCallback *piConnectEvent)
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

    StringCchCopyA(ipaddr_,16,lpszIPAddr);
    portnum_ = Port;
    piEventCb_ = piConnectEvent;

    return hr;
}

HRESULT ClientImpl::Start()
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




DWORD WINAPI ClientImpl::WorkerThreadProc(LPVOID lpParam)
{
    ClientImpl* pThis = (ClientImpl*)lpParam;
    return pThis->WorkerThread();
}

DWORD ClientImpl::WorkerThread()
{
    base = event_base_new();
    if (!base) {
        LOGF(_T("Could not initialize libevent!\n"));
        return 1;
    }

    struct sockaddr_in sin = { 0 };

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(ipaddr_);
    sin.sin_port = htons(portnum_);

    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    if (bev == NULL)
    {
        LOGE(_T("socket init failed\n"));
        return 1;
    }


    ConnectImplClient* pConn = new ConnectImplClient(bev);


    bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, pConn);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    int flag = bufferevent_socket_connect(bev, (struct sockaddr*)&sin, sizeof(sin));
    if (-1 == flag)
    {
        LOGE(_T("connect failed\n"));
        return 1;
    }

    pConn->Initialize([this, pConn]()
                        { OnConnection (pConn); }, 
                       [this, pConn]()
                        { OnConnectionClose(pConn); } 
                        
                        );

    event_base_dispatch(base);
    event_base_free(base);


    return 0;
}




void ClientImpl::conn_writecb(struct bufferevent* bev, void* user_data)
{
    // LOGI(_T("conn_writecb\n"));

}

void ClientImpl::conn_readcb(struct bufferevent* bev, void* user_data)
{
   
    // LOGI(_T("conn_readcb\n"));

    ConnectionImpl* connection = static_cast<ConnectionImpl*>(user_data);


    struct evbuffer* input = bufferevent_get_input(bev);
    size_t sz = evbuffer_get_length(input);
    size_t Length = 0;
    size_t count = 0;
    DWORD* lenPacket = 0;
    void* buff = connection->getRecvBuffer();
    IConnectionCallback* piSesion = connection->GetConnectionCallback();
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
            // LOGI(_T("packet payload not ready!, totol len=%d\n"), Length);
            break;
        }
    }

}

void ClientImpl::conn_eventcb(struct bufferevent* bev, short events, void* user_data)
{
    ConnectImplClient* pConn = static_cast<ConnectImplClient*>(user_data);

    if (events & BEV_EVENT_EOF) {
        LOGI(_T("Connection closed.\n"));
        pConn->OnEvent(CONNECTION_CLOSE);

    }
    else if (events & BEV_EVENT_ERROR) {
        //printf("Got an error on the connection: %s\n",
        //    strerror(errno));/*XXX win32*/
        LOGI(_T("Connection error.\n"));
        pConn->OnEvent(CONNECTION_ERROR);

    }
    else if (events & BEV_EVENT_CONNECTED)
    {
        LOGI(_T("Connection success\n"));
        pConn->OnEvent(CONNECTION_ESTABLISHED);
        return;
    }
    bufferevent_free(bev);
}


