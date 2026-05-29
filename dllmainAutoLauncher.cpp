#include <algorithm>
#include <string>
#include <vector>
#include <windows.h>

// Helper to find the launcher window even if the title varies slightly
struct Param {
  std::vector<HWND> *list;
  const char *title;
};
BOOL CALLBACK EnumKenshiWindows(HWND hwnd, LPARAM lp) {
  Param *pp = (Param *)lp;
  char title[256];
  GetWindowTextA(hwnd, title, sizeof(title));
  if (strcmp(title, pp->title) == 0) {
    if (!GetDlgItem(hwnd, 1)) {
      pp->list->push_back(hwnd);
    }
  }
  return TRUE;
}

DWORD WINAPI AutoAcceptThread(LPVOID lpParam) {
  const char *targetTitle = "Kenshi 1.0.65 - x64 (Newland)";

  // 1. Initial Launcher Phase
  HWND launcherHwnd = NULL;
  for (int i = 0; i < 300; ++i) {
    launcherHwnd = FindWindowA(NULL, targetTitle);
    if (launcherHwnd) {
      // Confirm it's the launcher (has an OK button)
      HWND btnOk = GetDlgItem(launcherHwnd, 1);
      if (!btnOk)
        btnOk = GetDlgItem(launcherHwnd, 1001);

      if (btnOk) {
        SetForegroundWindow(launcherHwnd);
        SendMessage(launcherHwnd, WM_COMMAND,
                    MAKEWPARAM(GetDlgCtrlID(btnOk), BN_CLICKED), (LPARAM)btnOk);
        PostMessage(launcherHwnd, WM_KEYDOWN, VK_RETURN, 0);
        PostMessage(launcherHwnd, WM_KEYUP, VK_RETURN, 0);
        break;
      }
    }
    Sleep(100);
  }

  // 2. Transition Phase (Wait for launcher to close and game to open)
  Sleep(500);

  // 3. Escape Spam Phase
  // Re-find window handle as it changes between launcher and game
  for (int i = 0; i < 3; ++i) {
    HWND gameHwnd = FindWindowA(NULL, targetTitle);
    if (gameHwnd) {
      HWND btnOk = GetDlgItem(gameHwnd, 1);
      if (!btnOk) {
        SetForegroundWindow(gameHwnd);

        INPUT inputs[2] = {};
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_ESCAPE;
        inputs[0].ki.wScan = 0x01;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_ESCAPE;
        inputs[1].ki.wScan = 0x01;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(2, inputs, sizeof(INPUT));
      }
    }
    Sleep(500);
  }

  // 4. Load Save "test"
  // Wait for the engine to be fully ready for save loading
  Sleep(1000);

  uintptr_t moduleBase = (uintptr_t)GetModuleHandle(NULL);

  // SaveManager::getSingleton RVA: 0x37D7E0
  typedef void *(*GetSaveManager)();
  GetSaveManager getSM = (GetSaveManager)(moduleBase + 0x37D7E0);

  // SaveManager::load(std::string) RVA: 0x47AD00
  // Note: Kenshi uses standard MSVC std::string, which we must match
  typedef void(__thiscall * LoadSave)(void *, const std::string &);
  LoadSave loadFunc = (LoadSave)(moduleBase + 0x47AD00);

  for (int i = 0; i < 100; ++i) { // Try for 10 seconds
    void *sm = getSM();
    if (sm) {
      std::string saveName = "test";
      loadFunc(sm, saveName);
      break;
    }
    Sleep(100);
  }

  // 5. Position Windows
  // If multiple instances are open, tile them
  Sleep(1000);

  std::vector<HWND> kenshiWindows;
  Param p = {&kenshiWindows, targetTitle};
  EnumWindows(EnumKenshiWindows, (LPARAM)&p);

  if (kenshiWindows.size() >= 2) {
    // Sort by HWND to ensure consistent ordering across instances
    std::sort(kenshiWindows.begin(), kenshiWindows.end());

    HWND myHwnd = NULL;
    DWORD myPid = GetCurrentProcessId();
    for (size_t i = 0; i < kenshiWindows.size(); ++i) {
      HWND h = kenshiWindows[i];
      DWORD pid;
      GetWindowThreadProcessId(h, &pid);
      if (pid == myPid) {
        myHwnd = h;
        break;
      }
    }

    if (myHwnd) {
      RECT rect;
      SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

      int workW = rect.right - rect.left;
      int workH = rect.bottom - rect.top;

      int topPadding = (workH * 15) / 100;
      int winH = workH - topPadding;
      int halfW = workW / 2;

      if (myHwnd == kenshiWindows[0]) {
        // Bottom Left - No gaps
        MoveWindow(myHwnd, rect.left, rect.top + topPadding, halfW, winH, TRUE);
      } else if (myHwnd == kenshiWindows[1]) {
        // Bottom Right - Fill the rest of the width to avoid gaps
        int win2W = workW - halfW;
        MoveWindow(myHwnd, rect.left + halfW, rect.top + topPadding, win2W,
                   winH, TRUE);
      }
    }
  }

  return 0;
}

extern "C" __declspec(dllexport) void dllStartPlugin(void) {
  // Start the auto-accept thread
  CreateThread(NULL, 0, AutoAcceptThread, NULL, 0, NULL);
}

extern "C" __declspec(dllexport) void dllStopPlugin(void) {}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  return TRUE;
}
