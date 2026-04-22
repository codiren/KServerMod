#pragma once
#include "shared.h"
#include <map>

typedef void (*CmdFunc)();

struct Command {
  CmdFunc func;
  std::string description;
};

static std::map<std::string, Command> &get_registry() {
  static std::map<std::string, Command> r;
  return r;
}

static bool &discovery_mode() {
  static bool m = false;
  return m;
}

static std::string &current_reg_name() {
  static std::string n;
  return n;
}

namespace c {
inline void log(std::string msg) {
  if (discovery_mode())
    return;
  ::log("[CMD]: " + msg);
}
inline void broadcast(std::string msg) {
  if (discovery_mode())
    return;
  ::Broadcast("[Server]: " + msg, INVALID_SOCKET);
}
} // namespace c

inline void desc(std::string d) {
  if (discovery_mode()) {
    get_registry()[current_reg_name()].description = d;
  }
}

inline void reg(std::string name, CmdFunc f) {
  // Use g_prefix from config
  if (name.empty())
    return;
  if (name[0] != g_prefix[0]) {
    name = g_prefix + name;
  }
  current_reg_name() = name;
  get_registry()[name].func = f;

  // "Sniff" description
  discovery_mode() = true;
  if (f)
    f();
  discovery_mode() = false;
}

inline void run_cmd(std::string line) {
  auto &reg_map = get_registry();
  if (line == g_prefix + "help") {
    ::log("[CMD]: Available Commands:");
    for (auto const &it : reg_map) {
      std::string msg = "  " + it.first;
      if (!it.second.description.empty())
        msg += " - " + it.second.description;
      ::log(msg);
    }
    ::log("  " + g_prefix + "help - list all commands");
  } else {
    auto it = reg_map.find(line);
    if (it != reg_map.end()) {
      if (it->second.func)
        it->second.func();
    }
  }
}
