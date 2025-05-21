#include "pch.h"
#include "gameStateSetters.h"
#include "gameStateGetters.h"
#include "gameState.h"
#include "offsets.h"
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <map>
#include "utils.h"
#include <queue>
namespace gameState {
    uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    structs::GameWorldClass* gameWorld = reinterpret_cast<structs::GameWorldClass*>(moduleBase + offsets::gameWorldOffset);

    std::vector < trackedVariable> variables;
    bool gameLoaded = false; // true = in main menu
    void init() {
        while (gameLoaded == false)Sleep(500);
        setupHooks();
        //variables.push_back(trackedVariable([](const std::string& _) {}, []() -> std::string { return "B";}));//test var
        variables.push_back(trackedVariable(setFaction, getFaction));
        variables.push_back(trackedVariable(setSpeed, getSpeed));
        //variables.push_back(trackedVariable(setPlayers, getPlayer));
        variables.push_back(trackedVariable(setPlayer1, getPlayer1));
        variables.push_back(trackedVariable(setPlayer2, getPlayer2));
    }
    void setData(const std::string& data) {
        std::string line;
        std::string key;
        std::istringstream iss(data);
        while (std::getline(iss, line)) {if (key == "") { key = line;continue; }
            trackedVariable& curVar = variables[std::stoi(key)];
            if(curVar.oldVal == curVar.getter()){//only accept server changes if local var wasn't changed
                if (line != curVar.getter()) {
                    //std::cout << line <<" " << curVar.getter() << "\n";
                    //std::cout << "applied from server: "<< key << "\n";
                    curVar.setter(line);
                }
                curVar.changed = false;
            } else {
                //std::cout << "changed" << key << "\n";
                curVar.changed = true;
            }
            curVar.oldVal = curVar.getter();
        key = "";}
    }
    std::string getData() {
        std::string data = "0\nB\n";
        for (int i = 1;i<variables.size();i++) {
            if (variables[i].changed == false)continue;
            data += std::to_string(i) + "\n" + variables[i].getter() + "\n";
            variables[i].changed = false;
        }
        return data;
    }
    std::map<structs::AnimationClassHuman*, std::pair<std::string, long long>> chars;
    std::map<structs::Building*, std::pair<std::string, long long>> builds;
    structs::AnimationClassHuman* player = 0;
    structs::AnimationClassHuman* otherplayers = 0;
    void onCharUpdate(structs::AnimationClassHuman* num) {
        structs::AnimationClassHuman* dataToSave = (structs::AnimationClassHuman*)num->character;
        if (chars.find(dataToSave) == chars.end()) {
            const char* chosenName = num->character->getName();
            chars[dataToSave] = std::make_pair(std::string(chosenName), 0);
            //std::cout << "new npc " << num << ", name: " << chosenName << "\n";
            if (strcmp(chosenName, getOwnCharName().c_str()) == 0) player = num;
            if (strcmp(chosenName, getOtherCharName().c_str()) == 0) otherplayers = num;
        }
        chars[dataToSave].second = GetTickCount64();
    }
    void onBuildingUpdate(structs::Building* num) {
        if (builds.find(num) == builds.end()) {
            const char* chosenName = num->getName();
            builds[num] = std::make_pair(std::string(chosenName), 0);
            //if (std::strcmp(chosenName, "Sitting spot (invisible)") == 0) return;
            //std::cout << "new build " << num << ", name: " << chosenName << "\n";
        }
        builds[num].second = GetTickCount64();
    }
    int squadSpawnBypassAmm = 0;
    structs::activePlatoon* bypassedPlatoon = 0;
    int checkRestoration;
    void bypassSquadSpawningCheck(structs::activePlatoon* actvPlatoon) {
        if (squadSpawnBypassAmm > 0
            && actvPlatoon->skipSpawningCheck3 == 0 //check3 memory needs to be 0, usually its a pointer?
            && actvPlatoon->skipSpawningCheck1 == 1) { // use the platoon that would be skipped naturally
            bypassedPlatoon = actvPlatoon;
            actvPlatoon->skipSpawningCheck1 = 0;//if 1 then skip spawning
            checkRestoration = actvPlatoon->skipSpawningCheck2;
            actvPlatoon->skipSpawningCheck2 = 0;//if greater than 0 also cancel
            squadSpawnBypassAmm--;//i hope those steps always spawn char and not 99% of the time
        }
    }
    std::queue<customStructs::Char> charSpawningQueue;
    char returns[sizeof(customStructs::Char)];// Char will live here
    void* __fastcall spawnSquadInjection(
        void* garbage,        //rcx
        structs::Faction* faction,        //rdx
        structs::Vector3   position,       //r8
        void* townOrNest,     //r9
        long long          stackOffset1,
        long long          stackOffset2,//  -1 cause function called
        long long          magic,          //[rsp+20]  999
        void* homeBuilding,   //[rsp+28]
        structs::GameData* squadTemplate,  //[rsp+30]
        structs::activePlatoon* squad     //[rsp+38]
    ) {
        //garbage = *(reinterpret_cast<void**>(moduleBase + offsets::squadSpawningHand));//+offsets::squadSpawningHand
        customStructs::Char* returnDude = reinterpret_cast<customStructs::Char*>(returns);
        returnDude->moduleBase = moduleBase + offsets::squadSpawningHand;
        if (bypassedPlatoon == 0 || bypassedPlatoon != squad){ // don't interrupt natural spawning
            returnDude->data = 0;
            return &returns;
        };
        bypassedPlatoon = 0;
        squad->skipSpawningCheck1 = 1;//restore original state
        squad->skipSpawningCheck2 = checkRestoration;//restore original state
        customStructs::Char dude = charSpawningQueue.front();charSpawningQueue.pop();
        returnDude->data = dude.data;
        returnDude->pos = dude.pos;
        return &returns;
    }
    /*void* spawnSquadInjection() {

        return &returns;
    }*/
    void setupHooks() {
        utils::createHook(
            moduleBase + offsets::charUpdateHook,
            { 0x48, 0x8B, 0x8B, 0x20, 0x03, 0x00, 0x00,   //mov rcx,[rbx+00000320]
              0x40, 0x88, 0xB3, 0x7C, 0x03, 0x00, 0x00 }, //mov [rbx+0000037C],sil
            {},
            &onCharUpdate,
            {}
        );
        utils::createHook(
            moduleBase + offsets::buildingUpdateHook,
            { 0x48, 0x8B, 0x43, 0x60,              //mov rax,[rbx+60]
             0x4C, 0x8B, 0x24, 0x28,               //mov r12,[rax+rbp]
             0x49, 0x8B, 0xCC,                     //mov rcx,r12
             0x49, 0x8B, 0x04, 0x24,               //mov rax,[r12]
             0xFF, 0x90, 0xD8, 0x00, 0x00, 0x00 }, //call qword ptr [rax+000000D8] 
            {},
            &onBuildingUpdate,//it shouldn't work cause arg is in rdx, but somehow it works so whatever
            {}
        );
        utils::createHook(
            moduleBase + offsets::spawnSquadBypass,
            {
                0x48, 0x8D, 0xAC, 0x24, 0x30, 0xFF, 0xFF, 0xFF, //- lea rbp,[rsp - 000000D0]
                0x48, 0x81, 0xEC, 0xD0, 0x01, 0x00, 0x00        //- sub rsp,000001D0
            },
            {0x52},//push rdx
            &bypassSquadSpawningCheck,
            {0x5A}//pop rdx
        );
        utils::createHook(
            moduleBase + offsets::spawnSquadFuncCall,
            {
                //  0x4C, 0x8B, 0x4E, 0x30                         // mov r9,[rsi+30]
                //, 0x4C, 0x8D, 0x45, 0xA0                        // lea r8,[rbp-60]
                  0x90, 0x90, 0x90, 0x90                         
              , 0x90, 0x90, 0x90, 0x90                        
              ,0x90,0x90,0x90,0x90,0x90,0x90,0x90
            //, 0x48, 0x8B, 0x0D, 0x49, 0x3A, 0xC3, 0x01     //- mov rcx,[kenshi.exe+21334E0]
            },
            { 0x52,0x41,0x52, //push rdx  push r10

            },
            & spawnSquadInjection, // TODO write this in assembler
            //& utils::nop,
            { 
                0x41,0x5A, 0x5A // pop r10, pop rdx
                ,0x48 ,0x8B ,0x48 ,0x18 //- mov rcx,[rax+38]
                , 0x48 , 0x8B , 0x09 //- mov rcx,[rcx]
                
                , 0x48, 0x83, 0xF8, 0x00        // cmp rax,00
                , 0x0F, 0x84, 0x15, 0x00, 0x00, 0x00  // je 204E001F
                , 0x4C, 0x8B, 0x08              // mov r9,[rax]
                , 0x4C, 0x89, 0x4C, 0x24, 0x30     // mov[rsp + 30],r9
                , 0x4C, 0x8D, 0x40, 0x08        // lea r8,[rax + 08]
                , 0x4C, 0x8B, 0x4E, 0x30        // mov r9,[rsi + 30]
                , 0xE9, 0x08, 0x00, 0x00, 0x00     // jmp 204E0027
                , 0x4C, 0x8B, 0x4E, 0x30        // mov r9,[rsi + 30]
                , 0x4C, 0x8D, 0x45, 0xA0        // lea r8,[rbp - 60]

            }
        );
    }
    std::map<std::string,structs::GameData*> DB;
    void scanHeap() {
        long long startTime;
        std::vector<uintptr_t> result;
        auto var = (*(reinterpret_cast<void**>(moduleBase + offsets::squadSpawningHand)));
        //std::cout << var << "\n";
        while (*(reinterpret_cast<void**>(moduleBase + offsets::squadSpawningHand)) == 0) {
            var = (*(reinterpret_cast<void**>(moduleBase + offsets::squadSpawningHand)));
            //std::cout << var << "\n";
            Sleep(500); }
        
        while (result.size() < 54949) {//if you have at least this ammount then probably everything was loaded
            Sleep(500);
            startTime = GetTickCount64();
            result = std::move(utils::scanMemoryForValue(gameState::moduleBase + offsets::GameDataManagerMain));
        }
        gameLoaded = true;
        int ii = 0;
        for (uintptr_t i : result) {
            structs::GameData* data = reinterpret_cast<structs::GameData*>(i-0x10);
            auto name = data->getName();
            if(utils::isValidName(name))DB[name] = data;
            /*else {
                std::cout << utils::isValidName(name) << " " << data;
                utils::pause();
            }*/
        }
        std::cout << "HeapScan: found " << DB.size() << " entries in " << ((GetTickCount64() - startTime) / 1000.) << "s.\n";
    }
}