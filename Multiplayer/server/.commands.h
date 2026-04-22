#pragma once
#include "registry.h"
#include <shellapi.h>

#include <direct.h>

inline void InitCommands() {
  reg("test", []() {
    desc("check if logging works");
    c::log("Server terminal text");
    c::broadcast("Server broadcast text");
  });

  reg("client", []() {
    desc("launch a game client");
    if (discovery_mode())
      return;

    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path = buffer;
    // .../mods/Multiplayer/server.exe -> .../
    path = path.substr(0, path.find_last_of("\\/"));
    // .../mods/Multiplayer -> .../mods
    path = path.substr(0, path.find_last_of("\\/"));
    // .../mods -> .../KenshiRoot
    path = path.substr(0, path.find_last_of("\\/"));

    std::string fullPath = path + "\\kenshi_GOG_1.0.65_x64.exe";
    c::log("Launching: " + fullPath);

    ShellExecuteA(NULL, "open", fullPath.c_str(), NULL, path.c_str(), SW_SHOW);
  });

  reg("clients", []() {
    desc("Launch two clients");
    if (discovery_mode())
      return;

    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path = buffer;
    path = path.substr(0, path.find_last_of("\\/"));
    path = path.substr(0, path.find_last_of("\\/"));
    path = path.substr(0, path.find_last_of("\\/"));

    std::string fullPath = path + "\\kenshi_GOG_1.0.65_x64.exe";
    c::log("Launching #1: " + fullPath);
    ShellExecuteA(NULL, "open", fullPath.c_str(), NULL, path.c_str(), SW_SHOW);
    Sleep(1000);
    c::log("Launching #2");
    ShellExecuteA(NULL, "open", fullPath.c_str(), NULL, path.c_str(), SW_SHOW);
  });
}