#define OGRE_STATIC_LIB
#define _OgreExport

#include <sstream>
#include <string>
#include <vector>

#include <kenshi/Building.h>
#include <kenshi/CharStats.h>
#include <kenshi/Character.h>
#include <kenshi/CombatTechniqueData.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/InputHandler.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/Platoon.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObjectFactory.h>
#include <kenshi/TitleScreen.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Delegate.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ListBox.h>
#include <mygui/MyGUI_Window.h>
#include <ois/OISKeyboard.h>

#include <core/Functions.h>

// UI Globals
MyGUI::ListBox *mListBox = NULL;
MyGUI::EditBox *mEditBox = NULL;
MyGUI::Window *mPanel = NULL;
bool uiInitialized = false;
bool uiExpanded = true;
int initialHeight = 0;
int initialWidth = 0;

// Chat history
std::vector<std::string> chatHistory;
int historyPos = -1;
#include ".commands.h"
#include ".network.h"
#include "MainMenu.h"
#include "tests.h"

// Log function for Registry
void LogToChat(const std::string &msg) {
  // Log to file
  FILE *fp = fopen("multiplayer_debug.log", "a");
  if (fp) {
    fprintf(fp, "%s\n", msg.c_str());
    fclose(fp);
  }

  if (mListBox) {
    mListBox->addItem(msg);
    // Scroll to bottom
    mListBox->beginToItemLast();
  }
}

// Key press handler for history navigation
void OnKeyButtonPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key,
                        MyGUI::Char _char) {
  if (chatHistory.empty())
    return;

  if (_key == MyGUI::KeyCode::ArrowUp) {
    if (historyPos < (int)chatHistory.size() - 1) {
      historyPos++;
      mEditBox->setCaption(chatHistory[chatHistory.size() - 1 - historyPos]);
    }
  } else if (_key == MyGUI::KeyCode::ArrowDown) {
    if (historyPos > 0) {
      historyPos--;
      mEditBox->setCaption(chatHistory[chatHistory.size() - 1 - historyPos]);
    } else if (historyPos == 0) {
      historyPos = -1;
      mEditBox->setCaption("");
    }
  }
}

// On Enter pressed in EditBox
void OnEditAccept(MyGUI::EditBox *_sender) {
  std::string text = _sender->getCaption().asUTF8();
  if (text.empty())
    return;

  // Save to history
  if (chatHistory.empty() || chatHistory.back() != text) {
    chatHistory.push_back(text);
  }
  historyPos = -1;

  bool commandProcessed = false;
  if (text[0] == '/') {
    commandProcessed = run_cmd(text);
  }

  if (!commandProcessed && mListBox) {
    // Get all selected characters
    lektor<Character *> selection = getSelectedChars();

    // Only proceed if we have at least one character selected
    if (selection.size() > 0) {
      // Local echo in chat log
      std::string msg = "[" + myName + "]: " + text;
      mListBox->addItem(msg);
      mListBox->beginToItemLast();

      // Send ONE chat message to server for other clients' logs
      sendSignal("chat", text);

      // Make all selected characters speak and send say signals for speech
      // bubbles
      for (unsigned int i = 0; i < selection.size(); ++i) {
        Character *speaker = selection[i];
        if (speaker) {
          // Local speech bubble
          chat(speaker, text);

          // Send say signal for this character's speech bubble on other clients
          std::stringstream ss;
          ss << (long long)speaker << " " << text;
          sendSignal("say", ss.str());
        }
      }
    }
  }

  _sender->setCaption("");
}

// Toggle UI Callback
void ToggleUI(MyGUI::Widget *sender) {
  if (!mListBox || !mPanel || !mEditBox)
    return;

  uiExpanded = !uiExpanded;

  if (uiExpanded) {
    // Expand
    mListBox->setVisible(true);
    mEditBox->setVisible(true);
    if (initialHeight > 0 && initialWidth > 0)
      mPanel->setSize(initialWidth, initialHeight);
  } else {
    // Collapse
    if (initialHeight == 0) {
      initialHeight = mPanel->getHeight();
      initialWidth = mPanel->getWidth();
    }
    mListBox->setVisible(false);
    mEditBox->setVisible(false);

    // Shrink in both directions
    mPanel->setSize(150, 28);
  }
}

// Window Event Callback to intercept the Close button
void OnWindowEvent(MyGUI::Window *sender, const std::string &name) {
  if (name == "close") {
    ToggleUI(sender);
  }
}

// Helper to create the UI
void CreateUI() {
  if (uiInitialized)
    return;

  MyGUI::Gui *gui = MyGUI::Gui::getInstancePtr();
  if (!gui)
    return;

  // Create a MyGUI::Window in the bottom left
  mPanel = gui->createWidgetReal<MyGUI::Window>(
      "Kenshi_WindowCX", 0.01f, 0.45f, 0.25f, 0.35f, MyGUI::Align::Default,
      "Main", "MultiplayerStatusPanel");
  mPanel->setCaption("Multiplayer Log");
  mPanel->setMovable(true);

  // Bind the window event to catch the "X" button press
  mPanel->eventWindowButtonPressed += MyGUI::newDelegate(OnWindowEvent);

  // Create widgets on the CLIENT widget of the window
  MyGUI::Widget *client = mPanel->getClientWidget();

  // ListBox - reduced height (0.80) to leave room for EditBox at the bottom
  mListBox = client->createWidgetReal<MyGUI::ListBox>(
      "Kenshi_ListBox", 0.02f, 0.02f, 0.96f, 0.80f, MyGUI::Align::Stretch,
      "StatusList");

  // EditBox - placed at the bottom (y=0.85)
  mEditBox = client->createWidgetReal<MyGUI::EditBox>(
      "Kenshi_EditBox", 0.02f, 0.85f, 0.96f, 0.12f,
      MyGUI::Align::Bottom | MyGUI::Align::HStretch, "InputBox");
  mEditBox->setEditMultiLine(false);
  mEditBox->eventEditSelectAccept += MyGUI::newDelegate(OnEditAccept);
  mEditBox->eventKeyButtonPressed += MyGUI::newDelegate(OnKeyButtonPressed);

  uiInitialized = true;
}

// TitleScreen constructor hook
TitleScreen *(*TitleScreen_orig)(TitleScreen *) = NULL;
TitleScreen *TitleScreen_hook(TitleScreen *thisptr) {
  TitleScreen *screen = TitleScreen_orig(thisptr);
  CreateUI();
  AddMultiplayerButton(screen);
  return screen;
}

// TitleScreen update hook
void (*TitleScreen_update_orig)(TitleScreen *) = NULL;
void TitleScreen_update_hook(TitleScreen *thisptr) {
  TitleScreen_update_orig(thisptr);

  if (!uiInitialized) {
    CreateUI();
  }
}

// GameWorld update hook
void (*GameWorld_update_orig)(GameWorld *, float) = NULL;
void GameWorld_update_hook(GameWorld *thisptr, float time) {
  if (GameWorld_update_orig)
    GameWorld_update_orig(thisptr, time);

  receiveSignal();
  update(); // Every frame

  static unsigned long lastFixedUpdate = 0;
  unsigned long now = GetTickCount();
  if (now - lastFixedUpdate >= (unsigned long)(1000 / targetFPS)) {
    lastFixedUpdate = now;
    fixedUpdate();
  }
}

__declspec(dllexport) void startPlugin() {
  InitCommands();
  InitTests();
  KenshiLib::AddHook(KenshiLib::GetRealAddress(&TitleScreen::_CONSTRUCTOR),
                     TitleScreen_hook, (void **)&TitleScreen_orig);

  KenshiLib::AddHook(KenshiLib::GetRealAddress(&TitleScreen::_NV_update),
                     TitleScreen_update_hook,
                     (void **)&TitleScreen_update_orig);

  KenshiLib::AddHook(
      KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff),
      GameWorld_update_hook, (void **)&GameWorld_update_orig);
}
