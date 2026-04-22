#pragma once
#include <cstdlib>
#include <map>
#include <sstream>
#include <stddef.h>
#include <string>
#include <vector>

struct vec3 {
  float x, y, z;
};
struct quat {
  float w, x, y, z;
};

namespace s {

struct world;
struct faction;
struct character;

struct item {
  long long id;
  long long bagid;
  std::string sid;
  int x;
  int y;
  int quantity;
  float charges;
  std::string material;
  std::string manufacturer;
  int level;
  std::string section;

  s::character *parent; // DONT SERIALISE

  std::string serialise() const {
    std::stringstream ss;
    ss << id << "," << bagid << "," << sid << "," << x << "," << y << ","
       << quantity << "," << charges << "," << material << "," << manufacturer
       << "," << level << "," << section;
    return ss.str();
  }

  ~item() {}
  static item *deserialise(std::string data, s::faction *context = NULL);
};

struct character {
  long long id;
  std::string sid;
  std::string name;
  vec3 pos;
  quat rot;
  int task;
  vec3 moveTo;
  long long targetID;
  std::vector<long long> attackers;
  std::map<long long, s::item *> items;
  std::string appearance;

  std::string serialise() const {
    std::stringstream ss;
    ss << id << "," << sid << "," << name << "," << pos.x << "," << pos.y << ","
       << pos.z << "," << rot.w << "," << rot.x << "," << rot.y << "," << rot.z
       << "," << task << "," << moveTo.x << "," << moveTo.y << "," << moveTo.z
       << "," << targetID << ",#";
    for (size_t i = 0; i < attackers.size(); ++i) {
      if (i > 0)
        ss << ":";
      ss << attackers[i];
    }
    ss << "#(";

    for (std::map<long long, s::item *>::const_iterator it = items.begin();
         it != items.end(); ++it) {
      if (it != items.begin())
        ss << ";";
      ss << it->second->serialise();
    }
    ss << ")";

    if (!appearance.empty()) {
      ss << "@" << appearance;
    }
    return ss.str();
  }

  ~character() {}
  static character *deserialise(std::string data, s::faction *context = NULL);
};

struct squad {
  long long id;
  std::map<long long, s::character *> characters;

  std::string serialise() const {
    std::stringstream ss;
    ss << id << "{";
    for (std::map<long long, s::character *>::const_iterator it =
             characters.begin();
         it != characters.end(); ++it) {
      if (it != characters.begin())
        ss << "^";
      ss << it->second->serialise();
    }
    ss << "}";
    return ss.str();
  }

  ~squad() {
    for (std::map<long long, s::character *>::iterator it = characters.begin();
         it != characters.end(); ++it) {
      delete it->second;
    }
  }
  static squad *deserialise(std::string data, s::faction *context = NULL);
};

struct faction {
  std::string sid;
  std::map<long long, s::squad *> squads;
  std::map<long long, s::item *> items; // DONT SERIALISE

  std::string serialise() const {
    std::stringstream ss;
    ss << sid << "[";
    for (std::map<long long, s::squad *>::const_iterator it = squads.begin();
         it != squads.end(); ++it) {
      if (it != squads.begin())
        ss << "!";
      ss << it->second->serialise();
    }
    ss << "]";
    return ss.str();
  }

  ~faction() {
    for (std::map<long long, s::squad *>::iterator it = squads.begin();
         it != squads.end(); ++it) {
      delete it->second;
    }
    for (std::map<long long, s::item *>::iterator it = items.begin();
         it != items.end(); ++it) {
      delete it->second;
    }
  }
  static faction *deserialise(std::string data);

  item *getItem(long long id) {
    std::map<long long, s::item *>::iterator it = items.find(id);
    if (it != items.end())
      return it->second;
    return NULL;
  }
  void addItem(s::item *item) {
    if (item) {
      std::map<long long, s::item *>::iterator existing = items.find(item->id);
      if (existing != items.end() && existing->second != item) {
        delete existing->second;
      }
      items[item->id] = item;
      if (item->parent)
        item->parent->items[item->id] = item;
    }
  }
};

struct world {
  std::map<std::string, s::faction *> factions;
  float gameSpeed;
  std::string speedLastUpdatedBy; // DONT SERIALISE
  int gameSpeedVersion;

  std::string serialise() const {
    std::stringstream ss;
    ss << "WORLD:" << gameSpeed << ":" << gameSpeedVersion << ":";
    for (std::map<std::string, s::faction *>::const_iterator it =
             factions.begin();
         it != factions.end(); ++it) {
      if (it != factions.begin())
        ss << "|";
      ss << it->second->serialise();
    }
    return ss.str();
  }

  world() : gameSpeed(1.0f), gameSpeedVersion(0) {}

  world(std::string data) {
    gameSpeed = 1.0f;
    gameSpeedVersion = 0;
    if (data.find("WORLD:") != 0)
      return;

    // Find first colon (after WORLD:)
    size_t firstColon = data.find(':', 6);
    if (firstColon == std::string::npos)
      return;

    // Find second colon (after speed)
    size_t secondColon = data.find(':', firstColon + 1);
    if (secondColon == std::string::npos) {
      // Old format: WORLD:speed:factions
      gameSpeed = (float)atof(data.substr(6, firstColon - 6).c_str());
      std::string factionsData = data.substr(firstColon + 1);

      std::stringstream ss(factionsData);
      std::string factionData;
      while (std::getline(ss, factionData, '|')) {
        if (factionData.empty())
          continue;
        s::faction *f = s::faction::deserialise(factionData);
        if (f)
          factions[f->sid] = f;
      }
    } else {
      // New format: WORLD:speed:version:factions
      gameSpeed = (float)atof(data.substr(6, firstColon - 6).c_str());
      gameSpeedVersion = atoi(
          data.substr(firstColon + 1, secondColon - firstColon - 1).c_str());
      std::string factionsData = data.substr(secondColon + 1);

      std::stringstream ss(factionsData);
      std::string factionData;
      while (std::getline(ss, factionData, '|')) {
        if (factionData.empty())
          continue;
        s::faction *f = s::faction::deserialise(factionData);
        if (f)
          factions[f->sid] = f;
      }
    }
  }

  ~world() {
    for (std::map<std::string, s::faction *>::iterator it = factions.begin();
         it != factions.end(); ++it) {
      delete it->second;
    }
  }
};

// Implementations to handle forward declaration of s::faction
inline item *item::deserialise(std::string data, s::faction *context) {
  std::stringstream ss(data);
  std::string part;
  item *it = new item();
  if (std::getline(ss, part, ','))
    it->id = _atoi64(part.c_str());
  if (std::getline(ss, part, ','))
    it->bagid = _atoi64(part.c_str());
  std::getline(ss, it->sid, ',');
  if (std::getline(ss, part, ','))
    it->x = atoi(part.c_str());
  if (std::getline(ss, part, ','))
    it->y = atoi(part.c_str());
  if (std::getline(ss, part, ','))
    it->quantity = atoi(part.c_str());
  if (std::getline(ss, part, ','))
    it->charges = (float)atof(part.c_str());
  std::getline(ss, it->material, ',');
  std::getline(ss, it->manufacturer, ',');
  if (std::getline(ss, part, ','))
    it->level = atoi(part.c_str());
  std::getline(ss, it->section, ',');

  if (context != NULL) {
    context->addItem(it);
  }
  return it;
}

inline character *character::deserialise(std::string data,
                                         s::faction *context) {
  std::stringstream ss(data);
  std::string part;
  character *ch = new character();
  if (std::getline(ss, part, ','))
    ch->id = _atoi64(part.c_str());
  std::getline(ss, ch->sid, ',');
  std::getline(ss, ch->name, ',');
  if (std::getline(ss, part, ','))
    ch->pos.x = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->pos.y = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->pos.z = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->rot.w = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->rot.x = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->rot.y = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->rot.z = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->task = atoi(part.c_str());
  if (std::getline(ss, part, ','))
    ch->moveTo.x = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->moveTo.y = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->moveTo.z = (float)atof(part.c_str());
  if (std::getline(ss, part, ','))
    ch->targetID = _atoi64(part.c_str());

  if (std::getline(ss, part, '#')) {
    std::string attackersPart;
    if (std::getline(ss, attackersPart, '#')) {
      std::stringstream ssAttackers(attackersPart);
      std::string attackerIdStr;
      while (std::getline(ssAttackers, attackerIdStr, ':')) {
        if (!attackerIdStr.empty())
          ch->attackers.push_back(_atoi64(attackerIdStr.c_str()));
      }
    }
  }

  if (std::getline(ss, part, '(')) {
    std::string itemsPart;
    if (std::getline(ss, itemsPart, ')')) {
      std::stringstream ssItems(itemsPart);
      std::string itemData;
      while (std::getline(ssItems, itemData, ';')) {
        s::item *it = s::item::deserialise(itemData, NULL);
        if (it) {
          it->parent = ch;
          ch->items[it->id] = it;
          if (context)
            context->addItem(it);
        }
      }
    }

    std::string remaining;
    if (std::getline(ss, remaining, '@')) {
      std::getline(ss, ch->appearance);
    }
  }
  return ch;
}

inline squad *squad::deserialise(std::string data, s::faction *context) {
  std::stringstream ss(data);
  std::string part;
  squad *sq = new squad();
  if (std::getline(ss, part, '{'))
    sq->id = _atoi64(part.c_str());

  std::string charsPart;
  if (std::getline(ss, charsPart, '}')) {
    std::stringstream ssChars(charsPart);
    std::string charData;
    while (std::getline(ssChars, charData, '^')) {
      s::character *ch = s::character::deserialise(charData, context);
      if (ch)
        sq->characters[ch->id] = ch;
    }
  }
  return sq;
}

inline faction *faction::deserialise(std::string data) {
  std::stringstream ss(data);
  faction *f = new faction();
  if (!std::getline(ss, f->sid, '[')) {
    delete f;
    return NULL;
  }

  std::string squadsPart;
  if (std::getline(ss, squadsPart, ']')) {
    std::stringstream ssSquads(squadsPart);
    std::string squadData;
    while (std::getline(ssSquads, squadData, '!')) {
      s::squad *sq = s::squad::deserialise(squadData, f);
      if (sq)
        f->squads[sq->id] = sq;
    }
  }
  return f;
}

} // namespace s