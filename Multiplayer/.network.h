#pragma once
#include ".struct.h"
#include "Registry.h"
#include "network.cpp"
#include "network_internal.h"
#include <set>
#include <string>

static float lastSentSpeed = 1.0f;
static int localSpeedVer = 0;

// Forward declared from .func.h
inline void setGameSpeed(float speed);
void applyRemoteFaction(s::faction *f);
void cleanupDisconnectedFactions(const std::set<std::string> &activeFactions);
void remoteChat(long long charID, const std::string &msg);

extern std::string myName;

// User Chat API
// User Chat API
bool isPlayerFaction(const std::string &name) {
  std::string suffix = "-Multiplayer.mod";
  if (name.length() >= suffix.length()) {
    return (0 == name.compare(name.length() - suffix.length(), suffix.length(),
                              suffix));
  }
  return false;
}
struct ChatHandlers { // CLIENT
  ChatHandlers() {
    net_reg("chat <msg>", [](const NetArgs &args) { c::log(args.get("msg")); });

    // Handler for remote character speech bubbles
    // Format: "say <charID> <message>"
    net_reg("say <data>", [](const NetArgs &args) {
      std::string data = args.get("data");
      // Parse charID and message
      size_t spacePos = data.find(' ');
      if (spacePos == std::string::npos)
        return;

      long long charID = _atoi64(data.substr(0, spacePos).c_str());
      std::string message = data.substr(spacePos + 1);

      // Use helper function from .func.h to make the remote character speak
      remoteChat(charID, message);
    });

    net_reg("sync <data>", [](const NetArgs &args) {
      std::string data = args.get("data");
      // c::log("[DEBUG] Sync packet received, len: " +
      // ToString(data.length()));

      s::world w(data);
      // c::log("[DEBUG] World deserialized. Factions: " +
      //        ToString(w.factions.size()));

      std::set<std::string> activeFactions;

      for (std::map<std::string, s::faction *>::iterator it =
               w.factions.begin();
           it != w.factions.end(); ++it) {
        s::faction *val = it->second;
        bool isPlayer = isPlayerFaction(val->sid);
        bool isMe = (val->sid == myName);

        // c::log("[DEBUG] Faction: " + val->sid + " | isPlayer: " +
        //        ToString(isPlayer) + " | isMe: " + ToString(isMe));

        if (isPlayer && !isMe) {
          activeFactions.insert(val->sid);
          applyRemoteFaction(val);
        }
      }

      // Cleanup disconnected players (defined in .func.h)
      cleanupDisconnectedFactions(activeFactions);

      // Apply game speed from server using version-based synchronization
      int serverSpeedVer = w.gameSpeedVersion;
      float serverSpeed = w.gameSpeed;

      if (serverSpeedVer > localSpeedVer) {
        // Server has a newer version, apply it
        setGameSpeed(serverSpeed);
        lastSentSpeed = serverSpeed;
        localSpeedVer = serverSpeedVer;
      } else if (serverSpeedVer < localSpeedVer) {
        // Server has an older version? Fallback to server's state to resolve
        // conflict
        setGameSpeed(serverSpeed);
        lastSentSpeed = serverSpeed;
        localSpeedVer = serverSpeedVer;
      } else {
        // Version same. Just ensure speed is same (ACK)
        if (abs(lastSentSpeed - serverSpeed) > 0.001f) {
          setGameSpeed(serverSpeed);
          lastSentSpeed = serverSpeed;
        }
      }
    });
  }
} chat_handlers;

void sendChatMsg(const std::string &msg) { sendSignal("chat", msg); }