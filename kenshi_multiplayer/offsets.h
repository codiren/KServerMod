#include <cstdint>

namespace offsets {//GOG 1.0.68 - x64
    constexpr uintptr_t factionString = 0x16C2F68;
    constexpr uintptr_t gameWorldOffset = 0x2133040;
    constexpr uintptr_t setPaused = 0x7876A0;
    constexpr uintptr_t charUpdateHook = 0x65F6C7;
    constexpr uintptr_t buildingUpdateHook = 0x9FAA57;

    constexpr uintptr_t itemSpawningHand = 0x1E395F8;
    constexpr uintptr_t itemSpawningMagic = 0x21334E0;
    constexpr uintptr_t spawnItemFunc = 0x2E41F;
    constexpr uintptr_t getSectionFromInvByName = 0x4FE3F;

    constexpr uintptr_t GameDataManagerMain = 0x2133060;
    constexpr uintptr_t GameDataManagerFoliage = 0x21331E0;
    constexpr uintptr_t GameDataManagerSquads = 0x2133360;
    
    constexpr uintptr_t spawnSquadBypass = 0x4FF47C;
    constexpr uintptr_t spawnSquadFuncCall = 0x4FFA88;
    constexpr uintptr_t squadSpawningHand = 0x21334E0;


}





/*

mov rax, [kenshi.exe+2246BF0]
mov r9, rax { (7FF9C11445E8) }
mov r8b,01 { 1 }
mov rdx, 0
mov rcx,1087010  //faction  !!!!!!!!!!!
call kenshi.exe+1AFA   //returns newly created platoon
mov r15, rax           //platoon
mov rax,[rax+000001D8] // get active platoon

*/