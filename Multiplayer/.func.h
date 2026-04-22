#pragma once
#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

#include <ogre/OgreQuaternion.h>
#include <ogre/OgreVector3.h>

#include ".struct.h"
#include <kenshi/Character.h>
#include <kenshi/CombatClass.h>
#include <kenshi/Faction.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/Platoon.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObjectFactory.h>
#include <kenshi/util/lektor.h>
#include <windows.h>

// Externs from network.cpp
extern std::string myName;
void sendSignal(const std::string &msg);
void sendSignal(const std::string &type, const std::string &msg);
extern void LogToChat(const std::string &msg);

static s::world *oldWorld = nullptr;
class GameDataCopyStandalone;
static std::map<std::string, GameDataCopyStandalone *> g_savedLooks;

inline bool saveLook(Character *c, std::string name) {
  if (!c)
    return false;
  GameDataCopyStandalone *looks = c->getAppearanceData();
  if (!looks)
    return false;
  g_savedLooks[name] = looks;
  return true;
}

inline bool loadLook(Character *c, std::string name) {
  if (!c || !g_savedLooks.count(name))
    return false;
  c->setAppearanceData(g_savedLooks[name]);
  return true;
}

inline Item *getMouseItem() {
  char *base = (char *)GetModuleHandleA(NULL);
  void **mouseInvPtr = (void **)(base + 0x212FA88);
  if (!mouseInvPtr || !*mouseInvPtr)
    return nullptr;

  Item **itemPtr = (Item **)((char *)*mouseInvPtr + 0x30);
  return (itemPtr) ? *itemPtr : nullptr;
}

inline std::string toLower(std::string s) {
  for (size_t i = 0; i < s.length(); ++i) {
    s[i] = (char)tolower((unsigned char)s[i]);
  }
  return s;
}

inline bool destroy(RootObject *obj) {
  if (!ou || !obj)
    return false;
  return ou->destroy(obj, false, "COMMAND");
}

inline bool getBlockMode(Character *ch) {
  if (!ch)
    return false;
  // Character::getStandingOrder RVA: 0x5C8F40
  // M_SET_ORDER_DEFENSIVE_COMBAT = 555
  typedef bool(__thiscall * GetStandingOrderFn)(const Character *,
                                                MessageForB::StandingOrder);
  static GetStandingOrderFn getOrder =
      (GetStandingOrderFn)((intptr_t)GetModuleHandleA(NULL) + 0x5C8F40);
  return getOrder(ch, MessageForB::M_SET_ORDER_DEFENSIVE_COMBAT);
}

inline void setBlockMode(Character *ch, bool on) {
  if (!ch || !ou || !ou->player)
    return;

  // Character::setStandingOrder RVA: 0x5CA2D0
  typedef void(__thiscall * SetStandingOrderFn)(
      Character *, MessageForB::StandingOrder, bool);
  static SetStandingOrderFn setOrder =
      (SetStandingOrderFn)((intptr_t)GetModuleHandleA(NULL) + 0x5CA2D0);

  Faction *playerFac = ou->player->getFaction();
  Faction *origFac = ch->getFaction();

  // If character is not in player faction, swap to allow setting the order
  if (playerFac && origFac != playerFac) {
    ch->setFaction(playerFac, NULL);
    setOrder(ch, MessageForB::M_SET_ORDER_DEFENSIVE_COMBAT, on);
    ch->setFaction(origFac, NULL);
  } else {
    setOrder(ch, MessageForB::M_SET_ORDER_DEFENSIVE_COMBAT, on);
  }
}

template <typename T> std::string ToString(const T &value) {
  std::stringstream ss;
  ss << value;
  return ss.str();
}

inline std::vector<Item *> getAllItems(Inventory *inv) {
  std::vector<Item *> out;
  if (!inv)
    return out;

  lektor<InventorySection *> &sections = inv->sectionsInSearchOrder;
  if (!sections.stuff)
    return out;

  for (unsigned int i = 0; i < sections.count; ++i) {
    InventorySection *s = sections.stuff[i];
    if (!s)
      continue;

    const Ogre::vector<InventorySection::SectionItem>::type &itms =
        s->getItems();
    for (size_t j = 0; j < itms.size(); ++j) {
      Item *it = itms[j].item;
      if (!it)
        continue;

      out.push_back(it);

      if (it->itemFunction == ITEM_CONTAINER) {
        std::vector<Item *> sub = getAllItems(((ContainerItem *)it)->inventory);
        out.insert(out.end(), sub.begin(), sub.end());
      }
    }
  }
  return out;
}

inline Item *findItem(Inventory *inv, GameData *data) {
  if (!inv || !data)
    return nullptr;
  std::vector<Item *> items = getAllItems(inv);
  for (size_t i = 0; i < items.size(); ++i) {
    Item *it = items[i];
    if (it && it->data == data)
      return it;
  }
  return nullptr;
}

inline Inventory *getItemInventory(Item *item) {
  if (!item)
    return nullptr;

  const hand &h = item->getInventoryWeAreIn();
  if (!h)
    return nullptr;

  RootObject *obj = h.getRootObject();
  if (obj)
    return obj->getInventory();

  return nullptr;
}

inline bool destroyItem(Item *item, int quantity = 1) {
  if (!item)
    return false;

  Inventory *inv = getItemInventory(item);
  bool isWeapon = (item->isWeapon() != nullptr);

  // New method (official API) - works better for backpacks and general items
  if (!isWeapon && inv) {
    typedef bool(__thiscall * RemoveItemAutoDestroyFn)(Inventory *, Item *,
                                                       int);
    static RemoveItemAutoDestroyFn removeItemAutoDestroyFn =
        (RemoveItemAutoDestroyFn)((intptr_t)GetModuleHandleA(NULL) + 0x744BD0);
    if (removeItemAutoDestroyFn(inv, item, quantity))
      return true;
  }

  // Old method (manual) - specifically for weapons or fallback
  if (inv) {
    if (item->quantity > quantity) {
      item->quantity -= quantity;
      inv->notifyModified();
      return true;
    }

    // Manual section cleanup
    for (unsigned int i = 0; i < inv->sectionsInSearchOrder.count; ++i) {
      InventorySection *s = inv->sectionsInSearchOrder.stuff[i];
      if (s && s->hasItem(item)) {
        s->removeItem(item);
        break;
      }
    }
  }

  return destroy(item);
}

inline InventorySection *getInvSection(Item *item) {
  if (!item)
    return nullptr;

  Inventory *inv = getItemInventory(item);
  if (!inv)
    return nullptr;

  lektor<InventorySection *> &sections = inv->sectionsInSearchOrder;
  for (unsigned int i = 0; i < sections.count; ++i) {
    if (sections.stuff[i] && sections.stuff[i]->hasItem(item)) {
      return sections.stuff[i];
    }
  }
  return nullptr;
}

inline bool moveItem(Item *item, std::string targetSection, int x, int y,
                     Inventory *targetInv = nullptr) {
  if (!item)
    return false;

  // Get current inventory (where the item is now)
  Inventory *currentInv = getItemInventory(item);

  // Get target inventory (where we want to move it to)
  // If not specified, assume we're moving within the same inventory
  if (!targetInv)
    targetInv = currentInv;

  if (!targetInv)
    return false;

  // Find target section in target inventory
  lektor<InventorySection *> &targetSections = targetInv->sectionsInSearchOrder;

  // Find target section in target inventory
  InventorySection *targetSec = nullptr;
  for (unsigned int i = 0; i < targetSections.count; ++i) {
    if (targetSections.stuff[i] &&
        targetSections.stuff[i]->name == targetSection) {
      targetSec = targetSections.stuff[i];
      break;
    }
  }

  if (!targetSec)
    return false;

  // Check if item is already in the correct section and position
  if (currentInv == targetInv && targetSec->hasItem(item) &&
      item->inventorySection == targetSection && item->inventoryPos.x == x &&
      item->inventoryPos.y == y) {
    // Already in correct position, no need to move
    return true;
  }

  // Remove from current inventory section
  if (currentInv) {
    lektor<InventorySection *> &currentSections =
        currentInv->sectionsInSearchOrder;
    for (unsigned int i = 0; i < currentSections.count; ++i) {
      if (currentSections.stuff[i] && currentSections.stuff[i]->hasItem(item)) {
        currentSections.stuff[i]->removeItem(item);
        break;
      }
    }
  }

  // Add to target section at new position
  targetSec->_addItem(item, x, y);

  // Update item properties
  item->inventorySection = targetSection;
  item->inventoryPos.x = x;
  item->inventoryPos.y = y;

  // Sync the item's internal handle so getItemInventory() returns the correct
  // owner
  typedef void(__thiscall * SetInvHandFn)(Item *, const hand &);
  static SetInvHandFn setInvHand =
      (SetInvHandFn)((intptr_t)GetModuleHandleA(NULL) + 0xD22F0);
  setInvHand(item, targetInv->getHandle());

  // Notify the target section to update its UI
  targetSec->notifyModified();

  return true;
}

inline Item *give(Inventory *inv, GameData *data, int quantity = 1) {
  if (!inv || !data || !ou || !ou->theFactory)
    return nullptr;

  Character *target = (Character *)inv->owner;
  Item *newItem = nullptr;

  if (target && data->type == WEAPON) {
    struct CharacterHack : public Character {
      using Character::generateWeapon;
    };
    GameData *manufacturer = ou->gamedata.getData("917-gamedata.base");
    newItem = ((CharacterHack *)target)->generateWeapon(data, manufacturer);
    if (newItem) {
      newItem->materialData = ou->gamedata.getData("1065-gamedata.base");
    }
  } else {
    int level = (data->type == ARMOUR) ? 40 : 0;
    newItem = ou->theFactory->createItem(data, hand(), NULL, NULL, level, NULL);
  }

  if (newItem) {
    // 3 and 4 are droponfail and destroyonfail
    inv->addItem(newItem, quantity, false, true);
  }
  return newItem;
}

inline GameData *findData(std::string name) {
  if (!ou || name.empty())
    return nullptr;

  GameData *data = ou->gamedata.getData(name);
  if (data)
    return data;

  for (int i = 0; i < (int)OBJECT_TYPE_MAX; ++i) {
    data = ou->gamedata.getDataByName(name, (itemType)i);
    if (data)
      return data;
  }

  return nullptr;
}

inline lektor<Character *> getSelectedChars() {
  lektor<Character *> selection;
  if (ou && ou->player) {
    ou->player->getAllSelectedObjects(*(lektor<RootObject *> *)&selection,
                                      CHARACTER);
  }
  return selection;
}
inline Character *getSelectedChar() {
  lektor<Character *> selection = getSelectedChars();
  return (selection.size() > 0) ? selection[0] : nullptr;
}

// Makes the specified character say a line (speech bubble)
inline void chat(Character *speaker, const std::string &msg) {
  if (speaker) {
    speaker->sayALine(msg, true);
  }
}

inline Character *spawn(Platoon *squad, GameData *data, Ogre::Vector3 pos,
                        int quantity = 1) {
  if (!ou || !ou->theFactory || !ou->player || !squad || !data)
    return nullptr;

  Faction *faction = squad->ownerships.faction;
  RootObjectContainer *container = (RootObjectContainer *)squad->activePlatoon;

  Character *res = nullptr;
  for (int i = 0; i < quantity; i++) {
    res = (Character *)ou->theFactory->createRandomCharacter(
        faction, pos, container, data, NULL, 1.0f);
    pos.x += 1.0f;
  }
  return res;
}

inline Faction *getFaction(std::string stringid) {
  if (!ou || !ou->factionMgr)
    return nullptr;
  Faction *f = ou->factionMgr->getFactionByStringID(stringid);
  if (!f)
    f = ou->factionMgr->getFactionByName(stringid);
  return f;
}

inline s::item *readItem(Item *it, Character *owner = NULL,
                         s::faction *context = NULL) {
  if (!it)
    return nullptr;
  s::item *res = new s::item();
  res->id = (long long)it;
  res->bagid = 0;

  const hand &h = it->getInventoryWeAreIn();
  if (h) {
    RootObject *obj = h.getRootObject();
    if (obj && (long long)obj != (long long)owner) {
      res->bagid = (long long)obj;
    }
  }

  res->sid = (it->data) ? it->data->stringID : "NONE";
  res->x = it->inventoryPos.x;
  res->y = it->inventoryPos.y;
  res->quantity = it->quantity;
  res->charges = it->chargesLeft;
  res->material = (it->materialData) ? it->materialData->stringID : "NONE";
  res->manufacturer =
      (it->manufacturerData) ? it->manufacturerData->stringID : "NONE";
  res->level = it->getLevel();
  res->section = it->inventorySection;

  // Freeze dragged item data (inspired by user request)
  if (it == getMouseItem() && oldWorld) {
    for (std::map<std::string, s::faction *>::iterator f =
             oldWorld->factions.begin();
         f != oldWorld->factions.end(); ++f) {
      s::item *old = f->second->getItem(res->id);
      if (old) {
        // If current section is empty/NONE, use old one to "freeze" it in UI
        if (res->section.empty() || res->section == "NONE") {
          res->section = old->section;
          res->x = old->x;
          res->y = old->y;
          res->bagid = old->bagid;
        }
        break;
      }
    }
  }

  if (context)
    context->addItem(res);
  return res;
}

#include "tasks.h"

struct remoteMapper {
  std::map<Item *, long long> privateToPublicIDItems;
  std::map<long long, Item *> publicToPrivateIDItems;
  std::map<Character *, long long> privateToPublicIDCharacters;
  std::map<long long, Character *> publicToPrivateIDCharacters;
  std::map<Platoon *, long long> privateToPublicIDSquads;
  std::map<long long, Platoon *> publicToPrivateIDSquads;
  std::map<long long, std::string> lastAppearance;
  std::map<long long, long long> lastTargetID;
};
static std::map<std::string, remoteMapper> g_mappers;

inline long long getPublicID(Character *c) {
  if (!c)
    return 0;
  if (ou && ou->factionMgr) {
    Faction *f = ou->factionMgr->getFactionByStringID("204-gamedata.base");
    if (f && c->getFaction() == f)
      return (long long)c;
  }
  for (std::map<std::string, remoteMapper>::iterator it = g_mappers.begin();
       it != g_mappers.end(); ++it) {
    std::map<Character *, long long>::iterator mit =
        it->second.privateToPublicIDCharacters.find(c);
    if (mit != it->second.privateToPublicIDCharacters.end())
      return mit->second;
  }
  return -1;
}

inline Character *findCharacterByPublicID(long long publicID) {
  if (publicID <= 0)
    return nullptr;
  for (std::map<std::string, remoteMapper>::iterator it = g_mappers.begin();
       it != g_mappers.end(); ++it) {
    if (it->second.publicToPrivateIDCharacters.count(publicID)) {
      return it->second.publicToPrivateIDCharacters[publicID];
    }
  }
  // If not found in mappers, it might be a local character (pointer as ID)
  return (Character *)publicID;
}

inline s::character *readCharacter(Character *ch, s::faction *context = NULL) {
  if (!ch)
    return nullptr;
  s::character *res = new s::character();
  res->id = (long long)ch;
  res->sid = (ch->data) ? ch->data->stringID : "NONE";
  res->name = ch->getName();

  Ogre::Vector3 pos = ch->getPosition();
  res->pos.x = pos.x;
  res->pos.y = pos.y;
  res->pos.z = pos.z;

  Ogre::Quaternion rot = ch->rot;
  res->rot.w = rot.w;
  res->rot.x = rot.x;
  res->rot.y = rot.y;
  res->rot.z = rot.z;

  // Real-time Task reading via discovered AI offsets 20 -> 1C0 -> C
  int taskType = 14; // Default to IDLE
  try {
    AI *ai = ch->getAI();
    if (ai) {
      void *aiTaskSystem = *(void **)((char *)ai + 0x20);
      if (aiTaskSystem) {
        void *currentTask = *(void **)((char *)aiTaskSystem + 0x1C0);
        if (currentTask) {
          taskType = *(int *)((char *)currentTask + 0x0C);
        }
      }
    }
  } catch (...) {
    // Keep default
  }

  void *cm = *(void **)((char *)ch + 0x640);
  if (cm) {
    // isCurrentlyMoving RVA: 0x333830
    typedef bool(__thiscall * IsMovingFunc)(void *);
    static IsMovingFunc isMovingF =
        (IsMovingFunc)((char *)GetModuleHandleA(NULL) + 0x333830);

    if (isMovingF(cm)) {
      // If task is IDLE but we are moving, set to MOVE_CUS_ORDERED (29)
      if (taskType == 14)
        taskType = 29;
    }

    Ogre::Vector3 dest = *(Ogre::Vector3 *)((char *)cm + 0xE8);
    res->moveTo.x = dest.x;
    res->moveTo.y = dest.y;
    res->moveTo.z = dest.z;
  }
  res->task = taskType;

  static std::map<Item *, Inventory *> g_itemParentCache;
  Inventory *inv = ch->getInventory();
  if (inv) {
    std::vector<Item *> items = getAllItems(inv);

    // Track current inventory for all items to handle mouse drags correctly
    for (size_t i = 0; i < items.size(); ++i) {
      g_itemParentCache[items[i]] = inv;
    }

    // Mouse Item detection (inspired by v0.3)
    Item *mItem = getMouseItem();
    if (mItem && context && context->sid == "204-gamedata.base") {
      bool found = false;
      for (size_t i = 0; i < items.size(); ++i) {
        if (items[i] == mItem) {
          found = true;
          break;
        }
      }

      // If not in inventory, check if this character was the last known owner
      if (!found &&
          context->items.find((long long)mItem) == context->items.end()) {
        if (g_itemParentCache[mItem] == inv) {
          items.push_back(mItem);

          // Also add contents if dragging a container
          if (mItem->itemFunction == ITEM_CONTAINER) {
            std::vector<Item *> sub =
                getAllItems(((ContainerItem *)mItem)->inventory);
            items.insert(items.end(), sub.begin(), sub.end());
          }
        }
      }
    }

    for (size_t i = 0; i < items.size(); ++i) {
      s::item *si = readItem(items[i], ch, NULL);
      if (si) {
        si->parent = res;
        if (context)
          context->addItem(si);
        else
          res->items[si->id] = si;
      }
    }
  }

  // Serialise appearance
  GameData *gd = (GameData *)ch->getAppearanceData();
  if (gd) {
    std::stringstream ss;
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

    // Gender explicitly
    ss << "g::" << (ch->isFemale() ? 1 : 0) << "#";
    res->appearance = ss.str();
  }

  // Combat Target
  hand targetHand;
  typedef void(__thiscall * GetAttackTargetFn)(const Character *, hand *);
  static GetAttackTargetFn getTargetFn =
      (GetAttackTargetFn)((intptr_t)GetModuleHandleA(NULL) + 0x435020);
  getTargetFn(ch, &targetHand);
  res->targetID = getPublicID(targetHand.getCharacter());

  // Attackers
  CombatClass *cc = ch->getCombatClass();
  if (cc) {
    const lektor<hand> &atk = cc->getAttackers();
    if (atk.stuff) {
      for (unsigned int i = 0; i < atk.count; i++) {
        Character *attacker = atk.stuff[i].getCharacter();
        long long pubID = getPublicID(attacker);
        if (pubID != 0)
          res->attackers.push_back(pubID);
      }
    }
  }

  return res;
}

inline s::squad *readSquad(Platoon *p, s::faction *context = NULL) {
  if (!p || !p->activePlatoon)
    return nullptr;
  s::squad *res = new s::squad();
  res->id = (long long)p;

  lektor<RootObject *> &items = p->activePlatoon->things;
  for (unsigned int i = 0; i < items.count; ++i) {
    if (items.stuff[i] && items.stuff[i]->getDataType() == CHARACTER) {
      s::character *sc = readCharacter((Character *)items.stuff[i], context);
      if (sc)
        res->characters[sc->id] = sc;
    }
  }
  return res;
}

inline s::faction *readFaction(Faction *f) {
  if (!f)
    return nullptr;
  s::faction *res = new s::faction();
  res->sid = (f->data) ? f->data->stringID : f->name;

  const lektor<Platoon *> *platoons = f->getActivePlatoons();
  if (platoons && platoons->stuff) {
    for (unsigned int i = 0; i < platoons->count; ++i) {
      s::squad *ss = readSquad(platoons->stuff[i], res);
      if (ss)
        res->squads[ss->id] = ss;
    }
  }
  return res;
}

inline s::world *readWorld() {
  if (!ou || !ou->factionMgr)
    return nullptr;
  s::world *res = new s::world();
  res->gameSpeed = ou->frameSpeedMult;

  // Always read the player faction (204-gamedata.base) which has the actual
  // squads/characters
  Faction *f = ou->factionMgr->getFactionByStringID("204-gamedata.base");
  if (f) {
    s::faction *sf = readFaction(f);
    if (sf) {
      // Rename it to our assigned faction ID before sending to server
      sf->sid = myName;
      res->factions[sf->sid] = sf;
    }
  } else {
    c::log("[WARNING] Player faction (204-gamedata.base) not found!");
  }

  if (oldWorld)
    delete oldWorld;
  oldWorld = res;
  return res;
}

inline float getGameSpeed() {
  if (!ou)
    return 1.0f;
  if (ou->paused)
    return 0.0f;
  return ou->frameSpeedMult;
}

inline void setGameSpeed(float speed) {
  if (!ou)
    return;
  if (speed == 0.0f) {
    if (!ou->isPaused())
      ou->togglePause(true);
  } else {
    if (ou->isPaused())
      ou->togglePause(false);
    ou->setGameSpeed(speed, false);
  }
}
#include ".network.h"

static int targetFPS = 30;
extern bool g_trackCombat;

inline void fixedUpdate() {
  if (ou) {
    float currentSpeed = getGameSpeed();
    if (abs(currentSpeed - lastSentSpeed) > 0.001f) {
      lastSentSpeed = currentSpeed;
      localSpeedVer++;
      char buf[64];
      sprintf(buf, "%f %d", currentSpeed, localSpeedVer);
      sendSignal("speed", buf);
    }

    static unsigned long lastSync = 0;
    static unsigned long lastCombatLog = 0;
    unsigned long now = GetTickCount();

    if (now - lastSync >= 33) { // 30 FPS for SYNC
      lastSync = now;
      s::world *w = readWorld();
      if (w) {
        sendSignal("SYNC", w->serialise());
      }
    }
  }
}

inline void update() {
  // Runs every frame uncapped
}
// Makes a remote character speak by their public ID
inline void remoteChat(long long charID, const std::string &msg) {
  for (std::map<std::string, remoteMapper>::iterator it = g_mappers.begin();
       it != g_mappers.end(); ++it) {
    if (it->second.publicToPrivateIDCharacters.count(charID)) {
      Character *remoteChar = it->second.publicToPrivateIDCharacters[charID];
      if (remoteChar) {
        chat(remoteChar, msg);
      }
      return;
    }
  }
}

// Forward Declarations
void applyRemoteSquad(s::squad *s, Platoon *kp, remoteMapper &mapper);
void applyRemoteCharacter(s::character *c, Character *kc, remoteMapper &mapper);
void applyRemoteItems(s::character *sChar, Inventory *inv,
                      remoteMapper &mapper);

// Helper functions for Squad Management
inline Platoon *createSquad(Faction *f, std::string name, Ogre::Vector3 pos) {
  if (!f)
    return nullptr;
  Platoon *p = f->createNewEmptyActivePlatoon(nullptr, true, pos);
  if (p && p->activePlatoon) {
    p->activePlatoon->setName(name);
  }
  return p;
}

inline int deleteSquadsByName(Faction *f, std::string name) {
  if (!f)
    return 0;
  const lektor<Platoon *> *platoons = f->getActivePlatoons();
  if (!platoons || !platoons->stuff)
    return 0;

  int count = 0;
  std::vector<Platoon *> toDelete;
  for (unsigned int i = 0; i < platoons->count; ++i) {
    Platoon *p = platoons->stuff[i];
    if (p && p->activePlatoon && p->activePlatoon->getName() == name) {
      toDelete.push_back(p);
    }
  }

  for (size_t i = 0; i < toDelete.size(); ++i) {
    f->destroyPlatoon(toDelete[i]);
    count++;
  }
  return count;
}

inline Platoon *createRemoteSquad(Faction *kf, Ogre::Vector3 spawnPos,
                                  long long squadID, remoteMapper &mapper) {
  Platoon *kp = kf->createNewEmptyActivePlatoon(nullptr, false, spawnPos);
  if (kp) {
    if (kp->activePlatoon)
      kp->activePlatoon->setName("RemoteSquad");
    mapper.publicToPrivateIDSquads[squadID] = kp;
    mapper.privateToPublicIDSquads[kp] = squadID;
    // c::log("[DEBUG]: Created Remote Squad (ID: " + ToString(squadID) + ")");
  } else {
    // c::log("[DEBUG]: Failed to create Remote Squad (ID: " + ToString(squadID)
    // +
    //        ")");
  }
  return kp;
}

inline void
cleanupDisconnectedFactions(const std::set<std::string> &activeFactions) {
  std::vector<std::string> toRemove;
  for (std::map<std::string, remoteMapper>::iterator it = g_mappers.begin();
       it != g_mappers.end(); ++it) {
    if (activeFactions.find(it->first) == activeFactions.end()) {
      toRemove.push_back(it->first);
    }
  }

  for (size_t i = 0; i < toRemove.size(); ++i) {
    std::string factionId = toRemove[i];
    // c::log("[DEBUG] Cleaning up disconnected faction: " + factionId);

    if (!ou || !ou->factionMgr)
      continue;

    Faction *kf = ou->factionMgr->getFactionByStringID(factionId);
    if (kf && kf->activePlatoons.stuff) {
      remoteMapper &mapper = g_mappers[factionId];

      // Delete all squads for this faction
      std::vector<Platoon *> toDeleteSquads;
      for (unsigned int j = 0; j < kf->activePlatoons.count; ++j) {
        Platoon *p = kf->activePlatoons.stuff[j];
        if (mapper.privateToPublicIDSquads.count(p)) {
          toDeleteSquads.push_back(p);
        }
      }

      for (size_t j = 0; j < toDeleteSquads.size(); ++j) {
        kf->destroyPlatoon(toDeleteSquads[j]);
      }
    }

    g_mappers.erase(factionId);
  }
}

void applyRemoteFaction(s::faction *f) {
  // c::log("[DEBUG] applyRemoteFaction called for: " + f->sid);

  if (!ou || !ou->factionMgr) {
    // c::log("[DEBUG] applyRemoteFaction: ou or factionMgr is null");
    return;
  }

  Faction *kf = ou->factionMgr->getFactionByStringID(f->sid);
  if (!kf) {
    kf = ou->factionMgr->getOrCreateFaction(f->sid, f->sid);
    // c::log("[DEBUG]: Discovered/Created Remote Faction: " + f->sid);
  } else {
    // c::log("[DEBUG]: Found existing faction: " + f->sid);
  }
  if (!kf) {
    // c::log("[DEBUG] applyRemoteFaction: Failed to get/create faction");
    return;
  }

  remoteMapper &mapper = g_mappers[f->sid];

  std::set<long long> processedSquads;

  // c::log("[DEBUG] Remote faction has " + ToString(f->squads.size()) +
  //        " squads");

  // 1. Sync / Create Squads
  for (std::map<long long, s::squad *>::iterator it = f->squads.begin();
       it != f->squads.end(); ++it) {
    long long squadID = it->first;
    s::squad *sSquad = it->second;
    processedSquads.insert(squadID);

    Platoon *kp = nullptr;
    if (mapper.publicToPrivateIDSquads.count(squadID)) {
      kp = mapper.publicToPrivateIDSquads[squadID];
    }

    if (!kp) {
      // Create new squad
      Ogre::Vector3 spawnPos(0, 0, 0);
      if (!sSquad->characters.empty()) {
        s::character *firstChar = sSquad->characters.begin()->second;
        spawnPos =
            Ogre::Vector3(firstChar->pos.x, firstChar->pos.y, firstChar->pos.z);
      }

      kp = createRemoteSquad(kf, spawnPos, squadID, mapper);
    }

    if (kp) {
      applyRemoteSquad(sSquad, kp, mapper);
    }
  }

  // 2. Delete removed squads
  if (kf->activePlatoons.stuff) {
    std::vector<Platoon *> toDelete;
    for (unsigned int i = 0; i < kf->activePlatoons.count; ++i) {
      Platoon *p = kf->activePlatoons.stuff[i];
      if (mapper.privateToPublicIDSquads.count(p)) {
        long long remoteID = mapper.privateToPublicIDSquads[p];
        if (processedSquads.find(remoteID) == processedSquads.end()) {
          toDelete.push_back(p);
        }
      }
    }

    for (size_t i = 0; i < toDelete.size(); ++i) {
      Platoon *p = toDelete[i];
      long long rID = mapper.privateToPublicIDSquads[p];
      mapper.privateToPublicIDSquads.erase(p);
      mapper.publicToPrivateIDSquads.erase(rID);
      kf->destroyPlatoon(p);
    }
  }
}

void applyRemoteItems(s::character *sChar, Inventory *inv,
                      remoteMapper &mapper) {
  if (!inv)
    return;

  // Build set of item IDs that should exist
  std::set<long long> shouldExistIDs;
  for (std::map<long long, s::item *>::iterator itemIt = sChar->items.begin();
       itemIt != sChar->items.end(); ++itemIt) {
    shouldExistIDs.insert(itemIt->first);
  }

  // 1. CLEANUP / REMOVE ITEMS FIRST
  // Build a set of all item pointers that should exist
  std::set<Item *> shouldExist;
  for (std::map<long long, Item *>::iterator it =
           mapper.publicToPrivateIDItems.begin();
       it != mapper.publicToPrivateIDItems.end(); ++it) {
    if (shouldExistIDs.count(it->first)) {
      shouldExist.insert(it->second);
    }
  }

  // Remove ALL items that shouldn't exist (tracked or not)
  std::vector<Item *> toDelete;
  std::vector<Item *> allItems = getAllItems(inv);
  for (size_t i = 0; i < allItems.size(); ++i) {
    Item *item = allItems[i];
    if (shouldExist.find(item) == shouldExist.end()) {
      // This item shouldn't exist - remove it
      toDelete.push_back(item);
    }
  }

  for (size_t i = 0; i < toDelete.size(); ++i) {
    Item *item = toDelete[i];
    // Remove from mapper if it was tracked
    if (mapper.privateToPublicIDItems.count(item)) {
      long long rID = mapper.privateToPublicIDItems[item];
      mapper.privateToPublicIDItems.erase(item);
      mapper.publicToPrivateIDItems.erase(rID);
    }
    bool success = destroyItem(item, item->quantity);
    if (!success) {
      std::string itemName = (item->data) ? item->data->stringID : "UNKNOWN";
      c::log("[DEBUG] Failed to destroy item: " + itemName);
    }
  }

  // 2. SPAWN / UPDATE ITEMS (Two passes to ensure bags exist before contents)
  for (int pass = 0; pass < 2; ++pass) {
    for (std::map<long long, s::item *>::iterator itemIt = sChar->items.begin();
         itemIt != sChar->items.end(); ++itemIt) {
      long long itemID = itemIt->first;
      s::item *sItem = itemIt->second;

      // Skip items with bags in first pass
      if (pass == 0 && sItem->bagid != 0)
        continue;
      // Skip items without bags in second pass
      if (pass == 1 && sItem->bagid == 0)
        continue;

      Item *kItem = nullptr;
      if (mapper.publicToPrivateIDItems.count(itemID)) {
        kItem = mapper.publicToPrivateIDItems[itemID];
      }

      Inventory *targetInv = inv;
      if (sItem->bagid != 0 &&
          mapper.publicToPrivateIDItems.count(sItem->bagid)) {
        Item *bag = mapper.publicToPrivateIDItems[sItem->bagid];
        if (bag) {
          Inventory *bagInv = bag->getInventory();
          if (bagInv)
            targetInv = bagInv;
        }
      }

      if (!kItem) {
        // Item not found locally -> SPAWN IT
        GameData *itemData = findData(sItem->sid);
        if (itemData) {
          kItem = give(targetInv, itemData, sItem->quantity);
          if (kItem) {
            mapper.publicToPrivateIDItems[itemID] = kItem;
            mapper.privateToPublicIDItems[kItem] = itemID;

            // Set charges
            kItem->chargesLeft = sItem->charges;

            // Move item to correct section and position
            moveItem(kItem, sItem->section, sItem->x, sItem->y, targetInv);

            // Set material/manufacturer if available
            if (!sItem->material.empty() && sItem->material != "NONE") {
              GameData *mat = findData(sItem->material);
              if (mat)
                kItem->materialData = mat;
            }
            if (!sItem->manufacturer.empty() && sItem->manufacturer != "NONE") {
              GameData *manuf = findData(sItem->manufacturer);
              if (manuf)
                kItem->manufacturerData = manuf;
            }
          }
        }
      } else {
        // Update existing item properties
        kItem->quantity = sItem->quantity;
        kItem->chargesLeft = sItem->charges;

        // Move item to correct section/position
        moveItem(kItem, sItem->section, sItem->x, sItem->y, targetInv);
      }
    }
  }
}

void applyRemoteSquad(s::squad *s, Platoon *kp, remoteMapper &mapper) {
  if (!kp || !kp->activePlatoon)
    return;

  std::set<long long> processedChars;

  // ---------------------------------------------------------
  // 1. SPAWN / UPDATE CHARACTERS
  // ---------------------------------------------------------
  for (std::map<long long, s::character *>::iterator it = s->characters.begin();
       it != s->characters.end(); ++it) {
    long long charID = it->first;
    s::character *sChar = it->second;
    processedChars.insert(charID);

    Character *kc = nullptr;
    if (mapper.publicToPrivateIDCharacters.count(charID)) {
      kc = mapper.publicToPrivateIDCharacters[charID];
    }

    if (!kc) {
      // Character not found locally -> SPAWN IT
      Ogre::Vector3 pos(sChar->pos.x, sChar->pos.y, sChar->pos.z);
      GameData *templateData = findData(sChar->sid);
      if (!templateData)
        templateData = findData("Wanderer");

      if (templateData) {
        kc = spawn(kp, templateData, pos, 1);
        if (kc) {
          mapper.publicToPrivateIDCharacters[charID] = kc;
          mapper.privateToPublicIDCharacters[kc] = charID;
          // c::log("[DEBUG]: Spawned Remote Char: " + sChar->name);
        } else {
          // c::log("[DEBUG]: Failed to spawn Remote Char: " + sChar->name);
        }
      }
    }

    if (kc) {
      applyRemoteCharacter(sChar, kc, mapper);

      // Sync items
      Inventory *inv = kc->getInventory();
      if (inv) {
        applyRemoteItems(sChar, inv, mapper);
      }
    }
  }

  // ---------------------------------------------------------
  // 2. CLEANUP / DESPAWN CHARACTERS
  // ---------------------------------------------------------
  std::vector<Character *> toDelete;
  lektor<RootObject *> &things = kp->activePlatoon->things;
  for (unsigned int i = 0; i < things.count; ++i) {
    if (!things.stuff[i] || things.stuff[i]->getDataType() != CHARACTER)
      continue;
    Character *c = (Character *)things.stuff[i];

    // Only delete characters that we are tracking map-wise (synced chars)
    // If they are not in the processed set, they were removed remotely.
    if (mapper.privateToPublicIDCharacters.count(c)) {
      long long rID = mapper.privateToPublicIDCharacters[c];
      if (processedChars.find(rID) == processedChars.end()) {
        toDelete.push_back(c);
      }
    }
  }

  for (size_t i = 0; i < toDelete.size(); ++i) {
    Character *c = toDelete[i];
    long long rID = mapper.privateToPublicIDCharacters[c];
    mapper.privateToPublicIDCharacters.erase(c);
    mapper.publicToPrivateIDCharacters.erase(rID);
    destroy(c);
  }
}

void applyRemoteCharacter(s::character *sc, Character *kc,
                          remoteMapper &mapper) {
  if (!kc)
    return;

  // Update name
  if (kc->getName() != sc->name) {
    kc->setName(sc->name);
  }

  // Update position and rotation using CharMovement
  Ogre::Vector3 targetPos(sc->pos.x, sc->pos.y, sc->pos.z);
  Ogre::Quaternion targetRot(sc->rot.w, sc->rot.x, sc->rot.y, sc->rot.z);

  // CharMovement offset in Character is 0x640
  void *cm = *(void **)((char *)kc + 0x640);
  if (cm) {
    // _setPositionDirectionAndTeleport RVA: 0x65E260
    typedef void(__thiscall * SetPosDirFunc)(void *, const Ogre::Vector3 &,
                                             const Ogre::Quaternion &);
    static SetPosDirFunc setPosDir =
        (SetPosDirFunc)((intptr_t)GetModuleHandleA(NULL) + 0x65E260);
    setPosDir(cm, targetPos, targetRot);

    // Check if character is moving (task 29 = MOVE_CUS_ORDERED)
    if (sc->task == 29) {
      Ogre::Vector3 dest(sc->moveTo.x, sc->moveTo.y, sc->moveTo.z);
      // setDestination RVA: 0x660AF0
      typedef void(__thiscall * SetDestFunc)(void *, const Ogre::Vector3 &, int,
                                             bool);
      static SetDestFunc setDest =
          (SetDestFunc)((intptr_t)GetModuleHandleA(NULL) + 0x660AF0);
      setDest(cm, dest, 2, false); // MED_PRIORITY = 2
    } else {
      // halt RVA: 0x65F4F0
      typedef void(__thiscall * HaltFunc)(void *);
      static HaltFunc halt =
          (HaltFunc)((intptr_t)GetModuleHandleA(NULL) + 0x65F4F0);
      halt(cm);
    }
  } else {
    // Fallback: direct assignment
    kc->pos = targetPos;
    kc->rot = targetRot;
  }

  // Combat sync - use addOrder for safe target setting (like v0.3)
  long long oldTargetID = mapper.lastTargetID[sc->id];
  if (sc->targetID != 0) {
    if (sc->targetID != oldTargetID) {
      Character *targetChar = findCharacterByPublicID(sc->targetID);
      if (targetChar && targetChar != kc) {
        // Use addOrder with FOCUSED_MELEE_ATTACK - safe AI approach
        kc->addOrder(NULL, FOCUSED_MELEE_ATTACK, targetChar, false, true,
                     targetChar->getPosition());

        // char buf[256];
        // sprintf(buf, "[COMBAT] %s target %llu", kc->getName().c_str(),
        // sc->targetID); LogToChat(buf);

        mapper.lastTargetID[sc->id] = sc->targetID;
      }
    }
  } else if (oldTargetID != 0) {
    // No target - set to IDLE (task 14) to cancel combat
    kc->addOrder(NULL, IDLE, nullptr, false, true, kc->getPosition());

    // std::string msg = "[COMBAT] " + kc->getName() + " target NONE";
    // LogToChat(msg);

    mapper.lastTargetID[sc->id] = 0;
  }

  // Sync appearance only if changed
  long long charID = sc->id;
  if (!sc->appearance.empty() &&
      mapper.lastAppearance[charID] != sc->appearance) {
    mapper.lastAppearance[charID] = sc->appearance;

    // Decode appearance and apply
    GameData *gd = new GameData();
    std::stringstream ss(sc->appearance);
    std::string segment;
    bool isFemale = false;

    while (std::getline(ss, segment, '#')) {
      if (segment.size() < 4)
        continue;
      size_t first = segment.find(':');
      if (first == std::string::npos)
        continue;
      size_t second = segment.find(':', first + 1);
      if (second == std::string::npos)
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
      else if (type == 'b')
        gd->bdata[key] = (atoi(val.c_str()) == 1);
      else if (type == 'g') {
        isFemale = (atoi(val.c_str()) == 1);
        gd->bdata["female"] = isFemale;
      } else if (type == 'v') {
        Ogre::Vector3 v;
        sscanf(val.c_str(), "%f,%f,%f", &v.x, &v.y, &v.z);
        gd->vecdata[key] = v;
      } else if (type == 'q') {
        Ogre::Quaternion q;
        sscanf(val.c_str(), "%f,%f,%f,%f", &q.w, &q.x, &q.y, &q.z);
        gd->quatdata[key] = q;
      }
    }

    kc->setAppearanceData((GameDataCopyStandalone *)gd);

    // Also set gender on character's main data
    GameData *charData = kc->getGameData();
    if (charData) {
      charData->bdata["female"] = isFemale;
    }
  }
}
/*
void applyRemoteFaction_IGNORED(s::faction *f) {
  if doesnt
    exist then spawn squad, if not found in data then delete that remote squad
}
void applyRemoteSquad(s::squad *s) {}
void applyRemoteCharacter(s::character *c) {}
*/
