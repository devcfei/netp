#include <iostream>
#include <thread>
#include <map>
#include <netp.h>
#include "custom_packet.hpp"

using namespace netp;

class CustomServer {
public:
    CustomServer(const std::string& ip, uint16_t port) : port_(port) {
        server_ = createServer();
        server_->setConnectionHandler([this](ConnectionPtr conn) { onAccept(conn); });
        server_->setErrorHandler([](const std::string& error) {
            std::cout << "Server error: " << error << std::endl;
        });
    }

    bool start() {
        return server_->start(port_);
    }

    void stop() {
        server_->stop();
    }

protected:
    void onAccept(ConnectionPtr socket) {
        std::cout << "New client connected from " << socket->getRemoteAddress() << std::endl;
        clients_[socket] = 0; // Initialize with ID 0, will be set when client sends HELLO

        socket->setPacketHandler([this, socket](const std::vector<uint8_t>& data) {
            onReceive(socket, data);
        });

        socket->setDisconnectHandler([this, socket]() {
            onClose(socket);
        });
    }

    void onReceive(ConnectionPtr socket, const std::vector<uint8_t>& data) {
        CustomPacket packet;
        
        // Try to deserialize the packet
        if (!packet.deserialize(data.data(), data.size())) {
            std::cout << "Invalid packet received, terminating client" << std::endl;
            
            // Send KILL command before termination
            CustomPacket killPacket(0, CustomCommand::KILL);
            std::vector<uint8_t> killData = {'I', 'n', 'v', 'a', 'l', 'i', 'd', ' ', 'p', 'a', 'c', 'k', 'e', 't'};
            killPacket.setData(killData);
            socket->sendPacket(killPacket);
            
            socket->disconnect();
            clients_.erase(socket);
            return;
        }

        // Process the packet based on command
        switch (packet.getCommand()) {
            case CustomCommand::HELLO: {
                // Store client ID
                clients_[socket] = packet.getId();
                std::cout << "Client " << packet.getId() << " registered" << std::endl;
                
                // Send acknowledgment
                CustomPacket response(packet.getId(), CustomCommand::HELLO);
                socket->sendPacket(response);
                break;
            }
            
            case CustomCommand::DATA: {
                // Handle data differently based on client ID
                uint32_t clientId = packet.getId();
                std::cout << "Received DATA from client " << clientId << std::endl;
                
                CustomPacket response(clientId, CustomCommand::DATA);
                std::vector<uint8_t> responseData;
                
                // Different behavior based on client ID
                if (clientId % 2 == 0) {
                    // For even IDs, echo the data back
                    responseData = packet.getData();
                } else {
                    // For odd IDs, reverse the data
                    responseData = packet.getData();
                    std::reverse(responseData.begin(), responseData.end());
                }
                
                response.setData(responseData);
                socket->sendPacket(response);
                break;
            }
            
            default: {
                // Handle undefined command
                std::cout << "Undefined command received: " << static_cast<uint32_t>(packet.getCommand()) << std::endl;
                
                CustomPacket warning(packet.getId(), CustomCommand::WARNING);
                std::vector<uint8_t> warningData = {'U', 'n', 'd', 'e', 'f', 'i', 'n', 'e', 'd', ' ', 'c', 'm', 'd'};
                warning.setData(warningData);
                socket->sendPacket(warning);
                break;
            }
        }
    }

    void onClose(ConnectionPtr socket) {
        std::cout << "Client " << clients_[socket] << " disconnected" << std::endl;
        clients_.erase(socket);
    }

private:
    std::unique_ptr<Server> server_;
    uint16_t port_;
    std::map<ConnectionPtr, uint32_t> clients_;
};

class CustomClient {
public:
    CustomClient(uint32_t id) : id_(id), should_retry_(false) {
        client_ = createClient();
    }

    bool connect(const std::string& ip, uint16_t port) {
        last_ip_ = ip;
        last_port_ = port;
        
        if (!client_->connect(ip, port)) {
            std::cout << "Failed to initiate connection to " << ip << ":" << port << std::endl;
            return false;
        }

        auto conn = client_->getConnection();
        if (!conn) return false;

        // Set up error handler for connection failures
        conn->setErrorHandler([this](const std::string& error) {
            std::cout << "Client " << id_ << " error: " << error << std::endl;
            
            if (should_retry_) {
                std::cout << "Will retry connection in 5 seconds..." << std::endl;
                std::thread([this]() {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    if (should_retry_) {
                        std::cout << "Retrying connection..." << std::endl;
                        connect(last_ip_, last_port_);
                    }
                }).detach();
            }
        });

        conn->setPacketHandler([this](const std::vector<uint8_t>& data) {
            onReceive(data);
        });

        conn->setDisconnectHandler([this]() {
            onClose();
        });

        // Don't call onConnect here - it will be called when connection is actually established
        return true;
    }

    void disconnect() {
        should_retry_ = false;  // Stop retry attempts
        if (client_ && client_->isConnected()) {
            client_->disconnect();
        }
    }

    void sendHello() {
        if (!client_ || !client_->isConnected()) {
            std::cout << "Cannot send HELLO: not connected" << std::endl;
            return;
        }
        CustomPacket packet(id_, CustomCommand::HELLO);
        sendPacket(packet);
    }

    void sendData(const std::vector<uint8_t>& data) {
        if (!client_ || !client_->isConnected()) {
            std::cout << "Cannot send DATA: not connected" << std::endl;
            return;
        }
        CustomPacket packet(id_, CustomCommand::DATA);
        packet.setData(data);
        sendPacket(packet);
    }

    void sendInvalidCommand() {
        if (!client_ || !client_->isConnected()) {
            std::cout << "Cannot send command: not connected" << std::endl;
            return;
        }
        CustomPacket packet(id_, CustomCommand::UNDEFINED);
        sendPacket(packet);
    }

    void sendInvalidPacket() {
        if (!client_ || !client_->isConnected()) {
            std::cout << "Cannot send packet: not connected" << std::endl;
            return;
        }
        // Send raw bytes that don't follow the protocol
        std::vector<uint8_t> invalid = {1, 2, 3, 4};
        if (client_ && client_->isConnected()) {
            client_->getConnection()->sendRawData(invalid);
        }
    }

protected:
    void sendPacket(const CustomPacket& packet) {
        if (client_ && client_->isConnected()) {
            client_->getConnection()->sendPacket(packet);
        }
    }

    void onReceive(const std::vector<uint8_t>& data) {
        CustomPacket packet;
        if (!packet.deserialize(data.data(), data.size())) {
            std::cout << "Client received invalid packet" << std::endl;
            return;
        }

        switch (packet.getCommand()) {
            case CustomCommand::HELLO:
                std::cout << "Client " << id_ << " received HELLO response" << std::endl;
                break;
            
            case CustomCommand::DATA:
                std::cout << "Client " << id_ << " received DATA response" << std::endl;
                break;
            
            case CustomCommand::WARNING:
                std::cout << "Client " << id_ << " received WARNING: ";
                for (uint8_t byte : packet.getData()) {
                    std::cout << static_cast<char>(byte);
                }
                std::cout << std::endl;
                break;
            
            case CustomCommand::KILL:
                std::cout << "Client " << id_ << " received KILL command: ";
                for (uint8_t byte : packet.getData()) {
                    std::cout << static_cast<char>(byte);
                }
                std::cout << std::endl;
                disconnect();
                break;
        }
    }

    void onConnect() {
        std::cout << "Client " << id_ << " connected" << std::endl;
        sendHello();
    }

    void onClose() {
        std::cout << "Client " << id_ << " disconnected" << std::endl;
    }

private:
    uint32_t id_;
    std::unique_ptr<Client> client_;
    bool should_retry_;
    std::string last_ip_;
    uint16_t last_port_;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " [server|client] [client_id]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    const std::string ip = "127.0.0.1";
    const uint16_t port = 12345;

    if (mode == "server") {
        CustomServer server(ip, port);
        if (!server.start()) {
            std::cout << "Failed to start server" << std::endl;
            return 1;
        }
        
        std::cout << "Server started on " << ip << ":" << port << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        
        server.stop();
    }
    else if (mode == "client") {
        if (argc < 3) {
            std::cout << "Client ID required" << std::endl;
            return 1;
        }

        uint32_t clientId = std::stoul(argv[2]);
        CustomClient client(clientId);
        
        if (!client.connect(ip, port)) {
            std::cout << "Failed to connect to server" << std::endl;
            return 1;
        }

        // Test different scenarios
        std::cout << "1. Send data" << std::endl;
        std::cout << "2. Send invalid command" << std::endl;
        std::cout << "3. Send invalid packet" << std::endl;
        std::cout << "4. Exit" << std::endl;

        while (true) {
            int choice;
            std::cout << "\nEnter choice: ";
            std::cin >> choice;

            switch (choice) {
                case 1: {
                    std::vector<uint8_t> testData = {'H', 'e', 'l', 'l', 'o'};
                    client.sendData(testData);
                    break;
                }
                case 2:
                    client.sendInvalidCommand();
                    break;
                case 3:
                    client.sendInvalidPacket();
                    break;
                case 4:
                    return 0;
                default:
                    std::cout << "Invalid choice" << std::endl;
            }
        }
    }
    else {
        std::cout << "Invalid mode. Use 'server' or 'client'" << std::endl;
        return 1;
    }

    return 0;
} 