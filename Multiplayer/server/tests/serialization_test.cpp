#include "../../.struct.h"
#include <cassert>
#include <iostream>

// Simple test runner helper
#define TEST_ASSERT(cond, msg)                                                 \
  if (!(cond)) {                                                               \
    std::cerr << "FAILED: " << msg << std::endl;                               \
    exit(1);                                                                   \
  } else {                                                                     \
    std::cout << "PASSED: " << msg << std::endl;                               \
  }

int main() {
  std::cout << "Starting Serialization Unit Test..." << std::endl;

  // 1. Mock Data
  s::character og;
  og.id = 999;
  og.sid = "test_sid";
  og.name = "Beep";
  og.pos = {10.0f, 20.0f, 30.0f};
  og.rot = {1.0f, 0.0f, 0.0f, 0.0f}; // w,x,y,z
  og.task = 15;                      // IDLEish
  og.moveTo = {100.0f, 200.0f, 300.0f};
  og.targetID = 12345;
  og.attackers.push_back(111);
  og.attackers.push_back(222);

  s::item *it1 = new s::item();
  it1->id = 1;
  it1->sid = "iron_club";
  it1->quantity = 1;
  it1->x = 0;
  it1->y = 0;
  og.items[it1->id] = it1;

  s::item *it2 = new s::item();
  it2->id = 2;
  it2->sid = "dried_meat";
  it2->quantity = 5;
  it2->x = 2;
  it2->y = 3;
  og.items[it2->id] = it2;

  og.appearance = "f:body_scale:1.2#b:foid:1#g::1#";

  // 2. Serialize
  std::string serialised = og.serialise();
  std::cout << "Serialised string: " << serialised << std::endl;

  // 3. Deserialize
  s::character *current = s::character::deserialise(serialised, NULL);

  // 4. Compare
  TEST_ASSERT(current->id == og.id, "ID matches");
  TEST_ASSERT(current->sid == og.sid, "SID matches");
  TEST_ASSERT(current->name == og.name, "Name matches");
  TEST_ASSERT(current->pos.x == og.pos.x && current->pos.y == og.pos.y &&
                  current->pos.z == og.pos.z,
              "Position matches");
  TEST_ASSERT(current->rot.w == og.rot.w && current->rot.x == og.rot.x &&
                  current->rot.y == og.rot.y && current->rot.z == og.rot.z,
              "Rotation matches");
  TEST_ASSERT(current->task == og.task, "Task matches");
  TEST_ASSERT(current->moveTo.x == og.moveTo.x, "MoveTo X matches");
  TEST_ASSERT(current->targetID == og.targetID, "TargetID matches");
  TEST_ASSERT(current->attackers.size() == 2, "Attackers count matches");
  TEST_ASSERT(current->attackers[1] == 222, "Attacker 2 ID matches");

  TEST_ASSERT(current->items.size() == 2, "Item count matches");
  TEST_ASSERT(current->items[1]->sid == "iron_club", "Item 1 SID matches");
  TEST_ASSERT(current->items[2]->quantity == 5, "Item 2 quantity matches");
  TEST_ASSERT(current->items[2]->x == 2 && current->items[2]->y == 3,
              "Item 2 coordinates match");

  TEST_ASSERT(current->appearance == og.appearance, "Appearance data matches");

  std::cout << "\nAll Unit Tests Passed Successfully!" << std::endl;

  // Cleanup
  delete it1;
  delete it2;
  delete current;

  return 0;
}
