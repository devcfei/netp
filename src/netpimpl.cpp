#include "netpp.h"

HRESULT ConnectionImpl::SendPacket(const BYTE* Packet, SIZE_T Length)
{
    HRESULT hr = S_OK;
    
    if (!Packet || Length == 0)
        return E_INVALIDARG;

    if (Length > 4096)
        return NETP_E_BUFFER_FULL;

    if (!bev_)
        return NETP_E_INVALID_STATE;

    if (bufferevent_write(bev_, Packet, Length) == -1)
    {
        hr = NETP_E_SEND_FAILED;
    }

    return hr;
}

HRESULT ConnectionImpl::SetSession(ISession* piSession)
{
    if (!piSession)
        return E_INVALIDARG;

    if (piConnectionCb_)
        return NETP_E_INVALID_STATE;

    piConnectionCb_ = piSession;
    return S_OK;
}



