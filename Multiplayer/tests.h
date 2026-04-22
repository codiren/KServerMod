#pragma once

inline void runAllTests();

inline void InitTests() {
  reg("test", "Run all automated tests",
      [](const CmdArgs &args) { runAllTests(); });
}

#include "tests/character_tests.h"
#include "tests/item_tests.h"


inline void runAllTests() {
  c::log("========================================");
  c::log("RUNNING TESTS");
  c::log("========================================");

  runItemTests();
  runCharacterTests();

  c::log("========================================");
}
