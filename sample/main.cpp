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

        // Track number of responses received
        std::atomic<size_t> responses_received{0};

        // Set up packet handler
        conn->setPacketHandler([&responses_received](const std::vector<uint8_t>& data) {
            log("Client received raw data of size: " + std::to_string(data.size()));
            
            EchoPacket packet;
            if (packet.deserialize(data.data(), data.size())) {
                log("Client received echo: " + packet.getMessage());
                responses_received++;
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

        // Send test messages
        std::vector<std::string> messages = {
            "Hello, server!",
            "How are you?",
            "Testing packet communication",
            "Goodbye!"
        };
        
        size_t total_messages = messages.size();
        log("Client will send " + std::to_string(total_messages) + " messages");
        
        for (const auto& msg : messages) {
            EchoPacket packet(msg);
            auto serialized = packet.serialize();
            log("Sending message '" + msg + "' (serialized size: " + std::to_string(serialized.size()) + " bytes)");
            
            if (conn->sendPacket(packet)) {
                log("Client sent: " + msg);
            } else {
                log("Client failed to send: " + msg);
            }
            // Wait a bit longer between messages to ensure proper handling
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        
        // Wait for all responses with a timeout
        log("Waiting for responses...");
        auto start_time = std::chrono::steady_clock::now();
        while (responses_received < total_messages) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Timeout after 10 seconds
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 10) {
                log("Timeout waiting for responses. Received " + 
                    std::to_string(responses_received) + " out of " + 
                    std::to_string(total_messages) + " responses.");
                break;
            }
        }
        
        // Wait a bit before disconnecting to ensure last messages are processed
        std::this_thread::sleep_for(std::chrono::seconds(1));
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