#include "echo_packet.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>

std::mutex cout_mutex;
void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << msg << std::endl;
}

void runServer() {
    // Create and configure server
    auto server = netp::createServer();
    
    server->setConnectionHandler([](netp::ConnectionPtr conn) {
        // Wait a short moment for connection setup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::string remote_addr = conn->getRemoteAddress();
        uint16_t remote_port = conn->getRemotePort();
        
        if (remote_addr.empty() || remote_addr == "unknown") {
            log("New connection established but peer information not available");
        } else {
            log("New connection from " + remote_addr + ":" + std::to_string(remote_port));
        }

        // Set up packet handler
        conn->setPacketHandler([conn](const std::vector<uint8_t>& data) {
            std::string addr = conn->getRemoteAddress();
            uint16_t port = conn->getRemotePort();
            log("Server received data from " + addr + ":" + std::to_string(port) + ", size: " + std::to_string(data.size()));
            
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
        conn->setErrorHandler([conn](const std::string& error) {
            std::string addr = conn->getRemoteAddress();
            uint16_t port = conn->getRemotePort();
            log("Server connection error from " + addr + ":" + std::to_string(port) + " - " + error);
        });

        // Set up disconnect handler
        conn->setDisconnectHandler([conn]() {
            std::string addr = conn->getRemoteAddress();
            uint16_t port = conn->getRemotePort();
            log("Client disconnected: " + addr + ":" + std::to_string(port));
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
    bool should_run = true;
    const auto reconnect_delay = std::chrono::seconds(5);
    int reconnect_count = 0;
    
    while (should_run) {
        if (client->connect("127.0.0.1", 12345)) {
            log("Connected to server");
            reconnect_count = 0;  // Reset reconnect counter on successful connection
            
            auto conn = client->getConnection();
            
            // Wait a short moment for connection setup
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

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
            conn->setDisconnectHandler([&should_run]() {
                log("Disconnected from server, will attempt to reconnect");
            });

            log("Type your messages (type 'quit' to exit, 'reconnect' to force reconnection):");
            
            std::string input;
            while (client->isConnected()) {
                std::cout << "> ";
                std::getline(std::cin, input);
                
                if (input == "quit") {
                    log("Closing client...");
                    should_run = false;
                    break;
                }
                
                if (input == "reconnect") {
                    log("Forcing reconnection...");
                    break;
                }
                
                if (!input.empty()) {
                    EchoPacket packet(input);
                    if (conn->sendPacket(packet)) {
                        log("Client sent: " + input);
                    } else {
                        log("Client failed to send: " + input);
                        break;  // Break to trigger reconnection
                    }
                }
                
                // Small delay to prevent flooding
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            client->disconnect();
            
            if (!should_run) {
                break;  // Exit the reconnection loop if quitting
            }
            
            // Add delay before reconnection attempt
            std::this_thread::sleep_for(reconnect_delay);
            
        } else {
            reconnect_count++;
            log("Connection attempt " + std::to_string(reconnect_count) + 
                " failed, retrying in " + std::to_string(reconnect_delay.count()) + " seconds...");
            std::this_thread::sleep_for(reconnect_delay);
        }
    }
    
    log("Client terminated");
}

void printUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " server\n"
              << "  " << program << " client\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
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
            printUsage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 