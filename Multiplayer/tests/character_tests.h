#pragma once

inline void runCharacterTests() {
  Character *refChar = getSelectedChar();
  VALID(refChar, "No character selected (needed for position reference).");

  ActivePlatoon *activePlat = refChar->getPlatoon();
  VALID(activePlat, "Selected character not in a platoon.");

  Platoon *refSquad = activePlat->me;
  VALID(refSquad, "Selected character not in a platoon.");

  Ogre::Vector3 testPos = refChar->getPosition();
  testPos.x += 10.0f;

  int passed = 0;
  int total = 3;

  // Test 1: Spawn Character
  GameData *charTemplate = findData("Wanderer");
  Character *testChar = nullptr;
  if (charTemplate) {
    testChar = spawn(refSquad, charTemplate, testPos, 1);
    if (testChar) {
      passed++;
    } else {
      c::log("[TEST] FAIL: Failed to spawn character");
    }
  } else {
    c::log("[TEST] FAIL: Wanderer template not found");
  }

  // Test 2: Block Mode Toggle (Before cleanup)
  if (testChar) {
    bool initialBlock = getBlockMode(testChar);
    if (initialBlock == false) {
      setBlockMode(testChar, true);
      bool newBlock = getBlockMode(testChar);
      if (newBlock == true) {
        passed++;
      } else {
        c::log("[TEST] FAIL: Block mode did not set to true");
      }
    } else {
      c::log("[TEST] FAIL: Initial block mode was not false");
    }
  }

  // Test 3: Cleanup - Delete Character
  if (testChar) {
    if (destroy(testChar)) {
      passed++;
    } else {
      c::log("[TEST] FAIL: Failed to destroy test character");
    }
  }

  std::stringstream ss;
  ss << "[Character Tests] " << passed << "/" << total;
  c::log(ss.str());
}
