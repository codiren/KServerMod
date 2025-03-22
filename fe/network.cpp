#include "pch.h"
#include "gameState.h"
#include <winsock2.h>  // Must be included BEFORE windows.h
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")  // Link against Winsock library

#include "network.h"

namespace network {
    std::atomic<bool> running(true);
    bool initializeWinsock() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    SOCKET connectToServer(const std::string& ip, int port) {
        SOCKET client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd == INVALID_SOCKET) {
            std::cerr << "Socket creation failed!\n";
            return INVALID_SOCKET;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed!\n";
            closesocket(client_fd);
            return INVALID_SOCKET;
        }

        return client_fd;
    }
    void receiveMessages(SOCKET client_fd) {
        char response[1024];
        while (running) {
            memset(response, 0, sizeof(response));
            int bytes_received = recv(client_fd, response, sizeof(response) - 1, 0);

            if (bytes_received <= 0) {
                std::cerr << "Server disconnected!\n";
                running = false;
                break;
            }

            gameState::setData(response);
            std::string data = gameState::getData();
            sendMessage(client_fd, data);
        }
    }
    void sendMessage(SOCKET client_fd, const std::string& message) {
        int bytes_sent = send(client_fd, message.c_str(), message.size(), 0);
        if (bytes_sent == SOCKET_ERROR) {
            std::cerr << "Failed to send message!\n";
        }
    }
    void cleanup(SOCKET client_fd) {
        closesocket(client_fd);
        WSACleanup();
    }
}
