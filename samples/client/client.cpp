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

using netp::IEventHandler;
using netp::IConnection;
using netp::ISession;


class ClientSession : public ISession
{
public:
    ClientSession(IConnection* piConnection)
        : piConn_(piConnection)
    {
    }

    ~ClientSession()
    {
    }



    // netp::ISession
    virtual HRESULT OnPacket(BYTE* Packet, SIZE_T Length)
    {
        PACKET_HEADER *pkt = reinterpret_cast<PACKET_HEADER *>(Packet);

        switch (pkt->command)
        {
        case COMMAND_MESSAGE:
        {
            CMD_MESSAGE *pCmdMessage = reinterpret_cast<CMD_MESSAGE *>(Packet);
            std::cout << pCmdMessage->text << std::endl;
            break;
        }
        default:
            std::cout << "unsupported command :" << pkt->command << std::endl;

        }
        return S_OK;
    }

    HRESULT Login(const char* name, const char* pwd)
    {
        CMD_LOIGN cmd;

        cmd.hdr.length = sizeof(CMD_LOIGN);
        cmd.hdr.command = COMMAND_LOGIN;

        StringCchCopyA(cmd.username,32,name);
        StringCchCopyA(cmd.password,32,pwd);

        return piConn_->SendPacket((BYTE*)&cmd,sizeof(cmd));        
    }


    HRESULT Query()
    {
        CMD_QUERY cmd;

        cmd.hdr.length = sizeof(CMD_QUERY);
        cmd.hdr.command = COMMAND_QUERY;

        return piConn_->SendPacket((BYTE*)&cmd,sizeof(cmd));        
    }


private:
    IConnection* piConn_;

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

    virtual HRESULT OnEvent(NETP_EVENT_ID eEventId, ULONG_PTR ulParam)
    {
        HRESULT hr = S_OK;

        hr = NetpClientImpl<Client, ClientSession>::OnEvent(eEventId, ulParam);

        switch (eEventId)
        {
        case EVENT_NEW_CONNECTION:

            break;
        
        default:
            break;
        }

        return hr;
    }

private:
};


Client client;
// Function to process commands
void processCommand(const std::string &command)
{
    if (command == "start")
    {
        HRESULT hr;
        hr = client.Initialize("127.0.0.1", 4567);
        hr = client.Start();
    }
    else if (command == "stop")
    {
        HRESULT hr;
        hr = client.Stop();
    }
    else if (command == "login")
    {
        std::string username;  
        std::string password;

        std::cout << "username:";  
        std::getline(std::cin, username);  
        std::cout << "password:";  
        std::getline(std::cin, password);  


        client.GetSession()->Login(username.c_str(), password.c_str());
    }
    else if (command == "query")
    {
        client.GetSession()->Query();
    }
    else if (command == "help")
    {
        std::cout<< "command: start, stop ,help"<<std::endl;
    }
    else
    {
    }
}

int main() {  
    std::cout << "console client" << std::endl;  
    std::cout << "Type 'exit' the application." << std::endl;  
  
    std::string command;  
    while (true) {  
        // Prompt user for input  
        std::cout << ">";  
        std::getline(std::cin, command);  
  
        // Process the command  
        processCommand(command);  
  
        // Exit the loop if the command is 'exit'  
        if (command == "exit") {  
            break;  
        }  
    }  
  
    return 0;  
}  
