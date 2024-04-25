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
    #include "esp_system.h"
}

#include "evil-beacon.h"

EvilBeacon::EvilBeacon() {
    isAppRunning = false;
    ssid = "";
}

void EvilBeacon::init() {
    beaconCount = 0;
}

void EvilBeacon::emptyBeaconCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Stop");
}

void EvilBeacon::showBeaconApp() {
    // Set up application for first run
    if (!isAppRunning) {
        init();
        WiFi.mode(WIFI_MODE_AP);

        // Ask the user if they want to use custom beacons
        if (ui.confirmPopup("Use custom beacons?", false)) {
            customBeacons = readCustomBeacons("/config/config.txt"); // Replace with the actual path
        }

        sendUtilMessage("-------------------");
        sendUtilMessage("Starting Beacon Spam");
        sendUtilMessage("-------------------");
        toggleAppRunning();
    }

    // Run the app
    if (ui.clearScreenDelay()) {
        beaconAttack();
    }
}

void EvilBeacon::closeBeaconApp() {
    toggleAppRunning();
    sendUtilMessage("-------------------");
    sendUtilMessage("Stopping beacon Spam");
    sendUtilMessage("-------------------");
    wireless.restoreOriginalWiFiSettings();
    ui.waitAndReturnToMenu("Beacon Spam Stopped...");
}

void EvilBeacon::toggleAppRunning() {
    isAppRunning = !isAppRunning;
}

void EvilBeacon::beaconAttack() {
    std::vector<String> messages;
    messages.push_back("Beacon Spam running...");
    messages.push_back("Beacon sent:");
    ui.writeVectorMessage(messages, 10, 50, 20);

    // Generate a new SSID for the beacon
    if (!customBeacons.empty()) {
        ssid = customBeacons[beaconCount % customBeacons.size()]; // Use a custom beacon
    } else {
        ssid = wireless.generateRandomSSID(32); // Use a random beacon
    }

    // Clear the previous SSID display area
    M5.Display.setTextSize(1.5);
    ui.writeMessageXY(ssid, 5, 90, false);

    WiFi.softAP(ssid.c_str());
    delay(50);
    for (int channel = 1; channel <= 13; ++channel) {
        wireless.setRandomMAC_APKarma();
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        delay(50);
    }

    beaconCount++;
}

std::vector<String> EvilBeacon::readCustomBeacons(const char* filename) {
    File file = openFile(filename, FILE_READ);
    std::vector<String> customBeacons;

    if (!file) {
        return customBeacons;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.startsWith("CustomBeacons=")) {
            String beaconsStr = line.substring(String("CustomBeacons=").length());
            int idx = 0;
            while ((idx = beaconsStr.indexOf(',')) != -1) {
                customBeacons.push_back(beaconsStr.substring(0, idx));
                beaconsStr = beaconsStr.substring(idx + 1);
            }
            if (beaconsStr.length() > 0) {
                customBeacons.push_back(beaconsStr); // Add the last element
            }
            break;
        }
    }
    file.close();
    return customBeacons;
}
