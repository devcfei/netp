#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <initguid.h>
#include <netp.h>
#include <netphlp.h>
#include <iostream>
#include <string>

using namespace std;

#include <protocol.h>

class ClientSession : public ISession
{
public:
    ClientSession(IConnection *piConnection)
        : piConn_(piConnection)
    {
    }

    ~ClientSession()
    {
    }

    // netp::ISession
    virtual HRESULT OnPacket(const BYTE *Packet, SIZE_T Length)
    {
        const PACKET_HEADER *pkt = reinterpret_cast<const PACKET_HEADER *>(Packet);

        switch (pkt->command)
        {
        case COMMAND_MESSAGE:
        {
            const CMD_MESSAGE *pCmdMessage = reinterpret_cast<const CMD_MESSAGE *>(Packet);
            std::cout << pCmdMessage->text << std::endl;
            break;
        }
        default:
            std::cout << "unsupported command :" << pkt->command << std::endl;
        }
        return S_OK;
    }

    virtual HRESULT OnError(NETP_ERROR_CODE ErrorCode, LPCSTR ErrorMessage)
    {
        std::cout << "OnError: " << ErrorCode << " " << ErrorMessage << std::endl;
        return S_OK;
    }

    HRESULT Login(const char *name, const char *pwd)
    {
        CMD_LOIGN cmd;

        cmd.hdr.length = sizeof(CMD_LOIGN);
        cmd.hdr.command = COMMAND_LOGIN;

        StringCchCopyA(cmd.username, 32, name);
        StringCchCopyA(cmd.password, 32, pwd);

        return piConn_->SendPacket((BYTE *)&cmd, sizeof(cmd));
    }

    HRESULT Query()
    {
        CMD_QUERY cmd;

        cmd.hdr.length = sizeof(CMD_QUERY);
        cmd.hdr.command = COMMAND_QUERY;

        return piConn_->SendPacket((BYTE *)&cmd, sizeof(cmd));
    }

private:
    IConnection *piConn_;
};

class Client : public NetpClientImpl<Client, ClientSession>
{
public:
    Client()
    {
    }
    ~Client()
    {
    }

    HRESULT OnConnected(IConnection* piConn)
    {
        std::cout << "Connected to server!" << std::endl;
        return S_OK;
    }

    HRESULT OnDisconnected(IConnection* piConn)
    {
        std::cout << "Disconnected from server!" << std::endl;
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

Client client;

// Function to process commands
void processCommand(const std::string& command)
{
    if (command == "start")
    {
        HRESULT hr;
        NETP_CLIENT_CONFIG config = {0};
        StringCchCopyA(config.ipAddress, sizeof(config.ipAddress), "127.0.0.1");
        config.port = 4567;
        config.sendBufferSize = 65536;
        config.recvBufferSize = 65536;
        config.tcpNoDelay = TRUE;
        config.keepAlive.enabled = TRUE;
        config.keepAlive.idleTime = 60000;  // 60 seconds
        config.keepAlive.interval = 1000;   // 1 second
        config.connectTimeout = 5000;       // 5 seconds

        hr = client.Initialize(&config);
        if (SUCCEEDED(hr))
        {
            hr = client.Start();
            if (FAILED(hr))
            {
                std::cout << "Failed to start client" << std::endl;
            }
        }
        else
        {
            std::cout << "Failed to initialize client" << std::endl;
        }
    }
    else if (command == "stop")
    {
        HRESULT hr = client.Stop();
        if (FAILED(hr))
        {
            std::cout << "Failed to stop client" << std::endl;
        }
    }
    else if (command == "login")
    {
        std::string username;
        std::string password;

        std::cout << "username:";
        std::getline(std::cin, username);
        std::cout << "password:";
        std::getline(std::cin, password);

        ClientSession* pSession = client.GetSession();
        if (pSession)
        {
            pSession->Login(username.c_str(), password.c_str());
        }
        else
        {
            std::cout << "Not connected to server" << std::endl;
        }
    }
    else if (command == "query")
    {
        ClientSession* pSession = client.GetSession();
        if (pSession)
        {
            pSession->Query();
        }
        else
        {
            std::cout << "Not connected to server" << std::endl;
        }
    }
    else if (command == "help")
    {
        std::cout << "Commands available:" << std::endl;
        std::cout << "  start  - Connect to server" << std::endl;
        std::cout << "  stop   - Disconnect from server" << std::endl;
        std::cout << "  login  - Login to server" << std::endl;
        std::cout << "  query  - Query server" << std::endl;
        std::cout << "  help   - Show this help" << std::endl;
        std::cout << "  exit   - Exit application" << std::endl;
    }
}

int main()
{
    std::cout << "console client" << std::endl;
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
