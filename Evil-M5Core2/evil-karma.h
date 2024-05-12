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

#ifndef EVILKARMA_H
#define EVILKARMA_H

#include <set>
#include <string>

#include <M5StackMenuSystem.h>
#include <M5Unified.h>
#include <WiFi.h>

#include "evil-ui.h"
#include "evil-led.h"
#include "evil-util.h"
#include "evil-config.h"

extern "C" void autoKarmaPacketSniffer(void* buf, wifi_promiscuous_pkt_type_t type);
extern "C" void packetSnifferKarma(void* buf, wifi_promiscuous_pkt_type_t type);
extern bool isAPDeploying;
extern bool isScanningKarma;
extern bool newSSIDAvailable;
extern bool isScanningKarma;
extern int ssid_count_Karma;
extern char lastSSID[33];

class EvilKarma {
  public:
    EvilKarma();
    void showKarmaApp();
    void closeKarmaApp();
    static void emptyKarmaCallback(CallbackMenuItem& menuItem);
    void showAutoKarmaApp();
    void closeAutoKarmaApp();
    static void emptyAutoKarmaCallback(CallbackMenuItem& menuItem);
    void toggleAppRunning();
    void setIsKarmaMode(bool state);
    bool getIsAutoKarmaActive();
    bool getIsProbeKarmaAttackMode();
    bool getIsProbeSniffingMode();
    void setIsProbeSniffingMode(bool state);
    void saveSSIDToFile(const char* ssid);

  private:
    EvilUI ui;
    EvilConfig config;
    EvilLED led;
    bool isAppRunning;
    bool isAPDeploying;
    bool isKarmaMode;
    bool isProbeSniffingMode;
    bool isProbeKarmaAttackMode;
    std::set<std::string> seenWhitelistedSSIDs;
    int currentIndexKarma;
    int menuStartIndexKarma;
    int menuSizeKarma;
    int maxMenuDisplayKarma;
    bool newSSIDAvailable;
    int autoKarmaAPDuration; // Time for Auto Karma Scan can be ajusted if needed consider only add time(Under 10s to fast to let the device check and connect to te rogue AP)
    bool isAutoKarmaActive;
    bool isWaitingForProbeDisplayed;
    unsigned long lastProbeDisplayUpdate;
    int probeDisplayState;
    bool isInitialDisplayDone;
    char lastDeployedSSID[33];
    bool karmaSuccess;
};

#endif
