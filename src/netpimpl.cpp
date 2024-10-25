#include "netpp.h"







HRESULT ConnectionImpl::SendData(BYTE* Packet, SIZE_T Length )
{
    HRESULT hr = S_OK;
    
    if(Length>4096)
    {
        hr = E_INVALIDARG;
        return hr;
    }

    bufferevent_write(bev_, Packet, Length);

    return hr;
}


HRESULT ConnectionImpl::SetConnectionCallback(IConnectionCallback* piConnectionCb)
{
    HRESULT hr = S_OK;
    if(piConnectionCb_)
    {
        hr = E_NOT_VALID_STATE;
        return hr;
    }

    piConnectionCb_ = piConnectionCb;
    
    return hr;
}



