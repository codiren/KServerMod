#pragma once
#include <string>
#include <winsock2.h>

// Declarations
void log(const std::string &msg);
void Broadcast(const std::string &msg, SOCKET sender = INVALID_SOCKET);

// Configuration Globals
extern int g_port;
extern std::string g_prefix;
