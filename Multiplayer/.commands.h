#pragma once
#include "Registry.h"

#include ".func.h"
#include ".network.h"
#include <ogre/OgreVector3.h>
#include <sstream>

#define VALID(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      c::log(msg);                                                             \
      return;                                                                  \
    }                                                                          \
  } while (0)

inline void InitCommands() {
  reg("exit", "Exit the game", [](const CmdArgs &args) { exit(0); });

  reg("give <item_name/sid> [quantity]", "Give selected char an item",
      [](const CmdArgs &args) {
        int quantity = args.get("quantity", "1");
        std::string itemName = args.get("item_name/sid");

        GameData *data = findData(itemName);
        VALID(data, "Item not found: " + itemName);

        Character *target = getSelectedChar();
        VALID(target, "No character selected.");

        Inventory *inv = target->getInventory();
        VALID(inv, "Target has no inventory.");

        Item *newItem = give(inv, data, quantity);
        VALID(newItem, "Failed to create or add item.");

        std::stringstream ss;
        ss << quantity;
        c::log("Gave " + ss.str() + " x " + data->name);
      });

  reg("spawn [char_name/sid] [quantity]", "Spawns a character",
      [](const CmdArgs &args) {
        int quantity = args.get("quantity", "1");
        std::string charName = args.get("char_name/sid", "Wanderer");

        GameData *data = findData(charName);
        VALID(data, "Character template not found: " + charName);

        Character *target = getSelectedChar();
        VALID(target, "No character selected.");

        ActivePlatoon *activePlat = target->getPlatoon();
        VALID(activePlat, "Selected character not in a platoon.");
        Platoon *squad = activePlat->me;
        VALID(squad, "Selected character not in a platoon.");

        Ogre::Vector3 pos = target->getPosition();
        pos.x += 2.0f;

        Character *spawned = spawn(squad, data, pos, quantity);
        VALID(spawned, "Failed to spawn character.");

        c::log("[SYSTEM]: Spawned character: " + data->name);
      });

  reg("clone [quantity]", "Clones selected character", [](const CmdArgs &args) {
    int quantity = args.get("quantity", "1");
    Character *target = getSelectedChar();
    VALID(target, "No character selected.");

    GameData *data = target->data;
    VALID(data, "Target has no template data.");

    ActivePlatoon *activePlat = target->getPlatoon();
    VALID(activePlat, "Selected character not in a platoon.");
    Platoon *squad = activePlat->me;
    VALID(squad, "Selected character not in a platoon.");

    Ogre::Vector3 pos = target->getPosition();
    pos.x += 2.0f;

    GameDataCopyStandalone *looks = target->getAppearanceData();

    for (int i = 0; i < quantity; i++) {
      Character *cloned = spawn(squad, data, pos, 1);
      if (cloned) {
        if (looks)
          cloned->setAppearanceData(looks);
        cloned->setName(target->getName());
      }
      pos.x += 1.0f;
    }

    c::log("[SYSTEM]: Cloned character: " + target->getName());
  });

  reg("destroy", "Destroy selected characters", [](const CmdArgs &args) {
    lektor<Character *> selection = getSelectedChars();
    VALID(selection.size() > 0, "No characters selected.");

    int count = 0;
    for (size_t i = 0; i < selection.size(); ++i) {
      if (destroy(selection[i])) {
        count++;
      }
    }

    std::stringstream ss;
    ss << count;
    c::log("[SYSTEM]: Destroyed " + ss.str() + " characters.");
  });

  reg("take <item_name/sid> [quantity]", "Take item from selected characters",
      [](const CmdArgs &args) {
        int quantity = args.get("quantity", "1");
        std::string itemName = args.get("item_name/sid");

        GameData *data = findData(itemName);
        VALID(data, "Item not found: " + itemName);

        lektor<Character *> selection = getSelectedChars();
        VALID(selection.size() > 0, "No characters selected.");

        int totalTaken = 0;
        int remaining = quantity;

        for (size_t i = 0; i < selection.size(); ++i) {
          Character *target = selection[i];
          Inventory *inv = target->getInventory();
          if (!inv)
            continue;

          while (remaining > 0) {
            Item *it = findItem(inv, data);
            if (!it)
              break;

            int toTake = (it->quantity < remaining) ? it->quantity : remaining;
            if (destroyItem(it, toTake)) {
              remaining -= toTake;
              totalTaken += toTake;
            } else {
              break;
            }
          }
          if (remaining <= 0)
            break;
        }

        if (totalTaken > 0) {
          std::stringstream ss;
          ss << totalTaken;
          c::log("[SYSTEM]: Took " + ss.str() + " x " + data->name);
        } else {
          c::log("[SYSTEM]: Item not found in selection: " + data->name);
        }
      });

  reg("inv", "List selected character inventory", [](const CmdArgs &args) {
    Character *target = getSelectedChar();
    VALID(target, "No character selected.");

    Inventory *inv = target->getInventory();
    VALID(inv, "Target has no inventory.");

    std::vector<Item *> items = getAllItems(inv);
    c::log("[SYSTEM]: Inventory for " + target->getName() + ":");
    for (size_t i = 0; i < items.size(); ++i) {
      Item *it = items[i];
      if (!it)
        continue;
      std::stringstream ss;
      ss << " - " << it->getName() << " (qty: " << it->quantity
         << ", id: " << (it->data ? it->data->stringID : "NONE") << ")";
      c::log(ss.str());
    }
  });

  reg("connect [ip]", "Connect to a server", [](const CmdArgs &args) {
    std::string ip = args.get("ip", "127.0.0.1");
    sendSignal("CONNECT " + ip);
  });

  reg("disconnect", "Disconnect from server", [](const CmdArgs &args) {
    // Cleanup all remote characters before disconnecting
    std::set<std::string> empty;
    cleanupDisconnectedFactions(empty);
    sendSignal("DISCONNECT");
  });

  reg("host", "Launch server.exe", [](const CmdArgs &args) {
    c::log("[SYSTEM]: Launching server...");
    ShellExecuteA(NULL, "open", "mods\\Multiplayer\\server.exe", NULL, NULL,
                  SW_SHOW);
  });

  reg("adr", "Copy AI address to clipboard for selected character",
      [](const CmdArgs &args) {
        Character *target = getSelectedChar();
        VALID(target, "No character selected.");

        AI *ai = target->getAI();
        VALID(ai, "Character has no AI.");

        // Convert AI pointer to hex string
        std::stringstream ss;
        ss << std::hex << std::uppercase << (uintptr_t)ai;
        std::string hexAddr = ss.str();

        // Copy to clipboard
        if (OpenClipboard(NULL)) {
          EmptyClipboard();
          HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, hexAddr.length() + 1);
          if (hMem) {
            memcpy(GlobalLock(hMem), hexAddr.c_str(), hexAddr.length() + 1);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
          }
          CloseClipboard();
          c::log("[SYSTEM]: AI address copied to clipboard: 0x" + hexAddr);
        } else {
          c::log("[SYSTEM]: Failed to open clipboard");
        }
      });

  reg("client", "Reconnect to local or launch new game",
      [](const CmdArgs &args) {
        c::log("[SYSTEM]: Launching another client...");
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        ShellExecuteA(NULL, "open", buffer, NULL, NULL, SW_SHOW);
      });

  reg("squad_create <name>", "Create a new empty squad",
      [](const CmdArgs &args) {
        std::string name = args.get("name");
        VALID(!name.empty(), "Squad name cannot be empty.");

        Character *target = getSelectedChar();
        VALID(target, "No character selected (needed for position).");
        Ogre::Vector3 pos = target->getPosition();

        if (!ou || !ou->factionMgr)
          return;
        Faction *f = ou->factionMgr->getFactionByStringID("204-gamedata.base");
        VALID(f, "Player faction not found.");

        Platoon *p = createSquad(f, name, pos);
        VALID(p, "Failed to create squad.");

        c::log("[SYSTEM]: Created squad: " + name);
      });

  reg("squad_delete <name>", "Delete a squad by name", [](const CmdArgs &args) {
    std::string name = args.get("name");
    VALID(!name.empty(), "Squad name cannot be empty.");

    if (!ou || !ou->factionMgr)
      return;
    Faction *f = ou->factionMgr->getFactionByStringID("204-gamedata.base");
    VALID(f, "Player faction not found.");

    int count = deleteSquadsByName(f, name);

    std::stringstream ss;
    ss << count;
    c::log("[SYSTEM]: Deleted " + ss.str() + " squads named '" + name + "'.");
  });

  reg("build [building_name/sid]", "Build a building at character pos",
      [](const CmdArgs &args) {
        std::string arg = args.get("building_name/sid");
        VALID(!arg.empty(), "Usage: /build <building_name/sid>");

        Character *playerChar = getSelectedChar();
        VALID(playerChar, "No character selected.");

        // Priority search: Try building category directly first (best for
        // names)
        GameData *gd = ou->gamedata.getDataByName(arg, BUILDING);
        if (!gd)
          gd = findData(arg); // Fallback to SID or other categories

        VALID(gd, "Building template not found: " + arg);

        Ogre::Vector3 playerPos = playerChar->getPosition();
        Ogre::Quaternion rot = playerChar->getOrientation();

        // Manual rotation calculation for forward offset (Z-forward)
        float fx = 2.0f * (rot.x * rot.z + rot.w * rot.y);
        float fz = 1.0f - 2.0f * (rot.x * rot.x + rot.y * rot.y);

        Ogre::Vector3 pos = playerPos;
        pos.x += fx * 8.0f;
        pos.y = 0.0f; // 0.0 means "on the ground"
        pos.z += fz * 8.0f;

        Faction *faction = playerChar->getFaction();
        VALID(faction, "Selected character has no faction.");

        Building *b = ou->theFactory->createBuilding(
            gd, pos, NULL, faction, rot, NULL, NULL, NULL, NULL, NULL, false,
            true, false, 0, false);

        if (b) {
          if (key) {
            // Simulate Ctrl+Shift+F11 (Fix Navmesh / Regenerate Sector)
            key->keyDownEvent(OIS::KC_LCONTROL);
            key->keyDownEvent(OIS::KC_LSHIFT);
            key->keyDownEvent(OIS::KC_F11);

            key->keyUpEvent(OIS::KC_F11);
            key->keyUpEvent(OIS::KC_LSHIFT);
            key->keyUpEvent(OIS::KC_LCONTROL);
          }

          Ogre::Vector3 finalPos = b->getPosition();
          std::stringstream ss;
          ss << "[SYSTEM]: Built " << gd->name << " at " << (int)finalPos.x
             << ", " << (int)finalPos.y << ", " << (int)finalPos.z;
          c::log(ss.str());
        } else {
          c::log("[SYSTEM]: Build failed.");
        }
      });

  reg("savelook [name]", "Save selected character's look (pointer)",
      [](const CmdArgs &args) {
        Character *c = getSelectedChar();
        VALID(c, "No character selected.");

        std::string name = args.get("name", "default");
        if (saveLook(c, name)) {
          c::log("[SYSTEM]: Saved look '" + name + "' (pointer)");
        } else {
          c::log("[SYSTEM]: Failed to save look (pointer)");
        }
      });

  reg("loadlook [name]", "Load look onto selected character (pointer)",
      [](const CmdArgs &args) {
        Character *c = getSelectedChar();
        VALID(c, "No character selected.");

        std::string name = args.get("name", "default");
        if (loadLook(c, name)) {
          c::log("[SYSTEM]: Loaded look '" + name + "' (pointer)");
        } else {
          c::log("[SYSTEM]: Failed to load look - '" + name + "' not found");
        }
      });

  static std::map<std::string, std::string> g_serialisedLooks;

  reg("savelooks [name]", "Save selected character's look as string",
      [](const CmdArgs &args) {
        Character *c = getSelectedChar();
        VALID(c, "No character selected.");

        std::string name = args.get("name", "default");
        GameDataCopyStandalone *looks = c->getAppearanceData();
        VALID(looks, "Selected character has no appearance data.");

        GameData *gd = (GameData *)looks;

        std::stringstream ss;
        // f = float sliders, s = string properties, i = int properties
        for (auto it = gd->fdata.begin(); it != gd->fdata.end(); ++it)
          ss << "f:" << it->first << ":" << it->second << "#";
        for (auto it = gd->sdata.begin(); it != gd->sdata.end(); ++it)
          ss << "s:" << it->first << ":" << it->second << "#";
        for (auto it = gd->idata.begin(); it != gd->idata.end(); ++it)
          ss << "i:" << it->first << ":" << it->second << "#";
        for (auto it = gd->vecdata.begin(); it != gd->vecdata.end(); ++it)
          ss << "v:" << it->first << ":" << it->second.x << "," << it->second.y
             << "," << it->second.z << "#";
        for (auto it = gd->quatdata.begin(); it != gd->quatdata.end(); ++it)
          ss << "q:" << it->first << ":" << it->second.w << "," << it->second.x
             << "," << it->second.y << "," << it->second.z << "#";
        for (auto it = gd->bdata.begin(); it != gd->bdata.end(); ++it)
          ss << "b:" << it->first << ":" << (it->second ? 1 : 0) << "#";

        // Save gender explicitly
        ss << "g::" << (c->isFemale() ? 1 : 0) << "#";

        g_serialisedLooks[name] = ss.str();

        c::log("[SYSTEM]: Serialized look '" + name + "' (" +
               ToString(g_serialisedLooks[name].length()) + " bytes)");
      });

  reg("loadlooks [name]", "Load look from serialized string",
      [](const CmdArgs &args) {
        Character *c = getSelectedChar();
        VALID(c, "No character selected.");

        std::string name = args.get("name", "default");
        VALID(g_serialisedLooks.count(name), "Look '" + name + "' not found.");

        GameData *gd = (GameData *)c->getAppearanceData();
        VALID(gd, "Selected character has no appearance data.");

        // Also get the character's main data for gender
        GameData *charData = c->getGameData();

        std::string data = g_serialisedLooks[name];
        std::stringstream ss(data);
        std::string segment;
        bool isFoid = false;

        while (std::getline(ss, segment, '#')) {

          if (segment.size() < 4)
            continue;
          size_t first = segment.find(':');
          size_t second = segment.find(':', first + 1);
          if (first == std::string::npos || second == std::string::npos)
            continue;

          char type = segment[0];
          std::string key = segment.substr(first + 1, second - first - 1);
          std::string val = segment.substr(second + 1);

          if (type == 'f')
            gd->fdata[key] = (float)atof(val.c_str());
          else if (type == 's')
            gd->sdata[key] = val;
          else if (type == 'i')
            gd->idata[key] = atoi(val.c_str());
          else if (type == 'v') {
            float x, y, z;
            if (sscanf(val.c_str(), "%f,%f,%f", &x, &y, &z) == 3)
              gd->vecdata[key] = Ogre::Vector3(x, y, z);
          } else if (type == 'q') {
            float w, x, y, z;
            if (sscanf(val.c_str(), "%f,%f,%f,%f", &w, &x, &y, &z) == 4)
              gd->quatdata[key] = Ogre::Quaternion(w, x, y, z);
          } else if (type == 'b') {
            gd->bdata[key] = (atoi(val.c_str()) == 1);
          } else if (type == 'g') {
            // Gender: set on BOTH appearance data AND character's main data
            isFoid = (atoi(val.c_str()) == 1);
            gd->bdata["foid"] = isFoid;
            if (charData) {
              charData->bdata["foid"] = isFoid;
            }
          }
        }

        // Apply the appearance data
        c->setAppearanceData((GameDataCopyStandalone *)gd);

        c::log("[SYSTEM]: Applied serialized look '" + name + "' onto " +
               c->getName() + " (foid=" + (isFoid ? "true" : "false") + ")");
      });
}
