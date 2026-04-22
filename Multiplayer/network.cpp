#pragma once
#include "Registry.h"
#include "network_internal.h"
#include <functional>
#include <map>
#include <queue>
#include <shellapi.h>
#include <sstream>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// Networking globals
SOCKET clientSocket = INVALID_SOCKET;
std::queue<std::string> incomingMessages;
CRITICAL_SECTION msgLock;
bool networkingInitialized = false;
std::string myName = "Player";
std::string messageAccumulator;

struct NetCommand {
  NetHandler func;
  std::string templateStr;
  std::string prefix;
  std::vector<std::string> paramNames;
};

std::map<std::string, NetCommand> g_net_handlers;

struct Win32Lock {
  Win32Lock(CRITICAL_SECTION &cs) : m_cs(cs) { EnterCriticalSection(&m_cs); }
  ~Win32Lock() { LeaveCriticalSection(&m_cs); }
  CRITICAL_SECTION &m_cs;
};

void net_reg(const std::string &pattern, NetHandler handler) {
  NetCommand cmd;
  cmd.func = handler;
  cmd.templateStr = pattern;

  std::stringstream ss(pattern);
  std::string segment;
  std::vector<std::string> parts;
  while (ss >> segment)
    parts.push_back(segment);

  if (parts.empty())
    return;

  cmd.prefix = parts[0];

  for (size_t i = 1; i < parts.size(); ++i) {
    std::string p = parts[i];
    if (p.size() >= 2 && p[0] == '<' && p[p.size() - 1] == '>') {
      cmd.paramNames.push_back(p.substr(1, p.size() - 2));
    }
  }

  g_net_handlers[cmd.prefix] = cmd;
}

void InitNet() {
  net_reg("SET_FACTION: <id>", [](const NetArgs &args) {
    myName = args.get("id");
    c::log("[SYSTEM]: Assigned faction identity: " + myName);
  });
}

static void disconnectFromServer() {
  if (clientSocket != INVALID_SOCKET) {
    closesocket(clientSocket);
    clientSocket = INVALID_SOCKET;
    c::log("[SYSTEM]: Disconnected.");
  }
}

static void receiveThread() {
  char buffer[8192];
  while (clientSocket != INVALID_SOCKET) {
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
      buffer[bytesReceived] = '\0';
      messageAccumulator += buffer;

      size_t pos;
      while ((pos = messageAccumulator.find('\n')) != std::string::npos) {
        std::string singleMsg = messageAccumulator.substr(0, pos);
        messageAccumulator.erase(0, pos + 1);

        // Trim trailing whitespace/newlines
        while (!singleMsg.empty() &&
               (singleMsg.back() == '\r' || singleMsg.back() == '\n' ||
                singleMsg.back() == ' '))
          singleMsg.pop_back();

        if (!singleMsg.empty()) {
          Win32Lock lock(msgLock);
          incomingMessages.push(singleMsg);
        }
      }
    } else {
      // Error or close
      if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        Win32Lock lock(msgLock);
        incomingMessages.push("[SYSTEM]: Disconnected from server.");
      }
      break;
    }
  }
}

static DWORD WINAPI receiveThreadStatic(LPVOID param) {
  receiveThread();
  return 0;
}

static void connectToServer(const std::string &ip) {
  if (!networkingInitialized) {
    InitializeCriticalSection(&msgLock);
    networkingInitialized = true;
  }

  InitNet();

  if (clientSocket != INVALID_SOCKET) {
    closesocket(clientSocket);
    clientSocket = INVALID_SOCKET;
  }

  clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (clientSocket == INVALID_SOCKET) {
    c::log("[SYSTEM]: Failed to create socket.");
    return;
  }

  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(25565);
  serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

  if (connect(clientSocket, (struct sockaddr *)&serverAddr,
              sizeof(serverAddr)) == SOCKET_ERROR) {
    c::log("[SYSTEM]: Failed to connect to " + ip);
    closesocket(clientSocket);
    clientSocket = INVALID_SOCKET;
    return;
  }

  c::log("[SYSTEM]: Connected to " + ip);

  // Send our identifier - server will assign name/faction
  std::string nameMsg = "CONNECT\n";
  send(clientSocket, nameMsg.c_str(), (int)nameMsg.length(), 0);

  CreateThread(NULL, 0, receiveThreadStatic, NULL, 0, NULL);
}

void sendSignal(const std::string &msg) {
  if (msg.find("CONNECT ") == 0) {
    std::string ip = msg.substr(8);
    connectToServer(ip);
    return;
  }
  if (msg == "DISCONNECT") {
    disconnectFromServer();
    return;
  }

  // Normal send
  if (clientSocket != INVALID_SOCKET) {
    std::string packet = msg + "\n";
    send(clientSocket, packet.c_str(), (int)packet.length(), 0);
  } else {
    // c::log("[SYSTEM]: Not connected.");
  }
}

void sendSignal(const std::string &type, const std::string &msg) {
  sendSignal(type + " " + msg);
}

void receiveSignal() {
  if (!networkingInitialized)
    return;

  Win32Lock lock(msgLock);
  while (!incomingMessages.empty()) {
    std::string msg = incomingMessages.front();
    incomingMessages.pop();

    // c::log("[DEBUG NETWORK] Processing: " + msg.substr(0, 100));

    bool handled = false;
    for (std::map<std::string, NetCommand>::iterator it =
             g_net_handlers.begin();
         it != g_net_handlers.end(); ++it) {
      NetCommand &cmd = it->second;
      if (msg.find(cmd.prefix) == 0) {
        std::string payload = msg.substr(cmd.prefix.length());
        if (payload.length() > 0 && payload[0] == ' ')
          payload.erase(0, 1);

        NetArgs args;
        // Single greedy arg support
        if (!cmd.paramNames.empty()) {
          args.data[cmd.paramNames[0]] = payload;
        }

        cmd.func(args);
        handled = true;
        break;
      }
    }

    if (!handled) {
      c::log(msg);
    }
  }
}
