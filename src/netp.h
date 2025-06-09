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
    EVENT_BIND_ERROR,           // Failed to bind to port
    EVENT_NEW_CONNECTION,       // New client connected
    EVENT_CONNECTION_CLOSE,     // Connection closed
    EVENT_SEND_COMPLETE,        // Send operation completed
    EVENT_BUFFER_FULL,         // Send buffer is full
    EVENT_BUFFER_READY,        // Send buffer has space
    EVENT_NETWORK_ERROR,       // Network error occurred
    EVENT_MEMORY_ERROR,        // Memory allocation failed
} NETP_EVENT_ID;

typedef enum _NETP_ERROR_CODE
{
    NETP_E_SUCCESS = 0,
    NETP_E_BIND_FAILED,
    NETP_E_CONNECTION_FAILED,
    NETP_E_SEND_FAILED,
    NETP_E_RECEIVE_FAILED,
    NETP_E_BUFFER_FULL,
    NETP_E_INVALID_STATE,
    NETP_E_MEMORY_ERROR,
} NETP_ERROR_CODE;

typedef struct _NETP_BUFFER_STATUS
{
    SIZE_T sendBufferSize;      // Total send buffer size
    SIZE_T sendBufferUsed;      // Used send buffer size
    SIZE_T receiveBufferSize;   // Total receive buffer size
    SIZE_T receiveBufferUsed;   // Used receive buffer size
} NETP_BUFFER_STATUS;

typedef struct _NETP_STATISTICS
{
    ULONG64 totalBytesSent;     // Total bytes sent
    ULONG64 totalBytesReceived; // Total bytes received
    ULONG64 sendCount;          // Number of send operations
    ULONG64 receiveCount;       // Number of receive operations
    ULONG64 errorCount;         // Number of errors
} NETP_STATISTICS;

// Server configuration structure
typedef struct _NETP_SERVER_CONFIG
{
    WORD port;                  // Listen port
    SIZE_T maxConnections;      // Maximum allowed connections
    SIZE_T defaultSendBuffer;   // Default send buffer size
    SIZE_T defaultRecvBuffer;   // Default receive buffer size
    BOOL reuseAddr;            // Enable SO_REUSEADDR
    BOOL tcpNoDelay;           // Enable TCP_NODELAY
    struct {
        BOOL enabled;           // Enable keep-alive
        ULONG idleTime;         // Idle time in milliseconds
        ULONG interval;         // Probe interval in milliseconds
    } keepAlive;
} NETP_SERVER_CONFIG;

// Client configuration structure
typedef struct _NETP_CLIENT_CONFIG
{
    CHAR ipAddress[46];        // Target IP address (IPv4 or IPv6)
    WORD port;                 // Target port
    SIZE_T sendBufferSize;     // Send buffer size
    SIZE_T recvBufferSize;     // Receive buffer size
    BOOL tcpNoDelay;          // Enable TCP_NODELAY
    struct {
        BOOL enabled;          // Enable keep-alive
        ULONG idleTime;        // Idle time in milliseconds
        ULONG interval;        // Probe interval in milliseconds
    } keepAlive;
    ULONG connectTimeout;      // Connection timeout in milliseconds
} NETP_CLIENT_CONFIG;

namespace netp
{


class ISession
{
public:
    virtual ~ISession() {}
    virtual HRESULT OnPacket(const BYTE* Packet, SIZE_T Length) = 0;
    virtual HRESULT OnError(NETP_ERROR_CODE ErrorCode, LPCSTR ErrorMessage) = 0;
};



class IConnection
{
public:
    virtual ~IConnection() {}
    
    // Session management
    virtual HRESULT SetSession(ISession* piSession) = 0;
    virtual ISession* GetSession() = 0;

    // Basic operations
    virtual HRESULT SendPacket(const BYTE* Packet, SIZE_T Length) = 0;
    
    // Connection info
    virtual HRESULT GetRemoteIP(char* ipaddr, SIZE_T size) = 0;
    virtual HRESULT GetLocalIP(char* ipaddr, SIZE_T size) = 0;
    virtual HRESULT IsConnected(BOOL* pConnected) = 0;
    
    // Buffer management
    virtual HRESULT SetSendBufferSize(SIZE_T size) = 0;
    virtual HRESULT SetReceiveBufferSize(SIZE_T size) = 0;
    virtual HRESULT GetBufferStatus(NETP_BUFFER_STATUS* pStatus) = 0;
    
    // Flow control
    virtual HRESULT Pause() = 0;
    virtual HRESULT Resume() = 0;
    virtual HRESULT IsPaused(BOOL* pPaused) = 0;
    
    // Statistics and diagnostics
    virtual HRESULT GetStatistics(NETP_STATISTICS* pStats) = 0;
    virtual HRESULT GetLastError(NETP_ERROR_CODE* pErrorCode, char* errorMsg, SIZE_T msgSize) = 0;
};





class IEventHandler
{
public:
    virtual ~IEventHandler() {}
    virtual HRESULT OnEvent(NETP_EVENT_ID eEventId, 
                          ULONG_PTR ulParam,
                          NETP_ERROR_CODE errorCode,
                          LPCSTR errorMessage) = 0;
};



class IServer
{
public:
    virtual ~IServer() {}

    // Initialization and control
    virtual HRESULT Initialize(const NETP_SERVER_CONFIG* pConfig, IEventHandler* piEventHandler) = 0;
    virtual HRESULT Start() = 0;
    virtual HRESULT Stop() = 0;
    
    // Runtime configuration
    virtual HRESULT SetMaxConnections(SIZE_T maxConnections) = 0;
    virtual HRESULT GetActiveConnections(SIZE_T* pCount) = 0;
    
    // Socket options
    virtual HRESULT SetKeepAlive(BOOL enable, ULONG idleTime, ULONG interval) = 0;
    virtual HRESULT SetTcpNoDelay(BOOL enable) = 0;
    virtual HRESULT SetReuseAddr(BOOL enable) = 0;
};



class IClient
{
public:
    virtual ~IClient() {}

    // Initialization and control
    virtual HRESULT Initialize(const NETP_CLIENT_CONFIG* pConfig, IEventHandler* piEventHandler) = 0;
    virtual HRESULT Start() = 0;
    virtual HRESULT Stop() = 0;
    
    // Connection management
    virtual HRESULT Reconnect() = 0;
    virtual HRESULT IsConnected(BOOL* pConnected) = 0;
    
    // Configuration
    virtual HRESULT SetKeepAlive(BOOL enable, ULONG idleTime, ULONG interval) = 0;
    virtual HRESULT SetTcpNoDelay(BOOL enable) = 0;
};





} // @namespace netp



#endif// _NETP_H_