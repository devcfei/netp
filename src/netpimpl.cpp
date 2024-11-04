#include "netpp.h"







HRESULT ConnectionImpl::SendPacket(BYTE* Packet, SIZE_T Length )
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


HRESULT ConnectionImpl::SetSession(ISession* piSession)
{
    HRESULT hr = S_OK;
    if(piConnectionCb_)
    {
        hr = E_NOT_VALID_STATE;
        return hr;
    }

    piConnectionCb_ = piSession;
    
    return hr;
}



