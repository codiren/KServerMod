#pragma once
#include <functional>
#include <map>
#include <string>
#include <winsock2.h>

namespace s {
struct world;
}

struct PlayerData {
  std::string name;
  std::string factionId;
  s::world *worldState;
  PlayerData() : worldState(NULL) {}
};

struct Client {
  std::string name;
  SOCKET id;
};

struct NetArgs {
  std::map<std::string, std::string> data;
  Client client;
  std::string get(const std::string &key) const {
    std::map<std::string, std::string>::const_iterator it = data.find(key);
    return (it != data.end()) ? it->second : "";
  }
};

typedef std::function<void(const NetArgs &)> NetHandler;

void net_reg(const std::string &pattern, NetHandler handler);
void InitNet();
void sendAllClients(const std::string &type, const std::string &msg,
                    SOCKET exclude = INVALID_SOCKET);
