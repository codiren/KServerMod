#include"pch.h"
#include<iostream>
#include <sstream>
#include <map>
#include <algorithm>
#include <vector>
#include "gameState.h"
#include "commands.h"
#include "func.h"
namespace commands {
    using argsT = std::vector<std::string>;
    std::map<std::string, void(*)(std::vector<std::string>&)> commands;
    void commandsLoop() {
        init();
        bool running = true;
        while (running) {
            std::string commandLine;
            std::getline(std::cin, commandLine);
            std::cout << "> ";
            std::stringstream ss(commandLine);
            std::string command;
            ss >> command;

            argsT arguments;
            std::string argument;
            while (ss >> argument) {  // Get all arguments
                arguments.push_back(argument);
            }

            // Find the command in the map and call the associated function
            if (commands.find(command) != commands.end()) {
                commands[command](arguments);
            }
            else {
                std::cout << "Unknown command: \"" << command << "\". Type \"help\" to see a list of commands.\n";
            }
        }
    }
    void help(argsT& args) {
        if (args.size() > 0 && args[0] == "info") {
            std::cout << "help - brings up this list \n";return;
        }
        std::cout << "List of Commands: \n";
        std::vector<std::string> infoLabel;infoLabel.push_back("info");
        for (auto& i : commands)i.second(infoLabel);
    }
    void chars(argsT& args) {
        if (args.size() > 0 && args[0] == "info") {
            std::cout << "chars [name] - gets a list of all characters \n";return;
        }
        if (gameState::chars.size() == 0) { std::cout << "No characters found \n";return; }
        long long now = GetTickCount64();
        std::cout << "List of characters: \n";
        
        std::vector<std::pair<structs::AnimationClassHuman*, std::pair<std::string, long long>>> filteredChars;

        // Check if the args size is greater than 0 and args[0] is not "info"
        if (args.size() > 0 && args[0] != "info") {
            // Filter the elements based on whether the name starts with args[0]
            for (const auto& i : gameState::chars) {
                if (i.second.first.find(args[0]) == 0) { // check if name starts with args[0]
                    filteredChars.push_back(i);
                }
            }
        }
        else {
            // If no filtering is applied, copy all elements into the vector
            filteredChars.assign(gameState::chars.begin(), gameState::chars.end());
        }

        // Sort the filtered elements by timestamp in decreasing order
        std::sort(filteredChars.begin(), filteredChars.end(), [](const auto& a, const auto& b) {
            return a.second.second > b.second.second;  // Compare timestamps for decreasing order
            });

        // Now print the sorted and filtered result
        for (const auto& i : filteredChars) {
            std::cout << "Addr: " << i.first << " Name: " << i.second.first
                << ", Last seen: " << (now - i.second.second) / 1000.0 << "s.\n";
        }
    }
    void searchDB(argsT& args) {
        if (args.size() > 0 && args[0] == "info") {
            std::cout << "db <name> - search FCS database \n";
            return;
        }
        if (gameState::DB.empty()) {
            std::cout << "No entries found, how did that happen?! \n";
            return;
        }

        long long now = GetTickCount64();
        std::cout << "List of gameData: \n";

        std::string query;
        for (const auto& arg : args) {
            if (!query.empty()) query += " "; // Add space between words
            query += arg;
        }

        std::vector<std::pair<std::string, structs::GameData*>> filteredDB;

        // Check if filtering is needed
        if (args.size() > 0 && args[0] != "info") {
            for (const auto& i : gameState::DB) {
                if (i.first.find(query) == 0) { // Check if name starts with args[0]
                    filteredDB.push_back(i);
                }
            }
        }
        else {
            // If no filtering is applied, copy all elements
            filteredDB.assign(gameState::DB.begin(), gameState::DB.end());
        }

        // Sorting is removed since DB is not timestamp-based

        // Print the filtered results
        std::ostringstream printStream;
        for (const auto& i : filteredDB) {
            printStream << "Name: " << i.first << ", Adr: " << i.second << "\n";
        }
        std::cout << printStream.str();
    }

    void builds(argsT& args) {
        if (args.size() > 0 && args[0] == "info") {
            std::cout << "builds [name] - gets a list of all buildings \n";return;
        }
        if (gameState::builds.size() == 0) { std::cout << "No buildings found \n";return; }
        long long now = GetTickCount64();
        std::cout << "List of buildings: \n";

        std::vector<std::pair<structs::Building*, std::pair<std::string, long long>>> filteredChars;

        // Check if the args size is greater than 0 and args[0] is not "info"
        if (args.size() > 0 && args[0] != "info") {
            // Filter the elements based on whether the name starts with args[0]
            for (const auto& i : gameState::builds) {
                if (i.second.first.find(args[0]) == 0) { // check if name starts with args[0]
                    filteredChars.push_back(i);
                }
            }
        }
        else {
            // If no filtering is applied, copy all elements into the vector
            filteredChars.assign(gameState::builds.begin(), gameState::builds.end());
        }

        // Sort the filtered elements by timestamp in decreasing order
        std::sort(filteredChars.begin(), filteredChars.end(), [](const auto& a, const auto& b) {
            return a.second.second > b.second.second;  // Compare timestamps for decreasing order
            });

        // Now print the sorted and filtered result
        for (const auto& i : filteredChars) {
            std::cout << "Addr: " << i.first << " Name: " << i.second.first
                << ", Last seen: " << (now - i.second.second) / 1000.0 << "s.\n";
        }
    }
    void clear(argsT& args) {
        if (args.size() > 0 && args[0] == "info") {
            std::cout << "clear - clears console \n";return;
        }
        std::system("cls");
    }
    void spawnFunc(argsT& args) {
        //std::cout << (&(func::getInvSection)) << "\n";
        if (args.empty()) return;
        if (args.size() > 0 && args[0] == "info") {
            std::cout << "func - test123 \n";
            return;
        }
        uintptr_t address;
        std::stringstream ss(args[0]);  // Directly use args[0] in stringstream
        ss >> std::hex >> address;

        structs::AnimationClassHuman* masterChar = reinterpret_cast<structs::AnimationClassHuman*>(address);
        if (args.size() < 2) return;


        structs::kenshiString* tempString = new structs::kenshiString(args[1]);
        structs::InventorySection* result = func::getInvSection(masterChar->character, tempString);
        std::cout << (result) << "\n";
        //delete tempString;
    }
    void call(argsT& args) {
        if (args.size() < 1) return;

        if (args[0] == "info") {
            std::cout << "call - call a function using __fastcall\n";
            return;
        }

        uintptr_t address = 0;
        std::stringstream ss(args[0]);
        ss >> std::hex >> address;

        if (address == 0) {
            std::cerr << "Invalid function address\n";
            return;
        }

        // Parse up to 8 arguments from args
        long long argVals[8] = { 0 }; // Default all to 0
        for (size_t i = 1; i < args.size() && i <= 8; ++i) {
            std::stringstream argStream(args[i]);
            argStream >> std::hex >> argVals[i - 1]; // Convert hex input to int
        }
        if (args[0][0] == '.')address += gameState::moduleBase;
        // Call function dynamically
        void* result = func::call(true,address, argVals[0], argVals[1], argVals[2], argVals[3],
            argVals[4], argVals[5], argVals[6]);

        std::cout << "Result: " << result << "\n";
    }
    void heapScan(argsT& args) {//todo replace scan with hook
        if (args.size() > 0 && args[0] == "info") {
            std::cout << "heapScan - scans heap for data objects\n";
            return;
        }
        gameState::scanHeap();
    }
    void init() {
        commands["help"] = help;
        commands["chars"] = chars;
        commands["builds"] = builds;
        commands["clear"] = clear;
        commands["getSection"] = spawnFunc;
        commands["call"] = call;
        commands["db"] = searchDB;
        commands["heapScan"] = heapScan;
    }
}