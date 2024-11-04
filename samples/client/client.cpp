#include <windows.h>
#include <tchar.h>
#include <initguid.h>
#include <netp.h>
#include <netphlp.h>
#include <iostream>
using namespace std;

#include <protocol.h>

using netp::IEventHandler;
using netp::IConnection;
using netp::IConnectionCallback;


class ClientSession : public IConnectionCallback
{
public:
    ClientSession(IConnection* piConnection)
        : piConn_(piConnection)
    {
    }

    ~ClientSession()
    {
    }



    // netp::IConnectionCallback
    virtual HRESULT OnPacket(BYTE* Packet, SIZE_T Length)
    {
        PACKET_HEADER *pkt = reinterpret_cast<PACKET_HEADER *>(Packet);

        switch (pkt->command)
        {
        case COMMAND_MESSAGE:
        {
            CMD_MESSAGE *pCmdMessage = reinterpret_cast<CMD_MESSAGE *>(Packet);
            std::cout << "ReceiveMessage:" << pCmdMessage->text << std::endl;

            break;
        }
        default:
            std::cout << "unsupported command :" << pkt->command << std::endl;

        }
        return S_OK;
    }


    HRESULT SendMsg(const char* text)
    {
        CMD_MESSAGE cmd;

        cmd.hdr.length = sizeof(CMD_MESSAGE);
        cmd.hdr.command = COMMAND_MESSAGE;

        strcpy(cmd.text,text);

        return piConn_->SendData((BYTE*)&cmd,sizeof(cmd));
        
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

            GetSession()->SendMsg("what's your name?");

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

    Client client;
    hr = client.Initialize("127.0.0.1", 4567);

    hr = client.Start();




    getchar();
    

    return 0;
}