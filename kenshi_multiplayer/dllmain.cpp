#include "pch.h"
#include <windows.h>
#include <iostream>
#include "network.h"
#include "utils.h"
#include "gameState.h"
#include "commands.h"
#define PORT 8080

void dllmain() {
    utils::spawn_console();
    if (!gameState::gameWorld) { std::cerr << "Failed to locate GameWorldClass in memory!\n";return; }
    if (!network::initializeWinsock()) { std::cerr << "WSAStartup failed!\n";return; }

    std::cout << "Connecting to server.." << std::endl;
    SOCKET client_fd = network::connectToServer("127.0.0.1", PORT);
    if (client_fd != INVALID_SOCKET) {
        std::cout << "Connected to server!" << std::endl;
    }
    else {
        std::cout << "Failed to connect to server (restart the game to try again)." << std::endl;
    }
    
    gameState::scanHeap();
    gameState::init();
    std::thread(commands::commandsLoop).detach();
    std::cout << "[Console Ready]\n";
    std::thread(network::receiveMessages, client_fd).join();
    network::cleanup(client_fd);
}

DWORD WINAPI threadWrapper(LPVOID param) {
    dllmain();
    return 0;
}

extern "C" void __declspec(dllexport) dllStartPlugin(void) {
    CreateThread(NULL, 0, threadWrapper, 0, 0, 0);
}
