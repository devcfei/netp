#include "echo_packet.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

std::mutex cout_mutex;
void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << msg << std::endl;
}

void runServer() {
    // Create and configure server
    auto server = netp::createServer();
    
    server->setConnectionHandler([](netp::ConnectionPtr conn) {
        log("New connection from " + conn->getRemoteAddress() + ":" + std::to_string(conn->getRemotePort()));

        // Set up packet handler
        conn->setPacketHandler([conn](const std::vector<uint8_t>& data) {
            log("Server received raw data of size: " + std::to_string(data.size()));
            
            EchoPacket packet;
            if (packet.deserialize(data.data(), data.size())) {
                log("Server received message: " + packet.getMessage());
                
                // Create a new packet for response
                EchoPacket response(packet.getMessage());
                if (!conn->sendPacket(response)) {
                    log("Server failed to send echo response!");
                } else {
                    log("Server sent echo response: " + packet.getMessage());
                }
            } else {
                log("Server failed to deserialize packet! Raw data size: " + std::to_string(data.size()));
            }
        });

        // Set up error handler
        conn->setErrorHandler([](const std::string& error) {
            log("Server connection error: " + error);
        });

        // Set up disconnect handler
        conn->setDisconnectHandler([conn]() {
            log("Client disconnected: " + conn->getRemoteAddress() + ":" + std::to_string(conn->getRemotePort()));
        });
    });

    // Set up server error handler
    server->setErrorHandler([](const std::string& error) {
        log("Server error: " + error);
    });

    // Start server
    if (server->start(12345)) {
        log("Server started on port 12345");
        
        // Keep the server running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        log("Failed to start server");
    }
}

void runClient() {
    // Create and configure client
    auto client = netp::createClient();
    
    if (client->connect("127.0.0.1", 12345)) {
        log("Connected to server");
        
        auto conn = client->getConnection();

        // Set up packet handler
        conn->setPacketHandler([](const std::vector<uint8_t>& data) {
            log("Client received raw data of size: " + std::to_string(data.size()));
            
            EchoPacket packet;
            if (packet.deserialize(data.data(), data.size())) {
                log("Client received echo: " + packet.getMessage());
            } else {
                log("Client failed to deserialize response packet! Raw data size: " + std::to_string(data.size()));
            }
        });

        // Set up error handler
        conn->setErrorHandler([](const std::string& error) {
            log("Client connection error: " + error);
        });

        // Set up disconnect handler
        conn->setDisconnectHandler([]() {
            log("Disconnected from server");
        });

        log("Type your messages (type 'quit' to exit):");
        
        std::string input;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, input);
            
            if (input == "quit") {
                log("Closing client...");
                break;
            }
            
            if (!input.empty()) {
                EchoPacket packet(input);
                if (conn->sendPacket(packet)) {
                    log("Client sent: " + input);
                } else {
                    log("Client failed to send: " + input);
                }
            }
            
            // Small delay to prevent flooding
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Clean disconnect
        client->disconnect();
    } else {
        log("Failed to connect to server");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " [server|client]" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    
    try {
        if (mode == "server") {
            runServer();
        } else if (mode == "client") {
            runClient();
        } else {
            std::cerr << "Invalid mode. Use 'server' or 'client'" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 