#include "netpp.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

std::atomic<bool> g_server_running{false};
std::atomic<int> g_connection_count{0};
std::atomic<int> g_disconnection_count{0};

void runServer() {
    auto server = netp::createServer();
    
    server->setConnectionHandler([](netp::ConnectionPtr conn) {
        g_connection_count++;
        std::cout << "Server: New connection #" << g_connection_count << " from " 
                  << conn->getRemoteAddress() << ":" << conn->getRemotePort() << std::endl;

        // Set up packet handler
        conn->setPacketHandler([conn](const std::vector<uint8_t>& data) {
            std::cout << "Server: Received " << data.size() << " bytes" << std::endl;
            // Echo back the data
            conn->sendRawData(data);
        });

        // Set up error handler
        conn->setErrorHandler([](const std::string& error) {
            std::cout << "Server: Connection error: " << error << std::endl;
        });

        // Set up disconnect handler
        conn->setDisconnectHandler([conn]() {
            g_disconnection_count++;
            std::cout << "Server: Client disconnected #" << g_disconnection_count 
                      << " (" << conn->getRemoteAddress() << ":" << conn->getRemotePort() << ")" << std::endl;
        });
    });

    server->setErrorHandler([](const std::string& error) {
        std::cout << "Server error: " << error << std::endl;
    });

    if (server->start(12345)) {
        std::cout << "Server started on port 12345" << std::endl;
        g_server_running = true;
        
        while (g_server_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        server->stop();
        std::cout << "Server stopped" << std::endl;
    } else {
        std::cout << "Failed to start server" << std::endl;
    }
}

void runClient(int client_id) {
    auto client = netp::createClient();
    int reconnect_attempts = 0;
    const int max_attempts = 20;
    
    std::cout << "Client " << client_id << ": Starting reconnection test" << std::endl;
    
    while (reconnect_attempts < max_attempts && g_server_running) {
        if (client->connect("127.0.0.1", 12345)) {
            std::cout << "Client " << client_id << ": Connected successfully" << std::endl;
            
            auto conn = client->getConnection();
            
            // Set up packet handler
            conn->setPacketHandler([](const std::vector<uint8_t>& data) {
                std::cout << "Client: Received echo of " << data.size() << " bytes" << std::endl;
            });

            // Set up error handler
            conn->setErrorHandler([](const std::string& error) {
                std::cout << "Client: Connection error: " << error << std::endl;
            });

            // Set up disconnect handler
            conn->setDisconnectHandler([]() {
                std::cout << "Client: Disconnected from server" << std::endl;
            });

            // Send a test message
            std::string test_msg = "Hello from client " + std::to_string(client_id);
            std::vector<uint8_t> data(test_msg.begin(), test_msg.end());
            
            if (conn->sendRawData(data)) {
                std::cout << "Client " << client_id << ": Sent test message" << std::endl;
            } else {
                std::cout << "Client " << client_id << ": Failed to send message" << std::endl;
            }
            
            // Wait a bit then disconnect
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            client->disconnect();
            
            // Wait before reconnecting
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
        } else {
            reconnect_attempts++;
            std::cout << "Client " << client_id << ": Connection attempt " << reconnect_attempts 
                      << " failed, retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    std::cout << "Client " << client_id << ": Finished after " << reconnect_attempts << " attempts" << std::endl;
}

int main() {
    std::cout << "=== Reconnection Test ===" << std::endl;
    
    // Start server in background
    std::thread server_thread(runServer);
    
    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Start multiple clients
    std::vector<std::thread> client_threads;
    for (int i = 1; i <= 3; ++i) {
        client_threads.emplace_back(runClient, i);
    }
    
    // Let the test run for 30 seconds
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    // Stop everything
    g_server_running = false;
    
    // Wait for threads to finish
    for (auto& thread : client_threads) {
        thread.join();
    }
    server_thread.join();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Total connections: " << g_connection_count << std::endl;
    std::cout << "Total disconnections: " << g_disconnection_count << std::endl;
    std::cout << "Test completed!" << std::endl;
    
    return 0;
} 