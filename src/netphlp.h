#pragma once

#include "netp.h"

using namespace netp;

/// NetpServerImpl
template <typename T, typename TS>
class NetpServerImpl : public IEventHandler
{
public:
    NetpServerImpl() : piServer_(nullptr) {}
    virtual ~NetpServerImpl() 
    {
        if (piServer_)
        {
            piServer_->Stop();
            piServer_ = nullptr;
        }
    }

    HRESULT Initialize(WORD wPort)
    {
        NETP_SERVER_CONFIG config = {0};
        config.port = wPort;
        config.maxConnections = 1000;
        config.defaultSendBuffer = 65536;
        config.defaultRecvBuffer = 65536;
        config.reuseAddr = TRUE;
        config.tcpNoDelay = TRUE;
        config.keepAlive.enabled = TRUE;
        config.keepAlive.idleTime = 60000;  // 60 seconds
        config.keepAlive.interval = 1000;   // 1 second

        return Initialize(&config);
    }

    HRESULT Initialize(const NETP_SERVER_CONFIG* pConfig)
    {
        HRESULT hr = S_OK;
        
        // Create instance of IServer
        hr = NetpCreateInstance(NETP_ISERVER, (void**)&piServer_);
        if (FAILED(hr))
        {
            return hr;
        }

        // Initialize the server
        hr = piServer_->Initialize(pConfig, this);
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
    }

    HRESULT Start()
    {
        HRESULT hr = S_OK;
        if (nullptr == piServer_)
        {
            hr = NETP_E_INVALID_STATE;
            return hr;
        }

        hr = piServer_->Start();
        return hr;
    }

    HRESULT Stop()
    {
        HRESULT hr = S_OK;
        if (piServer_)
        {
            hr = piServer_->Stop();
        }
        return hr;
    }

    HRESULT GetActiveConnections(SIZE_T* pCount)
    {
        if (nullptr == piServer_)
        {
            return NETP_E_INVALID_STATE;
        }
        return piServer_->GetActiveConnections(pCount);
    }

protected:
    // IEventHandler implementation
    virtual HRESULT OnEvent(NETP_EVENT_ID eEventId, 
                          ULONG_PTR ulParam,
                          NETP_ERROR_CODE errorCode,
                          LPCSTR errorMessage) override
    {
        HRESULT hr = S_OK;
        switch(eEventId)
        {
        case EVENT_NEW_CONNECTION:
            hr = OnNewConnection(reinterpret_cast<IConnection*>(ulParam));
            break;
        case EVENT_BIND_ERROR:
            hr = OnBindError(errorCode, errorMessage);
            break;
        case EVENT_CONNECTION_CLOSE:
            hr = OnConnectionClose(reinterpret_cast<IConnection*>(ulParam));
            break;
        case EVENT_NETWORK_ERROR:
            hr = OnNetworkError(errorCode, errorMessage);
            break;
        default:             
            hr = static_cast<T*>(this)->OnCustomEvent(eEventId, ulParam, errorCode, errorMessage);
            break;
        }
        return hr;
    }

protected:
    IServer* piServer_;

private:
    HRESULT OnNewConnection(IConnection* piConn)
    {
        HRESULT hr = S_OK;
        
        if (!piConn)
            return E_INVALIDARG;

        TS* pSession = new TS(piConn);
        if (!pSession)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        
        hr = piConn->SetSession(pSession);
        if (FAILED(hr))
        {
            delete pSession;
            return hr;
        }

        return static_cast<T*>(this)->OnClientConnected(piConn);
    }
    
    HRESULT OnConnectionClose(IConnection* piConn)
    {
        if (!piConn)
            return E_INVALIDARG;

        ISession* pSession = piConn->GetSession();
        if (pSession)
        {
            delete pSession;
            piConn->SetSession(nullptr);
        }

        return static_cast<T*>(this)->OnClientDisconnected(piConn);
    }

    HRESULT OnBindError(NETP_ERROR_CODE errorCode, LPCSTR errorMessage)
    {
        return static_cast<T*>(this)->OnServerError(errorCode, errorMessage);
    }

    HRESULT OnNetworkError(NETP_ERROR_CODE errorCode, LPCSTR errorMessage)
    {
        return static_cast<T*>(this)->OnServerError(errorCode, errorMessage);
    }
};

/// NetpClientImpl
template <typename T, typename TS>
class NetpClientImpl : public IEventHandler
{
public:
    NetpClientImpl() : piClient_(nullptr), ptSession_(nullptr) {}
    virtual ~NetpClientImpl()
    {
        if (piClient_)
        {
            piClient_->Stop();
            piClient_ = nullptr;
        }
    }

    HRESULT Initialize(LPCSTR lpszIPAddress, WORD wPort)
    {
        NETP_CLIENT_CONFIG config = {0};
        strncpy_s(config.ipAddress, sizeof(config.ipAddress), lpszIPAddress, _TRUNCATE);
        config.port = wPort;
        config.sendBufferSize = 65536;
        config.recvBufferSize = 65536;
        config.tcpNoDelay = TRUE;
        config.keepAlive.enabled = TRUE;
        config.keepAlive.idleTime = 60000;  // 60 seconds
        config.keepAlive.interval = 1000;   // 1 second
        config.connectTimeout = 5000;       // 5 seconds

        return Initialize(&config);
    }

    HRESULT Initialize(const NETP_CLIENT_CONFIG* pConfig)
    {
        HRESULT hr = S_OK;
        
        hr = NetpCreateInstance(NETP_ICLIENT, (void**)&piClient_);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = piClient_->Initialize(pConfig, this);
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
    }

    HRESULT Start()
    {
        if (nullptr == piClient_)
        {
            return NETP_E_INVALID_STATE;
        }
        return piClient_->Start();
    }

    HRESULT Stop()
    {
        HRESULT hr = S_OK;
        if (piClient_)
        {
            hr = piClient_->Stop();
        }
        return hr;
    }

    HRESULT Reconnect()
    {
        if (nullptr == piClient_)
        {
            return NETP_E_INVALID_STATE;
        }
        return piClient_->Reconnect();
    }

    TS* GetSession()
    {
        return ptSession_;
    }

protected:
    // IEventHandler implementation
    virtual HRESULT OnEvent(NETP_EVENT_ID eEventId, 
                          ULONG_PTR ulParam,
                          NETP_ERROR_CODE errorCode,
                          LPCSTR errorMessage) override
    {
        HRESULT hr = S_OK;
        switch(eEventId)
        {
        case EVENT_NEW_CONNECTION:
            hr = OnNewConnection(reinterpret_cast<IConnection*>(ulParam));
            break;
        case EVENT_CONNECTION_CLOSE:
            hr = OnConnectionClose(reinterpret_cast<IConnection*>(ulParam));
            break;
        case EVENT_NETWORK_ERROR:
            hr = OnNetworkError(errorCode, errorMessage);
            break;
        default:
            hr = static_cast<T*>(this)->OnCustomEvent(eEventId, ulParam, errorCode, errorMessage);
            break;
        }
        return hr;
    }

protected:
    IClient* piClient_;
    TS* ptSession_;

private:
    HRESULT OnNewConnection(IConnection* piConn)
    {
        HRESULT hr = S_OK;
        
        if (!piConn)
            return E_INVALIDARG;

        TS* pSession = new TS(piConn);
        if (!pSession)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        
        ptSession_ = pSession;
        hr = piConn->SetSession(pSession);
        if (FAILED(hr))
        {
            delete pSession;
            ptSession_ = nullptr;
            return hr;
        }

        return static_cast<T*>(this)->OnConnected(piConn);
    }
    
    HRESULT OnConnectionClose(IConnection* piConn)
    {
        if (!piConn)
            return E_INVALIDARG;

        ISession* pSession = piConn->GetSession();
        if (pSession)
        {
            delete pSession;
            piConn->SetSession(nullptr);
            ptSession_ = nullptr;
        }

        return static_cast<T*>(this)->OnDisconnected(piConn);
    }

    HRESULT OnNetworkError(NETP_ERROR_CODE errorCode, LPCSTR errorMessage)
    {
        return S_OK;
    }
};