#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include "utils.h"
#include <cstring>
namespace structs {
    class Inventory;
    class ProgressContainer;

    struct kenshiString {
        char string[0x10];
        int size;
        private:
        char padding[0x100];//important, otherwise crash
        public:
        kenshiString(std::string from) {
            memset(this, 0, sizeof(*this));//cyber janitor
            size = (static_cast<int>(from.size())<0x10 - 1)? static_cast<int>(from.size()) :0x10-1; // Leave space for null terminator

            // Copy the string content into 'string' array
            strncpy_s(string, sizeof(string), from.c_str(), size);

            // Null terminate the string
            string[size] = '\0';
        }
    };
    class GameWorldClass {
        //listOfFactions lektor<faction> Offset 0x4A8
        char padding1[0x700];
        public:
        float gameSpeed;     // Offset 0x700
        private:
        char padding2[0x1B5];
        public:
        bool paused;         // Offset 0x8B9
    };
    struct Vector3Raw {//size 16, cause logic
        float x;
        float y;
        float z;
    };
    class Vector3 {
        char padding1[0x20];
        public:
        float x;
        float y;
        float z;
        void fromString(const std::string& str) {
            std::stringstream ss(str);
            char comma;
            ss >> x >> comma >> y >> comma >> z;
        }
        void zeroes() { x = 0;y = 0;z = 0; }
        std::string toString() const {
            std::ostringstream oss;
            oss << x << "," << y << "," << z;
            return oss.str();
        }
    };
    class Building {
        char padding1[0x18];
        char name[0x48 - sizeof(padding1)]; // Offset 0x18
            public:
        float x;                      // Offset 0x48 (readonly)
        float y;                      // Offset 0x4C (readonly)
        float z;                      // Offset 0x50 (readonly)
            private:
        char padding2[0x164 - (0x50 + 0x4)]; // Adjust padding up to 0x164
            public:
        float condition;              // Offset 0x164
            private:
        char padding3[0x430 - (0x164 + 0x4)]; // Adjust padding up to 0x430
            public:
        Inventory* inventory;         // Offset 0x430
            private:
        char padding4[0x448 - (0x430 + 0x8)];
            public:
        ProgressContainer* production;// Offset 0x448
        __declspec(nothrow) const char* getName() const {
            __try { // Sometimes it's not name, but pointer to name
                return utils::pickMoreReadable((*((char**)this->name)), this->name);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { // If segfault, then it's not a pointer
                return this->name;
            }
        }
    };
    class GameDataManager {

    };
    static char empString[] = "";
    class GameData {
        char padding1[0x10];
        public:
        GameDataManager* manager;//Offset 0x10
        private:
        char padding2[0x10];
        public:
        char name[0x10];//Offset 0x28
        int nameLength;//Offset 0x38
        //char stringID;//Offset 0x60
        __declspec(nothrow) const char* getName() const {
            __try {
                if (nameLength > 15)return (*((char**)this->name));
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            __try {
                return this->name;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { 
                return empString;
            }
        }
    };
    class Item {//0x18 string 0x28 string length
        char padding1[0xDC];
    public:
        int x;// Offset 0xDC readonly
        int y;// Offset 0xE0 readonly
        char* inventorySectionName;// Offset EC
    private:
        char padding2[0x3C];
    public:
        int quantity;//Offset 0x12C
    private:
        char padding3[1];
    public:
        //Inventory* inventory;//Offset 0x290
    };
    class InventorySection {
        char padding1[8];
        char name[0x10];//Offset 0x8  belt hip armour head legs back backpack_content backpack_attach out in1      00747500
        int nameLength;//Offset 0x18
        Item** itemList;//readonly Offset 0x40

        //pointer to inv 0xC8
    };
    class ProgressContainer {
    public:
        float progress;//Offset 0x0
    private:
        char padding1[0x4 + 0x8];
    public:
        GameData* itemInfo;//Offset 0x10
        InventorySection* invSection;//Offset 0x18
    };
    class medicalSystem {

    };
    struct platoon;
    class handleList;
    class CharacterHuman;
    struct activePlatoon {
        char padding0[0x58];
        bool skipSpawningCheck2;//Offset 0x58
        char padding1[0x78-0x59];
        platoon* squad;//Offset 0x78
        handleList* list;//Offset 0x80
        char padding2[0xA0-0x88];
        CharacterHuman* leader;//Offset 0xA0
        //float x;//Offset 0xB4
        //float y;//Offset 0xB8
        //float z;//Offset 0xBC
        //void* plrInterface;//Offset 0xE8
        char padding3[0xF0 - 0xA8];
        bool skipSpawningCheck1;//Offset 0xF0
        char padding4[0x250 - 0xF1];
        void* skipSpawningCheck3;//Offset 0x250
    };
    class handleList {
        char padding1[0x8];
        class list {
            char padding1[0x8];
            void* data[256];//Offset 0x8
        };
    };
    class Faction;
    struct platoon {
        char padding1[0x10];
        Faction* faction;//Offset 0x10
        char padding2[0x1D8-0x18];
        activePlatoon* active;//Offset 0x1D8
    };
    class charStats {
        char padding1[0x8];//vtable 
        medicalSystem* medical;//Offset 0x8
    };
    class Faction {
        char padding1[0x1A8];
        char name[0x10];        // Offset 0x1A8
        int nameLength;// Offset 0x1B8
        __declspec(nothrow) const char* getName() const {
            __try {
                if (nameLength > 15)return (*((char**)this->name));
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            __try {
                return this->name;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return empString;
            }
        }
    };
    class CharacterHuman {
        char padding1[0x10];
        public:
        Faction* faction;
        private:
        char name[8];        // Offset 0x18
        char padding2[0x48 -(0x18+8)];
        public:
        structs::Vector3Raw pos; // Offset 0x48   readonly! dont use this
        char padding21[0x2E8 - (0x48 + 16)];
        Inventory* inventory;// Offset  0x2E8
        private:
        char padding3[0x450-(0x2E8+8)];
        public:
        charStats* stats;// Offset  0x450
        __declspec(nothrow) const char* getName() const {
            __try {
                return utils::pickMoreReadable((*((char**)this->name)), this->name);
            }__except (EXCEPTION_EXECUTE_HANDLER) {
                return this->name;
            }
        }
    };
    class CharMovement {
        char padding1[0x320];
        public:
        Vector3* position; // Offset 0x320
    };
    class AnimationClassHuman {
        char padding1[0xC0];
        public:
        CharMovement* movement;       // Offset 0xC0
        private:
        char padding2[0x2D8 - 0xC0 - sizeof(CharMovement*)];
        public:
        CharacterHuman* character;    // Offset 0x2D8
    };
    class Inventory {
        char padding1[0x18];
        public:
        int numberOfItems;         //Offset 0x18 readonly
        private:
        char padding2[0x4];
        public:
        Item** itemList;           //Offset 0x20 readonly
        private:
        char padding3[0x80-0x28];
        public:
        CharacterHuman* character; // Offset 0x80
    };
    struct PreviewBuilding {

    };
    struct lektorPreviewBuilding {
        int size; //Offset 0xC
        int padding;
        PreviewBuilding** builds;//Offset 0x10
        PreviewBuilding* mouseBuild;//Offset 0x18
    };
}
namespace customStructs {
    struct Char {
        structs::GameData* data = 0;// Offset 0x0
        structs::Vector3Raw pos;// Offset 0x8
        uintptr_t moduleBase;// Offset 0x18
        Char() {} //dont use this one
        Char(structs::GameData* d, structs::Vector3Raw p) : data(d) {
            pos.x = p.x;
            pos.y = p.y;
            pos.z = p.z;
        }
    };
}