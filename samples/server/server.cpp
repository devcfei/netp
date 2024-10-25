#include <windows.h>
#include <tchar.h>
#include <initguid.h>
#include <iostream>
using namespace std;
#include <netp.h>
#include <netphlp.h>

using netp::IEventCallback;
using netp::IConnection;
using netp::IConnectionCallback;

#include <protocol.h>


class ServerSession : public IConnectionCallback
{
public:
    ServerSession(IConnection *piConnection)
        : piConn_(piConnection)
    {
    }
    ~ServerSession()
    {
    }
    // netp::IConnectionCallback
    virtual HRESULT OnPacket(BYTE *Packet, SIZE_T Length)
    {
        PACKET_HEADER *pkt = reinterpret_cast<PACKET_HEADER *>(Packet);

        switch (pkt->command)
        {
        case COMMAND_MESSAGE:
        {
            CMD_MESSAGE *pCmdMessage = reinterpret_cast<CMD_MESSAGE *>(Packet);
            std::cout << "ReceiveMessage:" << pCmdMessage->text << std::endl;

            SendMsg("my name is world!");

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

        return piConn_->SendData((BYTE*)&cmd,sizeof(cmd));
        
    }
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

int main()
{
    HRESULT hr;

    Server server;

    hr = server.Initialize(4567);

    hr = server.Start();

    getchar();


    return 1;
}