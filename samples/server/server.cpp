#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <initguid.h>
#include <iostream>  
#include <string> 


using namespace std;
#include <netp.h>
#include <netphlp.h>

using netp::IEventHandler;
using netp::IConnection;
using netp::ISession;

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
    virtual HRESULT OnPacket(BYTE *Packet, SIZE_T Length)
    {
        PACKET_HEADER *pkt = reinterpret_cast<PACKET_HEADER *>(Packet);

        switch (pkt->command)
        {
        case COMMAND_LOGIN:
        {
            CMD_LOIGN *pCmdMessage = reinterpret_cast<CMD_LOIGN *>(Packet);

            StringCchCopyA(username_,32, pCmdMessage->username);
            StringCchCopyA(password_,32, pCmdMessage->password);

            std::cout << "COMMAND_LOGIN" <<std::endl;
            CHAR text[64];
            StringCchPrintfA(text,64,"%s login",username_);
            SendMsg(text);


            break;
        }
        case COMMAND_QUERY:
        {
            std::cout << "COMMAND_QUERY" <<std::endl;
            CHAR text[64];
            StringCchPrintfA(text,64,"username: %s password %s",username_, password_);
            SendMsg(text);


            break;
        }

        default:
            std::cout << "unsupported command :" << pkt->command << std::endl;

        }

        return S_OK;
    }




private:
    IConnection* piConn_;

    HRESULT SendMsg(const char* text)
    {
        CMD_MESSAGE cmd;

        cmd.hdr.length = sizeof(CMD_MESSAGE);
        cmd.hdr.command = COMMAND_MESSAGE;

        strcpy(cmd.text,text);

        return piConn_->SendPacket((BYTE*)&cmd,sizeof(cmd));
        
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

    virtual HRESULT OnEvent(NETP_EVENT_ID eEventId, ULONG_PTR ulParam)
    {
        HRESULT hr = S_OK;
        hr = NetpServerImpl<Server, ServerSession>::OnEvent(eEventId, ulParam);
        
        switch (eEventId)
        {
        case EVENT_NEW_CONNECTION:
            std::cout<<"new connection!"<<std::endl;            
            break;
        
        default:
            break;
        }

        return hr;
    }

private:

};





  
Server server;

// Function to process commands
void processCommand(const std::string &command)
{
    if (command == "start")
    {
        HRESULT hr;
        hr = server.Initialize(4567);
        hr = server.Start();
    }
    else if (command == "stop")
    {
        HRESULT hr;
        hr = server.Stop();
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
    std::cout << "console server" << std::endl;  
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
