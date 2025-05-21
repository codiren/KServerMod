#include "pch.h"
#include "utils.h"
#include "offsets.h"
#include "gameState.h"
#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>

namespace utils {
    void spawn_console() {
        AllocConsole();
		SetConsoleTitle(L"Multiplayer Mod Console");
        FILE* stream;
        freopen_s(&stream, "CONOUT$", "w", stdout);
        freopen_s(&stream, "CONIN$", "r", stdin);
    }
	std::vector<std::string> split(const std::string& str, const std::string& delim) {
		std::vector<std::string> rez;
		size_t wher = 0;
		size_t prev = 0;
		while (wher != -1) {
			wher = str.find(delim, wher + 1);
			rez.push_back(str.substr(prev, wher - prev));
			prev = wher + 1;
		}
		return rez;
	}
	__declspec(nothrow) bool isValidName(const char* name) {
		__try {
			if (!name || strlen(name) < 2) return false; // Must be at least 2 characters long
			for (size_t i = 0; name[i] != '\0'; ++i) {
				if (!std::isalnum(name[i]) && name[i] != ' ' && name[i] != '(' && name[i] != ')' && name[i] != '_') {
					return false; // Found weird characters, so it's probably not a valid name
				}
			}
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}
	const char* pickMoreReadable(const char* derefName, const char* thisName) {
		bool valid1 = isValidName(derefName);
		bool valid2 = isValidName(thisName);

		if (valid1 && !valid2) { return derefName; }
		if (!valid1 && valid2) { return thisName;}

		if (!valid1 && !valid2) {
			std::cout <<"bad Names: " << &derefName << " " << &thisName << "\n";
			return thisName;
		}
		return (strlen(derefName) >= strlen(thisName)) ? derefName : thisName;
	}
	void pause() { int t;std::cin >> t; }
	void nop() {  }
	void log(std::string data) {
		std::ofstream file("C:\\Users\\sam\\Desktop\\logs.txt", std::ios::trunc); // Open in truncate mode (clears file)
		if (file.is_open()) {
			file << data;  // Write data to the file
			file.close();  // Close the file
		}
	}
	namespace asmb {
		std::vector<uint8_t> jmp = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
		std::vector<uint8_t> mov = { 0x48, 0x89, 0x1C, 0x25 };
		std::vector<uint8_t> ret = { 0xC3 };
		std::vector<uint8_t> nop = { 0x90 };
		std::vector<uint8_t> call = { 0xFF, 0x15, 0x02, 0x00, 0x00, 0x00, 0xEB, 0x08 };
		std::vector<uint8_t> saveRegisters = { 0x50,0x51,0x52,0x53,0x55,0x56,0x57,0x41,0x50,
			0x41,0x51,0x41,0x52,0x41,0x53,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57 };
		std::vector<uint8_t> restoreRegisters = { 0x41,0x5F,0x41,0x5E,0x41,0x5D,0x41,0x5C,0x41,
			0x5B,0x41,0x5A,0x41,0x59,0x41,0x58,0x5F,0x5E,0x5D,0x5B,0x5A,0x59,0x58 };
	}
	void disableHook(long long hookLocation, const std::vector<uint8_t>& oldCode) {
		DWORD oldProtect;
		VirtualProtect(reinterpret_cast<void*>(hookLocation), oldCode.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
		std::memcpy(reinterpret_cast<void*>(hookLocation), oldCode.data(), oldCode.size());
		VirtualProtect(reinterpret_cast<void*>(hookLocation), oldCode.size(), oldProtect, &oldProtect);
	}
	void createHook(long long hookLocation, const std::vector<uint8_t>& oldCode, 
		const std::vector<uint8_t>& befMyFunc, void* myFunc, const std::vector<uint8_t>& aftMyFunc) {
		size_t minSize = (asmb::jmp.size() + sizeof(void*)); //14
		if (oldCode.size() < minSize) {
			std::cerr << "createHook: oldCode size must be at least: " << minSize;
			std::cin >> minSize;//pause execution
			return;
		}
		long long returnLocation = hookLocation + oldCode.size();
		size_t size = befMyFunc.size() + asmb::call.size() + sizeof(void*) + aftMyFunc.size() + oldCode.size() + asmb::jmp.size() + sizeof(void*);
		char* data = reinterpret_cast<char*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		if (data == 0)return;//todo: do something maybe
		size_t ofst = 0;
		std::memcpy(reinterpret_cast<void*>(data + ofst), befMyFunc.data(), befMyFunc.size());ofst += befMyFunc.size();
		std::memcpy(reinterpret_cast<void*>(data + ofst), asmb::call.data(), asmb::call.size());ofst += asmb::call.size();
		std::memcpy(reinterpret_cast<void*>(data + ofst), &myFunc, sizeof(void*));ofst += sizeof(void*);
		std::memcpy(reinterpret_cast<void*>(data + ofst), aftMyFunc.data(), aftMyFunc.size());ofst += aftMyFunc.size();
		//if(executeOldCode)
		std::memcpy(reinterpret_cast<void*>(data + ofst), oldCode.data(), oldCode.size());ofst += oldCode.size();
		std::memcpy(reinterpret_cast<void*>(data + ofst), asmb::jmp.data(), asmb::jmp.size());ofst += asmb::jmp.size();
		std::memcpy(reinterpret_cast<void*>(data + ofst), &returnLocation, sizeof(returnLocation));ofst += sizeof(returnLocation);
		DWORD oldProtect;
		VirtualProtect(reinterpret_cast<void*>(hookLocation), oldCode.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
		std::memcpy(reinterpret_cast<void*>(hookLocation), asmb::jmp.data(), asmb::jmp.size());
		std::memcpy(reinterpret_cast<void*>(hookLocation + asmb::jmp.size()), &data, sizeof(void*));
		std::memset(reinterpret_cast<void*>(hookLocation + minSize), asmb::nop[0], oldCode.size() - minSize);
		VirtualProtect(reinterpret_cast<void*>(hookLocation), oldCode.size(), oldProtect, &oldProtect);
	}



	std::vector<uintptr_t> scanMemoryForValue(uint64_t targetValue) {
		std::vector<uintptr_t> results;
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);

		uintptr_t startAddr = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
		uintptr_t endAddr = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

		MEMORY_BASIC_INFORMATION memInfo;
		while (startAddr < endAddr) {
			if (VirtualQuery(reinterpret_cast<LPCVOID>(startAddr), &memInfo, sizeof(memInfo))) {
				// Check if the region is committed and readable
				if (memInfo.State == MEM_COMMIT &&
					(memInfo.Protect & (PAGE_READONLY | PAGE_READWRITE |
						PAGE_WRITECOPY | PAGE_EXECUTE_READ |
						PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) &&
					!(memInfo.Protect & PAGE_GUARD)) {

					uintptr_t regionStart = reinterpret_cast<uintptr_t>(memInfo.BaseAddress);
					uintptr_t regionEnd = regionStart + memInfo.RegionSize;

					// Align the start address to 8 bytes
					uintptr_t alignedStart = (regionStart + 7) & ~7ULL; // Faster alignment
					if (alignedStart >= regionEnd) {
						startAddr = regionEnd;
						continue;
					}

					// Calculate end of aligned addresses
					uintptr_t alignedEnd = regionEnd - 8;
					if (alignedEnd < alignedStart) continue;

					// Process in chunks for efficiency
					uint64_t* ptr = reinterpret_cast<uint64_t*>(alignedStart);
					size_t count = (alignedEnd - alignedStart) / 8 + 1;

					for (size_t i = 0; i < count; ++i) {
						if (ptr[i] == targetValue) {
							results.push_back(alignedStart + i * 8);
						}
					}
				}
				startAddr += memInfo.RegionSize;
			}
			else {
				break; // Exit on error
			}
		}
		return results;
	}
}
