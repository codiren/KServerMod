#pragma once

inline void runItemTests() {
  const char *testItems[] = {"Wooden Backpack", "Cap",
                             "Iron Club",       "Sake",
                             "Dried Meat",      "Lantern of Radiance"};

  Character *target = getSelectedChar();
  VALID(target, "No character selected.");

  Inventory *inv = target->getInventory();
  VALID(inv, "Target has no inventory.");

  int passed = 0;
  int total = 6;

  for (int i = 0; i < total; ++i) {
    std::string itemName = testItems[i];
    GameData *data = findData(itemName);
    if (!data) {
      c::log("[TEST] FAIL: Item data not found: " + itemName);
      continue;
    }

    // 1. Give
    Item *it = give(inv, data, 1);
    if (!it) {
      c::log("[TEST] FAIL: give failed for " + data->name);
      continue;
    }

    // 2. Take
    if (!destroyItem(it, 1)) {
      c::log("[TEST] FAIL: destroyItem failed for " + data->name);
      continue;
    }

    // 3. Verify
    std::vector<Item *> items = getAllItems(inv);
    bool stillExists = false;
    for (size_t j = 0; j < items.size(); ++j) {
      if (items[j] == it) {
        stillExists = true;
        break;
      }
    }

    if (stillExists) {
      c::log("[TEST] FAIL: " + data->name +
             " pointer still exists in inventory!");
    } else {
      passed++;
    }
  }

  std::stringstream ss;
  ss << "[Item Tests] " << passed << "/" << total;
  c::log(ss.str());
}
