#pragma once
#include <string>

#include <functional>
#include <map>
#include <vector>

void receiveSignal();
void sendSignal(const std::string &msg);
void sendSignal(const std::string &type, const std::string &msg);

void receiveChatMsg(const std::string &msg);
void sendChatMsg(const std::string &msg);

// Registry
struct NetArgs {
  std::map<std::string, std::string> data;
  std::string get(const std::string &key) const {
    std::map<std::string, std::string>::const_iterator it = data.find(key);
    return (it != data.end()) ? it->second : "";
  }
};

typedef std::function<void(const NetArgs &)> NetHandler;
void net_reg(const std::string &pattern, NetHandler handler);
void InitNet();
