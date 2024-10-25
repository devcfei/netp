#include "netpp.h"
using netp::IServer;
using netp::IClient;



HRESULT NetpCreateInstance(REFIID iftype , void **ppi)
{
    HRESULT hr = E_NOINTERFACE;

    if(iftype == NETP_ISERVER)
    {

        IServer *pi = new ServerImpl();

        if(pi)
        {
            *ppi = pi;
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if(iftype == NETP_ICLIENT)
    {
        IClient *pi = new ClientImpl();

        if(pi)
        {
            *ppi = pi;   
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}