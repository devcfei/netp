#ifndef _NETP_H_
#define _NETP_H_


#if !defined(_WINDOWS_)
#error "include <windows.h> before netp.h 
#endif


// {879c8cfa-fe57-4bf7-b92b-37d1927b37ef}
DEFINE_GUID(NETP_ISERVER, 0x879c8cfa, 0xfe57, 0x4bf7, 0xb9, 0x2b, 0x37, 0xd1, 0x92, 0x7b, 0x37, 0xef);

// {84d6027c-d0d7-427b-880a-1a232de6a44e}
DEFINE_GUID(NETP_ICLIENT, 0x84d6027c, 0xd0d7, 0x427b, 0x88, 0x0a, 0x1a, 0x23, 0x2d, 0xe6, 0xa4, 0x4e);


HRESULT NetpCreateInstance(REFIID iftype, void** ppi);


typedef enum _NETP_EVENT_ID
{
    EVENT_BIND_ERROR,
    EVENT_NEW_CONNECTION,
    EVENT_CONNECTION_CLOSE,
}NETP_EVENT_ID;

namespace netp
{


class IConnectionCallback
{
public:
    virtual HRESULT OnPacket(BYTE* Packet, SIZE_T Length) = 0;
};



class IConnection
{
public:
    virtual HRESULT SetConnectionCallback(IConnectionCallback* piConnectionCb) = 0;
    virtual IConnectionCallback* GetConnectionCallback() = 0;
    virtual HRESULT SendData(BYTE* Packet, SIZE_T Length ) = 0;
    virtual HRESULT GetRemoteIP(char *ipaddr, SIZE_T size) = 0;
};





class IEventHandler
{
public:
    virtual HRESULT OnEvent(NETP_EVENT_ID eEventId, ULONG_PTR ulParam) = 0;
};



class IServer
{
public:

    virtual HRESULT Initialize(WORD Port, IEventHandler *piEventHander) = 0;
    virtual HRESULT Start() = 0;

public:

};



class IClient
{
public:

    virtual HRESULT Initialize(const CHAR* lpszIPAddr, WORD Port, IEventHandler *piEventHander) = 0;
    virtual HRESULT Start() = 0;
    virtual HRESULT Stop() = 0;

public:

};





} // @namespace netp



#endif// _NETP_H_