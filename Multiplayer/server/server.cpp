#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <algorithm>
#include <d3d11.h>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <tchar.h>
#include <vector>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "../.struct.h"
#include "../tasks.h"

// --- Data Structures ---
struct ItemData {
  std::string name;
  std::string type;
  std::string details;
};

struct CharacterData {
  std::string name;
  std::string id;
  std::vector<ItemData> items;
};

struct SquadData {
  std::string name;
  std::string id;
  std::vector<CharacterData> characters;
};

#include ".commands.h"
#include "network_internal.h"
#include "registry.h"
#include "shared.h"

struct NetCommand {
  NetHandler func;
  std::string templateStr;
  std::string prefix;
  std::vector<std::string> paramNames;
};

std::map<std::string, NetCommand> g_net_handlers;

void net_reg(const std::string &pattern, NetHandler handler) {
  NetCommand cmd;
  cmd.func = handler;
  cmd.templateStr = pattern;

  std::stringstream ss(pattern);
  std::string segment;
  std::vector<std::string> parts;
  while (ss >> segment)
    parts.push_back(segment);

  if (parts.empty())
    return;

  cmd.prefix = parts[0];

  for (size_t i = 1; i < parts.size(); ++i) {
    std::string p = parts[i];
    if (p.size() >= 2 && p[0] == '<' && p[p.size() - 1] == '>') {
      cmd.paramNames.push_back(p.substr(1, p.size() - 2));
    }
  }

  g_net_handlers[cmd.prefix] = cmd;
}

// Include .network.h for the 'port' variable
#include ".network.h"

// ImGui includes
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// --- Globals ---
// Removed g_port, using 'port' from .network.h instead
void LoadConfig(); // Forward decl

std::string g_prefix = "/";

int port = 25565;
std::vector<SOCKET> g_clients;
std::map<SOCKET, PlayerData> g_playerData;
std::recursive_mutex g_clientsMutex;
bool g_running = true;

std::vector<std::string> g_consoleLogs;
char g_consoleInput[256] = "";
std::mutex g_consoleMutex;

int targetFPS = 30;
float g_masterSpeed = 1.0f;
int g_masterSpeedVersion = 0;
std::string g_lastSpeedUpdater = "None";

// History
std::vector<std::string> g_history;
int g_historyPos = -1;

// Faction Management
const int MAX_FACTIONS = 11;
bool g_factionInUse[MAX_FACTIONS + 1] = {false};
std::map<SOCKET, int> g_clientFactionIndex;

SOCKET g_selectedClient = INVALID_SOCKET;
int g_selectedSquadIdx = -1;
int g_selectedCharIdx = -1;
int g_selectedItemIdx = -1;

// DX11 Globals
static ID3D11Device *g_pd3dDevice = NULL;
static ID3D11DeviceContext *g_pd3dDeviceContext = NULL;
static IDXGISwapChain *g_pSwapChain = NULL;
static ID3D11RenderTargetView *g_mainRenderTargetView = NULL;

// Helper Implementation

void log(const std::string &msg) {
  std::lock_guard<std::mutex> lock(g_consoleMutex);
  g_consoleLogs.push_back(msg);
  if (g_consoleLogs.size() > 500)
    g_consoleLogs.erase(g_consoleLogs.begin());
}

void Broadcast(const std::string &msg, SOCKET sender) {
  std::lock_guard<std::recursive_mutex> lock(g_clientsMutex);
  std::string fullMsg = msg + "\n";
  for (size_t i = 0; i < g_clients.size(); ++i) {
    if (g_clients[i] != sender) {
      send(g_clients[i], fullMsg.c_str(), (int)fullMsg.length(), 0);
    }
  }
}

void sendAllClients(const std::string &type, const std::string &msg,
                    SOCKET exclude) {
  Broadcast(type + " " + msg, exclude);
}

void ParseData(SOCKET client, const std::string &raw) {
  // Parser...
}

std::string Trim(const std::string &s) {
  std::string r = s;
  r.erase(std::remove(r.begin(), r.end(), '\r'), r.end());
  r.erase(std::remove(r.begin(), r.end(), '\n'), r.end());
  return r;
}

// History logic
void SaveHistory() {
  std::ofstream file("history.txt");
  for (const auto &h : g_history) {
    file << h << std::endl;
  }
}

void LoadHistory() {
  std::ifstream file("history.txt");
  if (!file.is_open())
    return;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty())
      g_history.push_back(line);
  }
}

// Networking Logic
DWORD WINAPI ClientHandler(LPVOID param) {
  SOCKET client = (SOCKET)param;
  char buffer[4096];
  std::string msgBuffer;

  std::string msgToBroadcast;

  // Assign Faction on connect
  {
    std::lock_guard<std::recursive_mutex> lock(g_clientsMutex);
    int assignedIndex = -1;
    for (int i = 1; i <= MAX_FACTIONS; ++i) {
      if (!g_factionInUse[i]) {
        assignedIndex = i;
        break;
      }
    }

    std::string factionId;
    if (assignedIndex != -1) {
      g_factionInUse[assignedIndex] = true;
      g_clientFactionIndex[client] = assignedIndex;
      factionId = std::to_string(assignedIndex) + "-Multiplayer.mod";
    } else {
      factionId = "SPECTATOR-Multiplayer.mod";
    }

    g_playerData[client].name = factionId;
    g_playerData[client].factionId = factionId;

    std::string facMsg = "SET_FACTION:" + factionId + "\n";
    send(client, facMsg.c_str(), (int)facMsg.length(), 0);

    std::string connectMsg = "[SYSTEM]: " + factionId + " connected.";
    log(connectMsg);
    // Store message to broadcast after unlocking
    msgToBroadcast = connectMsg;
  } // Lock released here

  if (!msgToBroadcast.empty()) {
    Broadcast(msgToBroadcast, client);
  }

  while (g_running) {
    int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
      break;
    buffer[bytes] = '\0';
    msgBuffer += buffer;

    size_t pos;
    while ((pos = msgBuffer.find('\n')) != std::string::npos) {
      std::string rawLine = msgBuffer.substr(0, pos);
      msgBuffer.erase(0, pos + 1);
      std::string line = Trim(rawLine);
      if (line.empty())
        continue;

      bool handled = false;
      for (const auto &kv : g_net_handlers) {
        if (line.find(kv.second.prefix) == 0) {
          std::string payload = line.substr(kv.second.prefix.length());
          if (payload.length() > 0 && payload[0] == ' ')
            payload.erase(0, 1);
          NetArgs args;

          args.client.id = client;
          {
            std::lock_guard<std::recursive_mutex> lock(g_clientsMutex);
            if (g_playerData.count(client)) {
              args.client.name = g_playerData[client].name;
            } else {
              args.client.name = "Unknown";
            }
          }

          if (!kv.second.paramNames.empty()) {
            std::stringstream pss(payload);
            std::string p;
            size_t pIdx = 0;
            while (pss >> p && pIdx < kv.second.paramNames.size()) {
              if (pIdx == kv.second.paramNames.size() - 1) {
                // Last parameter gets the rest of the payload
                std::string rest;
                std::getline(pss, rest);
                args.data[kv.second.paramNames[pIdx]] = p + rest;
                break;
              }
              args.data[kv.second.paramNames[pIdx]] = p;
              pIdx++;
            }
          }
          kv.second.func(args);
          handled = true;
          break;
        }
      }

      if (!handled) {
        if (line == "CONNECT") {
          // Handshake ignored
        } else if (!line.empty() && line[0] == g_prefix[0]) {
          run_cmd(line);
        } else {
          // Chat or unhandled - don't spam console with syncing data
          // log(line);
        }
      }
      // Also ParseData just in case it contains game info later
      ParseData(client, line);
    }
  }

  std::string n = "Unknown";
  {
    std::lock_guard<std::recursive_mutex> lock(g_clientsMutex);
    if (g_playerData.count(client))
      n = g_playerData[client].name;
    log("Client disconnected: " + n);
    g_playerData.erase(client);
    for (size_t i = 0; i < g_clients.size(); ++i) {
      if (g_clients[i] == client) {
        g_clients.erase(g_clients.begin() + i);
        break;
      }
    }
    if (g_selectedClient == client)
      g_selectedClient = INVALID_SOCKET;

    if (g_clientFactionIndex.count(client)) {
      int idx = g_clientFactionIndex[client];
      if (idx >= 0 && idx <= MAX_FACTIONS) {
        g_factionInUse[idx] = false;
      }
      g_clientFactionIndex.erase(client);
    }
  } // Lock released here

  // Broadcast disconnect
  if (n != "Unknown") {
    std::string discMsg = "[SYSTEM]: " + n + " disconnected.";
    Broadcast(discMsg, INVALID_SOCKET);
  }
  closesocket(client);
  return 0;
}

DWORD WINAPI ServerThread(LPVOID param) {
  SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port); // Using 'port' from .network.h

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
    log("Bind failed!");
    return 1;
  }
  listen(server_fd, 10);

  log("Server listening on port " + std::to_string(port) + "...");

  while (g_running) {
    SOCKET client = accept(server_fd, NULL, NULL);
    if (client != INVALID_SOCKET) {
      {
        std::lock_guard<std::recursive_mutex> lock(g_clientsMutex);
        g_clients.push_back(client);
        PlayerData pd;
        pd.name = "Connecting...";
        g_playerData[client] = pd;
      }
      CreateThread(NULL, 0, ClientHandler, (LPVOID)client, 0, NULL);
    }
  }
  return 0;
}

int ConsoleCallback(ImGuiInputTextCallbackData *data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
    int prev_history_pos = g_historyPos;
    if (data->EventKey == ImGuiKey_UpArrow) {
      if (g_historyPos == -1)
        g_historyPos = (int)g_history.size() - 1;
      else if (g_historyPos > 0)
        g_historyPos--;
    } else if (data->EventKey == ImGuiKey_DownArrow) {
      if (g_historyPos != -1) {
        if (++g_historyPos >= (int)g_history.size())
          g_historyPos = -1;
      }
    }

    if (prev_history_pos != g_historyPos) {
      const char *history_str =
          (g_historyPos >= 0) ? g_history[g_historyPos].c_str() : "";
      data->DeleteChars(0, data->BufTextLen);
      data->InsertChars(0, history_str);
    }
  }
  return 0;
}

// UI Rendering
void RenderUI() {
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
  ImGui::Begin("Server", NULL,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove);

  float width = ImGui::GetContentRegionAvail().x;
  float height = ImGui::GetContentRegionAvail().y;

  // Console
  ImGui::BeginChild("Console", ImVec2(width * 0.61f, height), true);
  ImGui::Text("Server Log");
  ImGui::Separator();
  ImGui::BeginChild("Logs", ImVec2(0, -32));
  {
    std::lock_guard<std::mutex> lock(g_consoleMutex);
    for (const auto &l : g_consoleLogs) {
      ImGui::TextUnformatted(l.c_str());
    }
  }
  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();

  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::InputText("##In", g_consoleInput, 256,
                       ImGuiInputTextFlags_EnterReturnsTrue |
                           ImGuiInputTextFlags_CallbackHistory,
                       ConsoleCallback)) {
    std::string cmd = g_consoleInput;
    if (cmd.empty()) {
      run_cmd(g_prefix + "help");
    } else {
      if (!g_history.empty() && g_history.back() == cmd) {
        // Already there
      } else {
        g_history.push_back(cmd);
        SaveHistory();
      }
      g_historyPos = -1;

      if (cmd[0] == g_prefix[0])
        run_cmd(cmd);
      else {
        log("[SERVER]: " + cmd);
        Broadcast("[Server]: " + cmd, INVALID_SOCKET);
      }
    }
    memset(g_consoleInput, 0, 256);
    ImGui::SetKeyboardFocusHere(-1);
  }
  ImGui::EndChild();

  ImGui::SameLine();

  // Right Panel: 4 Lists
  ImGui::BeginChild("Lists", ImVec2(0, height), true);

  ImGui::Text("Global Server State");
  ImGui::Separator();
  ImGui::Text("Master Speed: %.2f", g_masterSpeed);
  ImGui::Text("Updated By: %s", g_lastSpeedUpdater.c_str());
  ImGui::Separator();

  float lH = (height - ImGui::GetCursorPosY() - 20) * 0.25f;

  std::lock_guard<std::recursive_mutex> l(g_clientsMutex);
  PlayerData *p = (g_selectedClient != INVALID_SOCKET &&
                   g_playerData.count(g_selectedClient))
                      ? &g_playerData[g_selectedClient]
                      : nullptr;

  // 1. Players
  ImGui::Text("Players");
  ImGui::Columns(2, "P", true);
  if (ImGui::BeginListBox("##PL", ImVec2(-1, lH))) {
    // Auto-select first player if none selected and players exist
    if (g_selectedClient == INVALID_SOCKET && !g_playerData.empty()) {
      g_selectedClient = g_playerData.begin()->first;
      g_selectedSquadIdx = g_selectedCharIdx = g_selectedItemIdx = -1;
    }
    for (auto it = g_playerData.begin(); it != g_playerData.end(); ++it) {
      if (ImGui::Selectable(it->second.name.c_str(),
                            g_selectedClient == it->first)) {
        g_selectedClient = it->first;
        g_selectedSquadIdx = g_selectedCharIdx = g_selectedItemIdx = -1;
      }
    }
    ImGui::EndListBox();
  }
  ImGui::NextColumn();
  if (ImGui::BeginListBox("##PD", ImVec2(-1, lH))) {
    if (p) {
      ImGui::Text("Name: %s", p->name.c_str());
      ImGui::Text("Faction: %s", p->factionId.c_str());
      if (p->worldState) {
        int squadCount = 0;
        for (std::map<std::string, s::faction *>::iterator fit =
                 p->worldState->factions.begin();
             fit != p->worldState->factions.end(); ++fit) {
          squadCount += (int)fit->second->squads.size();
        }
        ImGui::Text("Squads: %d", squadCount);
      }
    }
    ImGui::EndListBox();
  }
  ImGui::Columns(1);

  // 2. Squads
  ImGui::Text("Squads");
  ImGui::Columns(2, "S", true);
  if (ImGui::BeginListBox("##SL", ImVec2(-1, lH))) {
    if (p && p->worldState) {
      // Count total squads and auto-select first if none selected
      int totalSquads = 0;
      for (std::map<std::string, s::faction *>::iterator fit =
               p->worldState->factions.begin();
           fit != p->worldState->factions.end(); ++fit) {
        totalSquads += (int)fit->second->squads.size();
      }
      if (g_selectedSquadIdx == -1 && totalSquads > 0) {
        g_selectedSquadIdx = 0;
        g_selectedCharIdx = g_selectedItemIdx = -1;
      }

      int idx = 0;
      for (std::map<std::string, s::faction *>::iterator fit =
               p->worldState->factions.begin();
           fit != p->worldState->factions.end(); ++fit) {
        for (std::map<long long, s::squad *>::iterator sit =
                 fit->second->squads.begin();
             sit != fit->second->squads.end(); ++sit) {
          std::string label = "Squad " + std::to_string(sit->first);
          if (ImGui::Selectable(label.c_str(), g_selectedSquadIdx == idx)) {
            g_selectedSquadIdx = idx;
            g_selectedCharIdx = g_selectedItemIdx = -1;
          }
          idx++;
        }
      }
    }
    ImGui::EndListBox();
  }
  ImGui::NextColumn();
  if (ImGui::BeginListBox("##SD", ImVec2(-1, lH))) {
    if (p && p->worldState && g_selectedSquadIdx >= 0) {
      int idx = 0;
      s::squad *selectedSquad = NULL;
      for (std::map<std::string, s::faction *>::iterator fit =
               p->worldState->factions.begin();
           fit != p->worldState->factions.end(); ++fit) {
        for (std::map<long long, s::squad *>::iterator sit =
                 fit->second->squads.begin();
             sit != fit->second->squads.end(); ++sit) {
          if (idx == g_selectedSquadIdx) {
            selectedSquad = sit->second;
            break;
          }
          idx++;
        }
        if (selectedSquad)
          break;
      }
      if (selectedSquad) {
        ImGui::Text("ID: %lld", selectedSquad->id);
        ImGui::Text("Chars: %d", (int)selectedSquad->characters.size());
      }
    }
    ImGui::EndListBox();
  }
  ImGui::Columns(1);

  // 3. Characters
  ImGui::Text("Characters");
  ImGui::Columns(2, "C", true);
  s::squad *s = NULL;
  if (p && p->worldState && g_selectedSquadIdx >= 0) {
    int idx = 0;
    for (std::map<std::string, s::faction *>::iterator fit =
             p->worldState->factions.begin();
         fit != p->worldState->factions.end(); ++fit) {
      for (std::map<long long, s::squad *>::iterator sit =
               fit->second->squads.begin();
           sit != fit->second->squads.end(); ++sit) {
        if (idx == g_selectedSquadIdx) {
          s = sit->second;
          break;
        }
        idx++;
      }
      if (s)
        break;
    }
  }
  if (ImGui::BeginListBox("##CL", ImVec2(-1, lH))) {
    if (s) {
      // Auto-select first character if none selected and characters exist
      if (g_selectedCharIdx == -1 && !s->characters.empty()) {
        g_selectedCharIdx = 0;
        g_selectedItemIdx = -1;
      }

      int idx = 0;
      for (std::map<long long, s::character *>::iterator cit =
               s->characters.begin();
           cit != s->characters.end(); ++cit) {
        std::string label =
            cit->second->name.empty()
                ? cit->second->sid + " (" + std::to_string(cit->first) + ")"
                : cit->second->name + " (" + std::to_string(cit->first) + ")";
        if (ImGui::Selectable(label.c_str(), g_selectedCharIdx == idx)) {
          g_selectedCharIdx = idx;
          g_selectedItemIdx = -1;
        }
        idx++;
      }
    }
    ImGui::EndListBox();
  }
  ImGui::NextColumn();
  if (ImGui::BeginListBox("##CD", ImVec2(-1, lH))) {
    if (s && g_selectedCharIdx >= 0) {
      s::character *selectedChar = NULL;
      int idx = 0;
      for (std::map<long long, s::character *>::iterator cit =
               s->characters.begin();
           cit != s->characters.end(); ++cit) {
        if (idx == g_selectedCharIdx) {
          selectedChar = cit->second;
          break;
        }
        idx++;
      }
      if (selectedChar) {
        ImGui::Text("Name: %s", selectedChar->name.c_str());
        ImGui::Text("SID: %s", selectedChar->sid.c_str());
        ImGui::Text("ID: %lld", selectedChar->id);
        ImGui::Text("Pos: %.1f, %.1f, %.1f", selectedChar->pos.x,
                    selectedChar->pos.y, selectedChar->pos.z);
        ImGui::Text("Rot: %.2f, %.2f, %.2f, %.2f", selectedChar->rot.w,
                    selectedChar->rot.x, selectedChar->rot.y,
                    selectedChar->rot.z);
        ImGui::Text("Task: %s", getTaskName(selectedChar->task).c_str());
        ImGui::Text("MoveTo: %.1f, %.1f, %.1f", selectedChar->moveTo.x,
                    selectedChar->moveTo.y, selectedChar->moveTo.z);
        ImGui::Text("Attack Target ID: %lld", selectedChar->targetID);
        ImGui::Text("Attackers Count: %d", (int)selectedChar->attackers.size());

        // Parse gender from appearance string if available
        if (!selectedChar->appearance.empty()) {
          size_t gPos = selectedChar->appearance.find("g::");
          if (gPos != std::string::npos) {
            // Find next '#' after gPos
            size_t hashPos = selectedChar->appearance.find("#", gPos);
            if (hashPos != std::string::npos) {
              char gender = selectedChar->appearance[hashPos - 1];
              ImGui::Text("foid: %c", gender);
            } else {
              ImGui::Text("foid: ?");
            }
          } else {
            ImGui::Text("foid: ?");
          }
        } else {
          ImGui::Text("foid: ?");
        }

        ImGui::Text("Items: %d", (int)selectedChar->items.size());
      }
    }
    ImGui::EndListBox();
  }
  ImGui::Columns(1);

  // 4. Items
  ImGui::Text("Items");
  ImGui::Columns(2, "I", true);
  s::character *c = NULL;
  if (s && g_selectedCharIdx >= 0) {
    int idx = 0;
    for (std::map<long long, s::character *>::iterator cit =
             s->characters.begin();
         cit != s->characters.end(); ++cit) {
      if (idx == g_selectedCharIdx) {
        c = cit->second;
        break;
      }
      idx++;
    }
  }
  if (ImGui::BeginListBox("##IL", ImVec2(-1, lH))) {
    if (c) {
      // Auto-select first item if none selected and items exist
      if (g_selectedItemIdx == -1 && !c->items.empty()) {
        g_selectedItemIdx = 0;
      }

      int idx = 0;
      for (std::map<long long, s::item *>::iterator iit = c->items.begin();
           iit != c->items.end(); ++iit) {
        std::string label =
            iit->second->sid + "##" + std::to_string(iit->first);
        if (ImGui::Selectable(label.c_str(), g_selectedItemIdx == idx)) {
          g_selectedItemIdx = idx;
        }
        idx++;
      }
    }
    ImGui::EndListBox();
  }
  ImGui::NextColumn();
  if (ImGui::BeginListBox("##ID", ImVec2(-1, lH))) {
    if (c && g_selectedItemIdx >= 0) {
      s::item *selectedItem = NULL;
      int idx = 0;
      for (std::map<long long, s::item *>::iterator iit = c->items.begin();
           iit != c->items.end(); ++iit) {
        if (idx == g_selectedItemIdx) {
          selectedItem = iit->second;
          break;
        }
        idx++;
      }
      if (selectedItem) {
        ImGui::Text("ID: %lld", selectedItem->id);
        ImGui::Text("Bag ID: %lld", selectedItem->bagid);
        ImGui::Text("SID: %s", selectedItem->sid.c_str());
        ImGui::Text("Qty: %d", selectedItem->quantity);
        ImGui::Text("Charges: %.1f", selectedItem->charges);
        ImGui::Text("Material: %s", selectedItem->material.c_str());
        ImGui::Text("Manufacturer: %s", selectedItem->manufacturer.c_str());
        ImGui::Text("Level: %d", selectedItem->level);
        ImGui::Text("Section: %s", selectedItem->section.c_str());
        ImGui::Text("Inv Pos: (%d, %d)", selectedItem->x, selectedItem->y);
      }
    }
    ImGui::EndListBox();
  }
  ImGui::Columns(1);

  ImGui::EndChild();
  ImGui::End();
}

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE h, HINSTANCE, LPSTR, int) {
  LoadHistory();

  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);

  WNDCLASSEX wc = {sizeof(WNDCLASSEX),    CS_CLASSDC, WndProc, 0L,   0L,
                   GetModuleHandle(NULL), NULL,       NULL,    NULL, NULL,
                   _T("KenshiSS"),        NULL};
  RegisterClassEx(&wc);
  HWND hwnd =
      CreateWindow(wc.lpszClassName, _T("Kenshi Server"), WS_OVERLAPPEDWINDOW,
                   100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

  if (!CreateDeviceD3D(hwnd))
    return 1;

  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

  InitCommands();
  InitNet();
  CreateThread(NULL, 0, ServerThread, NULL, 0, NULL);

  MSG msg;
  ZeroMemory(&msg, sizeof(msg));
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();
      RenderUI();
      ImGui::Render();
      const float clear_color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
      g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
      g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
                                                 clear_color);
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
      g_pSwapChain->Present(1, 0);
      Sleep(10); // Throttle a bit
    }
  }

  g_running = false;
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
  CleanupDeviceD3D();
  return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
                                    NULL, 0, D3D11_SDK_VERSION, &sd,
                                    &g_pSwapChain, &g_pd3dDevice, NULL,
                                    &g_pd3dDeviceContext) != S_OK)
    return false;
  CreateRenderTarget();
  return true;
}
void CreateRenderTarget() {
  ID3D11Texture2D *pBackBuffer;
  g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&pBackBuffer);
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL,
                                       &g_mainRenderTargetView);
  pBackBuffer->Release();
}
void CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = NULL;
  }
}
void CleanupDeviceD3D() {
  CleanupRenderTarget();
  if (g_pSwapChain)
    g_pSwapChain->Release();
  if (g_pd3dDeviceContext)
    g_pd3dDeviceContext->Release();
  if (g_pd3dDevice)
    g_pd3dDevice->Release();
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM,
                                                             LPARAM);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;
  if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}
