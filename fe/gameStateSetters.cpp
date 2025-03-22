#include "pch.h"
#include "offsets.h"
#include "gameState.h"
#include "gameStateSetters.h"
#include "gameStateGetters.h"
#include "utils.h"
#include <string>
#include <iostream>

namespace gameState {
    extern structs::GameWorldClass* gameWorld;
    using GameWorldOffset = void(*)(structs::GameWorldClass*, bool);
    GameWorldOffset setPaused = reinterpret_cast<GameWorldOffset>(moduleBase + offsets::setPaused);

    void setSpeed(const std::string& data) {
        float desiredSpeed = std::stof(data);
        gameWorld->gameSpeed = desiredSpeed;
        if (desiredSpeed == 0&& gameWorld->paused == false) {//pause
            setPaused(gameWorld, true);
        }else if(desiredSpeed != 0 && gameWorld->paused == true) {//unpause
            gameWorld->paused = false;
            setPaused(gameWorld, true);
            setPaused(gameWorld, false);
        }
    }
    void setFaction(const std::string& data) {// can only be done before game is launched
        char* factionStringPtr = reinterpret_cast<char*>(moduleBase + offsets::factionString);
        DWORD oldProtect;//location is not writable so we cheat
        if (!VirtualProtect(factionStringPtr, data.size() + 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::cerr << "Failed to change memory protection!" << std::endl;
            return;
        }
        std::memcpy(factionStringPtr, data.c_str(), data.size() + 1);
        VirtualProtect(factionStringPtr, data.size() + 1, oldProtect, &oldProtect);
    }
    void setPlayers(const std::string& data) {
        if (otherplayers == 0)return;
        for (auto i : utils::split(data, ";")) {
            auto ii = utils::split(i, "=");
            if (ii[0] == getOtherCharName()) {
                otherplayers->movement->position->fromString(ii[1]);
            }
        }
    }
    void setPlayer1(const std::string& data) {
        if (!otherplayers)return;
        if ("Player 1" == getOtherCharName()) {
            otherplayers->movement->position->fromString(data);
        }
        else {
            //player->movement->position->fromString(data);
        }
    }
    void setPlayer2(const std::string& data) {
        if (!otherplayers)return;
        if ("Player 2" == getOtherCharName()) {
            otherplayers->movement->position->fromString(data);
        }
        else {
            //player->movement->position->fromString(data);
        }
    }
}