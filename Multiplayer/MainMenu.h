#pragma once

#include <core/Functions.h>
#include <kenshi/TitleScreen.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_LayerManager.h>

#include <string>

// Extern globals from Multiplayer.cpp
extern MyGUI::Window *mPanel;
extern MyGUI::ListBox *mListBox;
extern void LogToChat(const std::string &msg);

// Main Menu Button Globals
MyGUI::Button *multiplayerButton = nullptr;
bool mainMenuInitialized = false;

// Callback when Multiplayer button is clicked
void OnMultiplayerButtonClick(MyGUI::Widget *_sender) {
  LogToChat("=== Multiplayer Menu Clicked ===");
  LogToChat("Multiplayer features coming soon!");

  // Show the multiplayer UI if it's hidden
  if (mPanel && !mPanel->getVisible()) {
    mPanel->setVisible(true);
  }
}

// Safe text retrieval helper
std::string GetWidgetText(MyGUI::Widget *w) {
  if (!w)
    return "";
  try {
    // Check type name string (safer than RTTI in some contexts)
    if (w->getTypeName() == "Button") {
      // We know it's a button, so static_cast is safe enough
      return static_cast<MyGUI::Button *>(w)->getCaption().asUTF8();
    }
    return "";
  } catch (...) {
    return "";
  }
}

// Recursive search for a widget with specific text
MyGUI::Widget *FindWidgetByText(MyGUI::Widget *parent,
                                const std::string &text) {
  if (!parent)
    return nullptr;

  MyGUI::EnumeratorWidgetPtr children = parent->getEnumerator();
  while (children.next()) {
    MyGUI::Widget *child = children.current();
    if (!child)
      continue;

    std::string cap = GetWidgetText(child);
    std::string name = child->getName();
    std::string type = child->getTypeName();

    if (cap.find(text) != std::string::npos ||
        name.find(text) != std::string::npos) {
      // LogToChat("!!! FOUND MATCH: " + name + " (Type: " + type + ") !!!");
      return child;
    }

    // Setup recursion
    MyGUI::Widget *found = FindWidgetByText(child, text);
    if (found)
      return found;
  }
  return nullptr;
}

// Hook for TitleScreen constructor to add our button
void AddMultiplayerButton(TitleScreen *titleScreen) {
  if (mainMenuInitialized || !titleScreen) {
    return;
  }

  MyGUI::Gui *gui = MyGUI::Gui::getInstancePtr();
  if (!gui) {
    return;
  }

  // Get the main widget from the title screen
  MyGUI::Widget *mainWidget = titleScreen->mMainWidget;
  if (!mainWidget) {
    return;
  }

  MyGUI::Widget *anchorButton = nullptr;

  // LogToChat("Scanning layout for buttons...");

  // Try to find "Continue" first
  anchorButton = FindWidgetByText(mainWidget, "Continue");
  if (!anchorButton) {
    LogToChat("Continue button not found, searching for New Game...");
    anchorButton = FindWidgetByText(mainWidget, "New Game");
  }

  if (anchorButton) {
    // LogToChat("Found anchor button: " + anchorButton->getName() + " ('" +
    // GetWidgetText(anchorButton) + "')");
  } else {
    LogToChat("No anchor button found (Continue/New Game).");
  }

  if (anchorButton) {

    // Instead of attaching to a layer, attach to the PARENT Widget of the
    // anchor. This allows the button to inherit visibility/destruction events
    // from the TitleScreen.
    MyGUI::Widget *parent = anchorButton->getParent();

    // Get dimensions first
    int width = anchorButton->getWidth();
    int height = anchorButton->getHeight();

    if (parent) {
      // Use RELATIVE coordinates to the parent
      int relX = anchorButton->getLeft();
      int relY = anchorButton->getTop();
      int targetX = relX + width + 20;
      int targetY = relY;

      std::string skinName = "Kenshi_Button1";
      if (anchorButton->isUserString("Skin"))
        skinName = anchorButton->getUserString("Skin");

      multiplayerButton = parent->createWidget<MyGUI::Button>(
          skinName, targetX, targetY, width, height, MyGUI::Align::Default,
          "MultiplayerButton");
    } else {
      // Fallback to layer if no parent found (unlikely if anchor exists)
      // Use ABSOLUTE coordinates
      int absX = anchorButton->getAbsoluteLeft();
      int absY = anchorButton->getAbsoluteTop();
      int targetX = absX + width + 20;
      int targetY = absY;
      std::string targetLayer = "Main";
      multiplayerButton = gui->createWidget<MyGUI::Button>(
          "Kenshi_Button1", targetX, targetY, width, height,
          MyGUI::Align::Default, targetLayer, "MultiplayerButton");
    }

  } else {
    // Fallback using GUI directly
    multiplayerButton = gui->createWidget<MyGUI::Button>(
        "Kenshi_Button1", 450, 450, 200, 40, MyGUI::Align::Default, "Main",
        "MultiplayerButton");
  }

  if (multiplayerButton) {
    multiplayerButton->setCaption("MULTIPLAYER");

    // Make sure it handles inputs
    multiplayerButton->setNeedMouseFocus(true);
    multiplayerButton->setNeedKeyFocus(true);

    multiplayerButton->eventMouseButtonClick +=
        MyGUI::newDelegate(OnMultiplayerButtonClick);

    mainMenuInitialized = true;
    multiplayerButton->setVisible(true);

    if (mListBox) {
      LogToChat("Main menu button added successfully!");
    }
  }
}

// Alternative approach: Hook TitleScreen::show to add button when menu is
// shown
void (*TitleScreen_show_orig)(TitleScreen *, bool) = nullptr;
void TitleScreen_show_hook(TitleScreen *thisptr, bool on) {
  // Call original
  if (TitleScreen_show_orig) {
    TitleScreen_show_orig(thisptr, on);
  }

  // Add our button when the title screen is shown
  if (on && !mainMenuInitialized) {
    AddMultiplayerButton(thisptr);
  }
}
