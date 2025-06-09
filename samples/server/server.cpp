#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <initguid.h>
#include <iostream>
#include <string>

using namespace std;
#include <netp.h>
#include <netphlp.h>

#include <protocol.h>

class ServerSession : public ISession
{
public:
    ServerSession(IConnection *piConnection)
        : piConn_(piConnection)
    {
    }
    ~ServerSession()
    {
    }
    // netp::ISession
    virtual HRESULT OnPacket(const BYTE *Packet, SIZE_T Length)
    {
        const PACKET_HEADER *pkt = reinterpret_cast<const PACKET_HEADER *>(Packet);

        switch (pkt->command)
        {
        case COMMAND_LOGIN:
        {
            const CMD_LOIGN *pCmdMessage = reinterpret_cast<const CMD_LOIGN *>(Packet);

            StringCchCopyA(username_, 32, pCmdMessage->username);
            StringCchCopyA(password_, 32, pCmdMessage->password);

            std::cout << "COMMAND_LOGIN" << std::endl;
            CHAR text[64];
            StringCchPrintfA(text, 64, "%s login", username_);
            SendMsg(text);

            break;
        }
        case COMMAND_QUERY:
        {
            std::cout << "COMMAND_QUERY" << std::endl;
            CHAR text[64];
            StringCchPrintfA(text, 64, "username: %s password %s", username_, password_);
            SendMsg(text);

            break;
        }

        default:
            std::cout << "unsupported command :" << pkt->command << std::endl;
        }

        return S_OK;
    }

    HRESULT OnError(NETP_ERROR_CODE ErrorCode, LPCSTR ErrorMessage)
    {
        std::cout << "OnError: " << ErrorCode << " " << ErrorMessage << std::endl;
        return S_OK;
    }


private:
    IConnection *piConn_;

    HRESULT SendMsg(const char *text)
    {
        CMD_MESSAGE cmd;

        cmd.hdr.length = sizeof(CMD_MESSAGE);
        cmd.hdr.command = COMMAND_MESSAGE;

        StringCchCopyA(cmd.text, 64, text);

        return piConn_->SendPacket((BYTE *)&cmd, sizeof(cmd));
    }

    char username_[32];
    char password_[32];
};

class Server : public NetpServerImpl<Server, ServerSession>
{
public:
    Server()
    {
    }
    ~Server()
    {
    }

    HRESULT OnClientConnected(IConnection* piConn)
    {
        char ipaddr[46];
        piConn->GetRemoteIP(ipaddr, sizeof(ipaddr));
        std::cout << "New client connected from " << ipaddr << std::endl;
        return S_OK;
    }

    HRESULT OnClientDisconnected(IConnection* piConn)
    {
        std::cout << "Client disconnected" << std::endl;
        return S_OK;
    }

    HRESULT OnServerError(NETP_ERROR_CODE errorCode, LPCSTR errorMessage)
    {
        std::cout << "Server error: " << errorMessage << std::endl;
        return S_OK;
    }

    HRESULT OnCustomEvent(NETP_EVENT_ID eEventId, 
                         ULONG_PTR ulParam,
                         NETP_ERROR_CODE errorCode,
                         LPCSTR errorMessage)
    {
        return S_OK;
    }
};

Server server;

// Function to process commands
void processCommand(const std::string& command)
{
    if (command == "start")
    {
        HRESULT hr;
        NETP_SERVER_CONFIG config = {0};
        config.port = 4567;
        config.maxConnections = 1000;
        config.defaultSendBuffer = 65536;
        config.defaultRecvBuffer = 65536;
        config.reuseAddr = TRUE;
        config.tcpNoDelay = TRUE;
        config.keepAlive.enabled = TRUE;
        config.keepAlive.idleTime = 60000;  // 60 seconds
        config.keepAlive.interval = 1000;   // 1 second

        hr = server.Initialize(&config);
        if (SUCCEEDED(hr))
        {
            hr = server.Start();
            if (FAILED(hr))
            {
                std::cout << "Failed to start server" << std::endl;
            }
            else
            {
                std::cout << "Server started on port " << config.port << std::endl;
            }
        }
        else
        {
            std::cout << "Failed to initialize server" << std::endl;
        }
    }
    else if (command == "stop")
    {
        HRESULT hr = server.Stop();
        if (FAILED(hr))
        {
            std::cout << "Failed to stop server" << std::endl;
        }
        else
        {
            std::cout << "Server stopped" << std::endl;
        }
    }
    else if (command == "help")
    {
        std::cout << "Commands available:" << std::endl;
        std::cout << "  start  - Start server" << std::endl;
        std::cout << "  stop   - Stop server" << std::endl;
        std::cout << "  help   - Show this help" << std::endl;
        std::cout << "  exit   - Exit application" << std::endl;
    }
}

int main()
{
    std::cout << "console server" << std::endl;
    std::cout << "Type 'exit' the application." << std::endl;

    std::string command;
    while (true)
    {
        // Prompt user for input
        std::cout << ">";
        std::getline(std::cin, command);

        // Process the command
        processCommand(command);

        // Exit the loop if the command is 'exit'
        if (command == "exit")
        {
            break;
        }
    }

    return 0;
}
