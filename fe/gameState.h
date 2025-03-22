#include <string>
#include <vector>
#include <map>
#include <queue>
#include "structs.h"
namespace gameState {
    extern uintptr_t moduleBase;
    extern structs::GameWorldClass* gameWorld;

    struct trackedVariable {
        std::string oldVal;
        void (*setter)(const std::string&);
        std::string(*getter)();
        bool changed = false;

        trackedVariable(void (*setter)(const std::string&), std::string(*getter)()) : setter(setter), getter(getter) {}
    };

    extern std::map<structs::AnimationClassHuman*, std::pair<std::string, long long>> chars;
    extern std::map<structs::Building*, std::pair<std::string, long long>> builds;
    extern std::map<std::string, structs::GameData*> DB;
    extern std::vector<void(*)(const std::string&)> oldsetters;
    extern std::vector<trackedVariable> variables;
    void setData(const std::string& data);
    void init();
    std::string getData();

    void setupHooks();
    extern structs::AnimationClassHuman* player;
    extern structs::AnimationClassHuman* otherplayers;
    void scanHeap();
}