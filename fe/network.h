#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

namespace network {
	extern std::atomic<bool> running;

	bool initializeWinsock();
	SOCKET connectToServer(const std::string& ip, int port);
	void receiveMessages(SOCKET client_fd);
	void sendMessage(SOCKET client_fd, const std::string& message);
	void cleanup(SOCKET client_fd);
}
#endif // NETWORK_H
