#include "pch.h"
#include "structs.h"
#include "gameState.h"
#include "offsets.h"
#include "func.h"
#pragma optimize("", off)//important
namespace func {
	structs::Item* spawnItem(structs::GameData* itemInfo) {
		typedef structs::Item* (__fastcall *spawnItemSign)(
			void* itemSpawningMagic,    // rcx
			structs::GameData* itemInfo,// rdx
			void* itemSpawningHand,     // r8
			void* unused_r9,            // r9
			void* unused_stack1,        // [rsp+20]
			void* unused_stack2,        // [rsp+28]
			void* unused_stack3         // [rsp+30]
			);
		spawnItemSign function = (spawnItemSign)(gameState::moduleBase + offsets::spawnItemFunc);
		return function(
			*((void**)(gameState::moduleBase + offsets::itemSpawningMagic)),//rcx
			(itemInfo),//rdx
			(void*)(gameState::moduleBase + offsets::itemSpawningHand),//r8
			0,//r9
			0,//[rsp+20]
			0,//[rsp+28]
			0//[rsp+30]
		);
	}
	structs::InventorySection* getInvSection(structs::CharacterHuman* npc, structs::kenshiString* sectionNameRaw) {
		typedef structs::InventorySection* (__fastcall* funcSign)(
			structs::Inventory* inv,    // rcx
			structs::kenshiString* secName// rdx
			);
		//std::cout << (npc->inventory)<<" "<< (sectionNameRaw)<<"\n";
		funcSign function = (funcSign)(gameState::moduleBase + offsets::getSectionFromInvByName);
		return function(
			(npc->inventory),//rcx
			(sectionNameRaw)//rdx
		);
	}
	void* call(bool confim,long long where, long long arg1, long long arg2, long long arg3, long long arg4, 
		long long arg5, long long arg6, long long arg7) {
		typedef void* (__fastcall* funcSign)(
			long long arg1,    // rcx
			long long arg2,    // rdx
			long long arg3,    // r8
			long long arg4,    // r9
			long long arg5,    // [rsp+20]
			long long arg6,    // [rsp+28]
			long long arg7     // [rsp+30]
		);
		
		if(confim){
			std::cout << "Calling function at: " << std::hex << where << "\n";
			std::cout << "Args: "
				<< std::hex << arg1 << " " << arg2 << " " << arg3 << " " << arg4 << " "
				<< arg5 << " " << arg6 << " " << arg7 << "\n";
			std::cout << "Confirm? y/n:\n";
			std::string res;std::getline(std::cin, res);
			if (res != "y") { std::cout << "Operation canceled\n";return 0; }
		}

		funcSign function = (funcSign)(where);
		return function(arg1, arg2,	arg3, arg4, arg5, arg6,	arg7
		);
	}
}