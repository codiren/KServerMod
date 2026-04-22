#pragma once
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Define LogToChat as extern so we can call it from here
extern void LogToChat(const std::string &msg);

struct ValueWrapper {
  std::string val;
  ValueWrapper(std::string v) : val(v) {}
  operator std::string() const { return val; }
  operator int() const {
    if (val.empty())
      return 1;
    return atoi(val.c_str());
  }
};

struct CmdArgs {
  std::map<std::string, std::string> data;
  ValueWrapper get(std::string key, std::string defaultVal = "") const {
    std::map<std::string, std::string>::const_iterator it = data.find(key);
    if (it == data.end() || it->second.empty())
      return ValueWrapper(defaultVal);
    return ValueWrapper(it->second);
  }
  bool empty() const { return data.empty(); }
  size_t count(std::string key) const { return data.count(key); }
};

typedef std::function<void(const CmdArgs &)> CmdFunc;

struct Command {
  Command() : requiredCount(0) {}
  CmdFunc func;
  std::string description;
  std::string templateStr;
  std::vector<std::string> paramNames;
  size_t requiredCount;
};

static std::map<std::string, Command> &get_registry() {
  static std::map<std::string, Command> r;
  return r;
}

namespace c {
inline void log(std::string msg) { LogToChat(msg); }
inline void broadcast(std::string msg) { LogToChat("[BROADCAST]: " + msg); }
} // namespace c

inline void reg(std::string cmdTemplate, std::string summary, CmdFunc f) {
  std::stringstream ss(cmdTemplate);
  std::vector<std::string> parts;
  std::string part;
  while (ss >> part)
    parts.push_back(part);

  if (parts.empty())
    return;

  std::string name = parts[0];
  std::string prefix = "/";
  if (name.size() < prefix.size() || name.substr(0, prefix.size()) != prefix) {
    name = prefix + name;
  }

  Command &cmd = get_registry()[name];
  cmd.func = f;
  cmd.description = summary;
  cmd.templateStr = cmdTemplate;
  cmd.paramNames.clear();
  cmd.requiredCount = 0;

  for (size_t i = 1; i < parts.size(); ++i) {
    std::string p = parts[i];
    if (p.size() >= 2 && p[0] == '<' && p[p.size() - 1] == '>') {
      cmd.paramNames.push_back(p.substr(1, p.size() - 2));
      cmd.requiredCount++;
    } else if (p.size() >= 2 && p[0] == '[' && p[p.size() - 1] == ']') {
      cmd.paramNames.push_back(p.substr(1, p.size() - 2));
    }
  }
}

inline bool run_cmd(std::string line) {
  auto &reg_map = get_registry();

  std::vector<std::string> segments;
  std::stringstream ss(line);
  std::string segment;
  while (ss >> segment)
    segments.push_back(segment);

  if (segments.empty())
    return false;
  std::string cmdName = segments[0];

  if (cmdName == "/help") {
    LogToChat("[SYSTEM]: Available Commands:");
    for (auto it = reg_map.begin(); it != reg_map.end(); ++it) {
      std::string msg = "  " + it->second.templateStr;
      if (!it->second.description.empty())
        msg += " - " + it->second.description;
      LogToChat(msg);
    }
    return true;
  }

  auto it = reg_map.find(cmdName);
  if (it == reg_map.end())
    return false;

  Command &cmd = it->second;
  if (!cmd.func)
    return true;

  size_t argCount = segments.size() - 1;
  if (argCount < cmd.requiredCount) {
    c::log("Usage: " + cmd.templateStr);
    return true;
  }

  CmdArgs mappedArgs;
  // Basic mapping:
  // If we have more segments than params, it's usually because one param has
  // spaces. We'll greedily fill from left, but if the last param is optional
  // and is a number, we treat it as the last param and everything else as the
  // previous one.

  if (cmd.paramNames.size() == 0) {
    cmd.func(mappedArgs);
    return true;
  }

  // Specialized logic for 2 params (like give <item> [qty])
  if (cmd.paramNames.size() == 1) {
    // Single parameter: Capture everything after the command
    std::string val;
    for (size_t i = 1; i < segments.size(); ++i) {
      if (i > 1)
        val += " ";
      val += segments[i];
    }
    mappedArgs.data[cmd.paramNames[0]] = val;
  } else if (cmd.paramNames.size() == 2) {
    // Two parameters (e.g., <name> [qty]): Check if last word is a quantity
    if (segments.size() > 1) {
      std::string lastSeg = segments[segments.size() - 1];
      bool lastIsQty = !lastSeg.empty();
      for (size_t j = 0; j < lastSeg.length(); ++j) {
        if (!isdigit((unsigned char)lastSeg[j])) {
          lastIsQty = false;
          break;
        }
      }

      if (lastIsQty && segments.size() > 2) {
        // Multiple words + number at end: number is qty, rest is name
        mappedArgs.data[cmd.paramNames[1]] = lastSeg;
        std::string firstVal;
        for (size_t i = 1; i < segments.size() - 1; ++i) {
          if (i > 1)
            firstVal += " ";
          firstVal += segments[i];
        }
        mappedArgs.data[cmd.paramNames[0]] = firstVal;
      } else if (lastIsQty && segments.size() == 2) {
        // Only a number provided: treat as quantity, leave name empty (use
        // default)
        mappedArgs.data[cmd.paramNames[1]] = lastSeg;
        mappedArgs.data[cmd.paramNames[0]] = "";
      } else {
        // Last word isn't a number, grab everything as the first param
        std::string firstVal;
        for (size_t i = 1; i < segments.size(); ++i) {
          if (i > 1)
            firstVal += " ";
          firstVal += segments[i];
        }
        mappedArgs.data[cmd.paramNames[0]] = firstVal;
      }
    }

  } else {
    // Fallback simple mapping for 3+ params
    for (size_t i = 0; i < cmd.paramNames.size(); ++i) {
      if (i + 1 < segments.size())
        mappedArgs.data[cmd.paramNames[i]] = segments[i + 1];
    }
  }

  cmd.func(mappedArgs);
  return true;
}
