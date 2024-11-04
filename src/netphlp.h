#pragma once


using netp::IServer;
using netp::IConnection;
using netp::IClient;

/// NetpServerImpl
template <typename T, typename TS>
class NetpServerImpl: public IEventHandler
{
public:
    
    HRESULT Initialize(WORD wPort)
    {
        HRESULT hr = S_OK;
        // create instance of IServer
        hr = NetpCreateInstance(NETP_ISERVER, (void **)&piServer_);
        if (FAILED(hr))
        {
            return hr;
        }

        // Initialize the server
        hr = piServer_->Initialize(wPort, this);
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
            hr = E_NOT_VALID_STATE;
            return hr;
        }

        // start server
        hr = piServer_->Start();

        return hr;
    }


    HRESULT Stop()
    {
        HRESULT hr = S_OK;

        // TODO:

        return hr;
    }

protected:
    // IEventHandler
    virtual HRESULT OnEvent(NETP_EVENT_ID eEventId, ULONG_PTR ulParam)
    {
        HRESULT hr = E_FAIL;
        switch(eEventId)
        {
        case EVENT_NEW_CONNECTION:
            hr = OnNewConnection(reinterpret_cast<IConnection*>(ulParam));
            break;

        case EVENT_BIND_ERROR:
            hr = OnBindError();
            break;
        case EVENT_CONNECTION_CLOSE:
            hr = OnConnectionCose(reinterpret_cast<IConnection*>(ulParam));
            break;
        default:             
            break;
        }
        return hr;
    }

protected:
    IServer *piServer_;


private:
    HRESULT OnNewConnection(IConnection *piConn)
    {
        HRESULT hr = S_OK;
        TS *pSession = new TS(piConn);
        if(!pSession)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        
        piConn->SetConnectionCallback(pSession);        
        return hr;
    }
    
    HRESULT OnConnectionCose(IConnection *piConn)
    {
        HRESULT hr = S_OK;
        return hr;
    }

    HRESULT OnBindError()
    {
        HRESULT hr = S_OK;
        return hr;
    }
};



/// NetpClientImpl
template <typename T, typename TS>
class NetpClientImpl: public IEventHandler
{
public:
    
    HRESULT Initialize(LPCTSTR lpszIPAddress, WORD wPort)
    {
        HRESULT hr = S_OK;
        // create instance of IServer
        hr = NetpCreateInstance(NETP_ICLIENT, (void **)&piClient_);
        if (FAILED(hr))
        {
            return hr;
        }

        // Initialize the server
        hr = piClient_->Initialize(lpszIPAddress, wPort, this);
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
    }


    HRESULT Start()
    {
        HRESULT hr = S_OK;
        if (nullptr == piClient_)
        {
            hr = E_NOT_VALID_STATE;
            return hr;
        }

        // start server
        hr = piClient_->Start();

        return hr;
    }


    HRESULT Stop()
    {
        HRESULT hr = S_OK;

        // TODO:

        return hr;
    }

    TS *GetSession()
    {
        return ptSession_;
    }

protected:
    // IEventHandler
    virtual HRESULT OnEvent(NETP_EVENT_ID eEventId, ULONG_PTR ulParam)
    {
        HRESULT hr = E_FAIL;
        switch(eEventId)
        {
        case EVENT_NEW_CONNECTION:
            hr = OnNewConnection(reinterpret_cast<IConnection*>(ulParam));
            break;

        case EVENT_CONNECTION_CLOSE:
            hr = OnConnectionCose(reinterpret_cast<IConnection*>(ulParam));
            break;
        default:             
            break;
        }
        return hr;
    }

protected:
    IClient *piClient_;


private:
    HRESULT OnNewConnection(IConnection *piConn)
    {
        HRESULT hr = S_OK;
        TS *pSession = new TS(piConn);
        if(!pSession)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        
        ptSession_ = pSession;
        piConn->SetConnectionCallback(pSession);        
        return hr;
    }
    
    HRESULT OnConnectionCose(IConnection *piConn)
    {
        HRESULT hr = S_OK;
        return hr;
    }

    TS *ptSession_;
};