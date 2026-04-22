#pragma once
#include "../.struct.h"
#include "network_internal.h"
#include "registry.h"
#include <mutex>

extern int port;
extern std::map<SOCKET, PlayerData> g_playerData;
extern std::recursive_mutex g_clientsMutex;
extern bool g_running;

extern int targetFPS;
extern float g_masterSpeed;
extern int g_masterSpeedVersion;
extern std::string g_lastSpeedUpdater;
void log(const std::string &msg);
void fixedUpdate();

inline void InitNet() {
  net_reg("chat <msg>", [](const NetArgs &args) {
    std::string msg = "[" + args.client.name + "]: " + args.get("msg");
    c::log(msg);
    sendAllClients("chat", msg, args.client.id);
  });

  // Handle speech bubble messages (from in-game chat)
  // Format: "say <charID> <message>"
  // Note: Client already adds chat log locally, so we only broadcast the say
  // command
  net_reg("say <data>", [](const NetArgs &args) {
    std::string data = args.get("data");
    if (data.empty())
      return;

    // Broadcast the say command to all OTHER clients (speech bubble only)
    sendAllClients("say", data, args.client.id);
  });

  net_reg("SYNC <data>", [](const NetArgs &args) {
    std::string data = args.get("data");
    // ::log("Server received SYNC from " + args.client.name + " (" +
    //       std::to_string(data.length()) + " bytes)");
    if (data.empty())
      return;

    s::world *newWorld = new s::world(data);

    std::lock_guard<std::recursive_mutex> lock(g_clientsMutex);
    if (g_playerData.count(args.client.id)) {
      if (g_playerData[args.client.id].worldState) {
        delete g_playerData[args.client.id].worldState;
      }
      g_playerData[args.client.id].worldState = newWorld;
    } else {
      delete newWorld;
    }
  });

  net_reg("speed <val> <ver>", [](const NetArgs &args) {
    float newSpeed = (float)atof(args.get("val").c_str());
    int newVer = atoi(args.get("ver").c_str());

    std::lock_guard<std::recursive_mutex> lock(g_clientsMutex);
    if (newVer > g_masterSpeedVersion) {
      g_masterSpeed = newSpeed;
      g_masterSpeedVersion = newVer;
      g_lastSpeedUpdater = args.client.name;
      ::log("[SYSTEM]: Speed set to " + args.get("val") + " by " +
            g_lastSpeedUpdater);
    }
  });

  ::log("[SYSTEM]: Network handlers initialized.");

  CreateThread(
      NULL, 0,
      (LPTHREAD_START_ROUTINE) + [](LPVOID) -> DWORD {
        while (g_running) {
          fixedUpdate();
          Sleep(1000 / targetFPS);
        }
        return 0;
      },
      NULL, 0, NULL);
}
inline void fixedUpdate() {
  std::lock_guard<std::recursive_mutex> lock(g_clientsMutex);
  if (g_playerData.empty()) {
    // ::log("No players to sync.");
    return;
  }

  std::stringstream ss;
  ss << "WORLD:" << g_masterSpeed << ":" << g_masterSpeedVersion << ":";
  bool first = true;
  for (std::map<SOCKET, PlayerData>::iterator it = g_playerData.begin();
       it != g_playerData.end(); ++it) {
    if (!it->second.worldState)
      continue;

    // ::log("Serializing world state for " + it->second.name);

    for (std::map<std::string, s::faction *>::iterator fit =
             it->second.worldState->factions.begin();
         fit != it->second.worldState->factions.end(); ++fit) {
      if (!first)
        ss << "|";
      ss << fit->second->serialise();
      first = false;
    }
  }
  // ::log("Sending sync to all clients: " + std::to_string(ss.str().length()) +
  //       " bytes");
  sendAllClients("sync", ss.str());
}
