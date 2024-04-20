/*
    Evil-M5Core2 - WiFi Network Testing and Exploration Tool

    Copyright (c) 2024 7h30th3r0n3
    Copyright (c) 2024 lvlhead

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Disclaimer:
    This tool, Evil-M5Core2, is developed for educational and ethical testing purposes only.
    Any misuse or illegal use of this tool is strictly prohibited. The creator of Evil-M5Core2
    assumes no liability and is not responsible for any misuse or damage caused by this tool.
    Users are required to comply with all applicable laws and regulations in their jurisdiction
    regarding network testing and ethical hacking.
*/

#ifndef EVILUI_H
#define EVILUI_H

#include <SD.h>
#include <M5Unified.h>
#include <vector>


class EvilUI {
  public:
    EvilUI();
    void init();
    void drawImage(const char *filepath);
    // Screens
    void aboutScreen(String message);
    void textFullScreen(String message);
    // Menu
    void menuLoop();
    void drawMenu();
    void handleMenuInput();
    void executeMenuItem(int index);
    void waitAndReturnToMenu(String message);
    void resetMenuDraw();
    // Helpers
    void setOperationInProgress();
    void resetLastIndex();
    bool getInMenu();
    void setInMenu(bool val);
    typedef void (EvilUI::*currentMenuInput)(int);
    currentMenuInput menuInput_funcPtr;

  private:
    std::vector<String> arrayMainMenu();
    std::vector<String> arraySubMenu1();
    std::vector<String> currentMenu;
    int menuSize;
    int currentIndex;
    int lastIndex;
    bool inMenu;
    int maxMenuDisplay;
    int menuStartIndex;
    bool isOperationInProgress;
};

#endif
