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

#include "evil-ui.h"
#include "evil-util.h"

EvilUI::EvilUI() {
}

void EvilUI::init() {
    maxMenuDisplay = 10;
    resetMenuDraw();
    // Configure display text/font settings
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setTextFont(1);
    // Setup current menu
    currentMenu = arrayMainMenu();
    menuSize = currentMenu.size();
    menuInput_funcPtr = &handleMenuInput;
}

void EvilUI::drawImage(const char *filepath) {
    fs::File file = SD.open(filepath);
    M5.Display.drawJpgFile(SD, filepath);
    file.close();
}

void EvilUI::textFullScreen(String message) {
}

void EvilUI::aboutScreen(String message) {
    int textY = 80;
    int lineOffset = 10;
    int lineY1 = textY - lineOffset;
    int lineY2 = textY + lineOffset + 30;

    M5.Display.clear();
    M5.Display.drawLine(0, lineY1, M5.Display.width(), lineY1, TFT_WHITE);
    M5.Display.drawLine(0, lineY2, M5.Display.width(), lineY2, TFT_WHITE);

    M5.Display.setCursor(80, textY);
    M5.Display.println(" Evil-M5Core2");
    M5.Display.setCursor(75, textY + 20);
    M5.Display.println("By 7h30th3r0n3");
    M5.Display.setCursor(102, textY + 45);
    M5.Display.println(APP_VERSION);
    M5.Display.setCursor(0 , textY + 120);
    M5.Display.println(message);
}

void EvilUI::menuLoop() {
    if (getInMenu()) {
        if (lastIndex != currentIndex) {
            drawMenu();
            lastIndex = currentIndex;
        }
        handleMenuInput();
    } else {
/*        switch (currentStateKarma) {
            case StartScanKarma:
                if (M5.BtnB.wasPressed()) {
                    startScanKarma();
                    currentStateKarma = ScanningKarma;
                }
                break;

            case ScanningKarma:
                if (M5.BtnB.wasPressed()) {
                    isKarmaMode = true;
                    stopScanKarma();
                    currentStateKarma = ssid_count_Karma > 0 ? StopScanKarma : StartScanKarma;
                }
                break;

            case StopScanKarma:
                handleMenuInputKarma();
                break;

            case SelectSSIDKarma:
                handleMenuInputKarma();
                break;
        }

        if (M5.BtnB.wasPressed() && currentStateKarma == StartScanKarma) {
            setInMenu(true);
            isOperationInProgress = false;
        }
*/
    }
}

std::vector<String> EvilUI::arrayMainMenu() {
    std::vector<String> menuItems = {"Scan WiFi", "Select Network", "Clone & Details",
                                     "Start Captive Portal", "Stop Captive Portal", "Change Portal",
                                     "Check Credentials", "Delete All Credentials", "Monitor Status",
                                     "Probe Attack", "Probe Sniffing", "Karma Attack", "Karma Auto",
                                     "Karma Spear", "Select Probe", "Delete Probe", "Delete All Probes",
                                     "Settings", "Wardriving", "Beacon Spam",
                                     "Deauth Detection", "Wall Of Flipper"};
    return menuItems;
}

std::vector<String> EvilUI::arraySubMenu1() {
    std::vector<String> menuItems = {"Brightness", "Bluetooth ON/OFF", "Return to Main Menu"};
    return menuItems;
}

void EvilUI::drawMenu() {
    menuSize = currentMenu.size();
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextFont(1);

    int lineHeight = 24;
    int startX = 5;
    int startY = 0;

    for (int i = 0; i < maxMenuDisplay; i++) {
        int menuIndex = menuStartIndex + i;
        if (menuIndex >= menuSize) break;

        if (menuIndex == currentIndex) {
            M5.Display.fillRect(0, startY + i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
            M5.Display.setTextColor(TFT_GREEN);
        } else {
            M5.Display.setTextColor(TFT_WHITE);
        }
        M5.Display.setCursor(startX, startY + i * lineHeight + (lineHeight / 2) - 8);
        M5.Display.println(currentMenu[menuIndex]);
    }
    M5.Display.display();
}

void EvilUI::handleMenuInput() {
    if (M5.BtnA.wasPressed()) {
        currentIndex--;
        if (currentIndex < 0) {
            currentIndex = menuSize - 1;
        }
    } else if (M5.BtnC.wasPressed()) {
        currentIndex++;
        if (currentIndex >= menuSize) {
            currentIndex = 0;
        }
    }
    menuStartIndex = max(0, min(currentIndex, menuSize - maxMenuDisplay));

    if (M5.BtnB.wasPressed()) {
        executeMenuItem(currentIndex);
    }
}

void EvilUI::executeMenuItem(int index) {
    setInMenu(false);
    isOperationInProgress = true;
    switch (index) {
        case 0:
            //scanWifiNetworks();
            break;
        case 1:
            //showWifiList();
            break;
        case 2:
            //showWifiDetails(currentListIndex);
            break;
        case 3:
            //createCaptivePortal();
            break;
        case 4:
            //stopCaptivePortal();
            break;
        case 5:
            //changePortal();
            break;
        case 6:
            //checkCredentials();
            break;
        case 7:
            //deleteCredentials();
            break;
        case 8:
            //displayMonitorPage1();
            break;
        case 9:
            //probeAttack();
            break;
        case 10:
            //probeSniffing();
            break;
        case 11:
            //karmaAttack();
            break;
        case 12:
            //startAutoKarma();
            break;
        case 13:
            //karmaSpear();
            break;
        case 14:
            //listProbes();
            break;
        case 15:
            //deleteProbe();
            break;
        case 16:
            //deleteAllProbes();
            break;
        case 17:
            //brightness();
            currentMenu = arraySubMenu1();
            resetMenuDraw();
            break;
        case 18:
            //onOffBleSerial();
            break;
        case 19:
            //wardrivingMode();
            break;
        case 20:
            //beaconAttack();
            break;
        case 21:
            //deauthDetect();
            break;
        case 22:
            //wallOfFlipper();
            break;
    }
    isOperationInProgress = false;
}

void EvilUI::waitAndReturnToMenu(String message) {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.fillRect(0, M5.Display.height() - 20, M5.Display.width(), 20, TFT_BLACK);
    M5.Display.setCursor(50 , M5.Display.height()/ 2 );
    M5.Display.println(message);
    M5.Display.display();
    delay(1500);
    setInMenu(true);
    drawMenu();
}

void EvilUI::resetMenuDraw() {
    currentIndex = 0;
    lastIndex = -1;
    inMenu = true;
    menuStartIndex = 0;
    isOperationInProgress = false;
}

void EvilUI::setOperationInProgress() {
    isOperationInProgress = true;
    setInMenu(false);
}

void EvilUI::resetLastIndex() {
    lastIndex = -1;
}

bool EvilUI::getInMenu() {
    return inMenu;
}

void EvilUI::setInMenu(bool val) {
    inMenu = val;
}
