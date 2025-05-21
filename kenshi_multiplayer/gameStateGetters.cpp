#include "pch.h"
#include "gameState.h"
#include "offsets.h"
#include <string>
#include <map>
namespace gameState {
	std::string getSpeed() {
		if(gameWorld->paused)return std::to_string(0.0f);
		return std::to_string(gameWorld->gameSpeed);
	}
	std::string getFaction() {
		char* factionStringPtr = reinterpret_cast<char*>(moduleBase + offsets::factionString);
		return std::string(factionStringPtr, 17);
	}
	std::string getOwnCharName() {
		static std::map<std::string, std::string> factionToChar = {
		{ "204-gamedata.base", "!fail" },
		{ "10-multiplayr.mod", "Player 1" },
		{ "12-multiplayr.mod", "Player 2" }
		};
		std::string faction = getFaction();
		auto it = factionToChar.find(faction);
		return (it != factionToChar.end()) ? it->second : "!fail";
	}
	std::string getOtherCharName() {
		static std::map<std::string, std::string> factionToChar = {
		{ "204-gamedata.base", "!fail" },
		{ "10-multiplayr.mod", "Player 2" },
		{ "12-multiplayr.mod", "Player 1" }
		};
		std::string faction = getFaction();
		auto it = factionToChar.find(faction);
		return (it != factionToChar.end()) ? it->second : "!fail";
	}
	std::string getPlayer() {
		if (player == 0)return "0,0,0";
		return player->movement->position->toString();
	}
	std::string getPlayer1() {
		if (!otherplayers||!player)return "-5139.11,158.019,345.631";
		if ("Player 1" == getOtherCharName()) {
			return otherplayers->movement->position->toString();
		}
		else {
			return player->movement->position->toString();
		}
	}
	std::string getPlayer2() {
		if (!otherplayers || !player)return "-5139.11,158.019,345.631";
		if ("Player 2" == getOtherCharName()) {
			return otherplayers->movement->position->toString();
		}
		else {
			return player->movement->position->toString();
		}
	}
}