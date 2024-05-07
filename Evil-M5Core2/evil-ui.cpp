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

EvilUI::EvilUI() {
    previousMillis = 0;
}

void EvilUI::init() {
}

void EvilUI::drawImage(const char *filepath) {
    fs::File file = SD.open(filepath);
    M5.Display.drawJpgFile(SD, filepath);
    file.close();
}

void EvilUI::writeSingleMessage(String message, bool clearScreen = false) {
    if (clearScreen) {
        M5.Display.clear();
    }
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(message, M5.Display.width() / 2, M5.Display.height() / 2);
}

void EvilUI::writeMessageXY(String message, int x, int y, bool clearScreen = false) {
    if (clearScreen) {
        M5.Display.clear();
    }
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.setCursor(x, y);
    M5.Display.println(message + "    ");
}

void EvilUI::writeMessageXY_small(String message, int x, int y, bool clearScreen = false) {
    if (clearScreen) {
        M5.Display.clear();
    }
    M5.Display.setTextSize(1.5);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    M5.Display.setCursor(x, y);
    M5.Display.println(message + "   ");
}

void EvilUI::writeVectorMessage(std::vector<String> messages, int x, int y, int incr) {
    //M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_BLACK, TFT_LIGHTGREY);

    for (int i = 0; i < messages.size(); i++)
    {
        M5.Display.setCursor(x, y);
        M5.Display.println(messages[i] + "   ");
        y += incr;
    }

    M5.Display.display();
}

void EvilUI::aboutScreen(String message) {
    int textY = 80;
    int lineOffset = 10;
    int lineY1 = textY - lineOffset;
    int lineY2 = textY + lineOffset + 30;

    M5.Display.clear(LIGHTGREY);
    M5.Display.drawLine(0, lineY1, M5.Display.width(), lineY1, BLACK);
    M5.Display.drawLine(0, lineY2, M5.Display.width(), lineY2, BLACK);

    writeMessageXY("Evil-M5Core2", 80, 80, false);
    writeMessageXY("By 7h30th3r0n3", 75, 100, false);
    writeMessageXY(APP_VERSION, 102, 125, false);
    writeMessageXY(message, 0, 200, false);
}

void EvilUI::waitAndReturnToMenu(String message) {
    writeSingleMessage(message, true);
    delay(1500);
}

bool EvilUI::confirmPopup(String message, bool clearScreen) {
    bool confirm = false;
    bool decisionMade = false;

    writeSingleMessage(message, clearScreen);
    M5.Display.setCursor(37, 220);
    M5.Display.setTextColor(TFT_GREEN);
    M5.Display.println("Yes");
    M5.Display.setTextColor(TFT_RED);
    M5.Display.setCursor(254, 220);
    M5.Display.println("No");
    M5.Display.setTextColor(TFT_WHITE);

    while (!decisionMade) {
        M5.update();
        if (M5.BtnA.wasPressed()) {
            confirm = true;
            decisionMade = true;
        }
        if (M5.BtnC.wasPressed()) {
            confirm = false;
            decisionMade = true;
        }
    }

    return confirm;
}

void EvilUI::clearAppScreen() {
    M5.Display.fillRect(0, 0, M5.Display.width(), 200, TFT_LIGHTGREY);
}

bool EvilUI::waitToRefresh() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) {
            previousMillis = currentMillis;
            return true;
    }
    return false;
}

bool EvilUI::clearScreenDelay() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 500) {
            previousMillis = currentMillis;
            clearAppScreen();
            return true;
    }
    return false;
}
