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

#include "evil-wof.h"

std::vector<ForbiddenPacket> forbiddenPackets = {
    {"4c0007190_______________00_____", "APPLE_DEVICE_POPUP"}, // not working ?
    {"4c000f05c0_____________________", "APPLE_ACTION_MODAL"}, // refactored for working
    {"4c00071907_____________________", "APPLE_DEVICE_CONNECT"}, // working
    {"4c0004042a0000000f05c1__604c950", "APPLE_DEVICE_SETUP"}, // working
    {"2cfe___________________________", "ANDROID_DEVICE_CONNECT"}, // not working cant find raw data in sniff
    {"750000000000000000000000000000_", "SAMSUNG_BUDS_POPUP"},// refactored for working
    {"7500010002000101ff000043_______", "SAMSUNG_WATCH_PAIR"},//working
    {"0600030080_____________________", "WINDOWS_SWIFT_PAIR"},//working
    {"ff006db643ce97fe427c___________", "LOVE_TOYS"} // working
};

void recordFlipper(const String& name, const String& macAddress, const String& color, bool isValidMac) {
    if (!isMacAddressRecorded(macAddress)) {
        File file = openFile("/WoF.txt", FILE_APPEND);
        if (file) {
            String status = isValidMac ? " - normal" : " - spoofed"; // Détermine le statut basé sur isValidMac
            file.println(name + " - " + macAddress + " - " + color + status);
            sendMessage("Flipper saved: \n" + name + " - " + macAddress + " - " + color + status);
        }
        file.close();
    }
}

bool matchPattern(const char* pattern, const uint8_t* payload, size_t length) {
    size_t patternLength = strlen(pattern);
    for(size_t i = 0, j = 0; i < patternLength && j < length; i+=2, j++) {
        char byteString[3] = {pattern[i], pattern[i+1], 0};
        if(byteString[0] == '_' && byteString[1] == '_') continue;

        uint8_t byteValue = strtoul(byteString, nullptr, 16);
        if(payload[j] != byteValue) return false;
    }
    return true;
}

bool isMacAddressRecorded(const String& macAddress) {
    File file = openFile("/WoF.txt", FILE_READ);
    if (!file) {
        return false;
    }
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.indexOf(macAddress) >= 0) {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    int lineCount = 0;
    const int maxLines = 10;
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        String deviceColor = "Unknown"; // Défaut
        bool isValidMac = false; // validité de l'adresse MAC
        bool isFlipper = false; // Flag pour identifier si le dispositif est un Flipper

        // Vérifier directement les UUIDs pour déterminer la couleur
        if (advertisedDevice.isAdvertisingService(BLEUUID("00003082-0000-1000-8000-00805f9b34fb"))) {
            deviceColor = "White";
            isFlipper = true;
        } else if (advertisedDevice.isAdvertisingService(BLEUUID("00003081-0000-1000-8000-00805f9b34fb"))) {
            deviceColor = "Black";
            isFlipper = true;
        } else if (advertisedDevice.isAdvertisingService(BLEUUID("00003083-0000-1000-8000-00805f9b34fb"))) {
            deviceColor = "Transparent";
            isFlipper = true;
        }

        // Continuer uniquement si un Flipper est identifié
        if (isFlipper) {
            String macAddress = advertisedDevice.getAddress().toString().c_str();
            if (macAddress.startsWith("80:e1:26") || macAddress.startsWith("80:e1:27")) {
                isValidMac = true;
            }

            M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            M5.Display.setCursor(0, 10);
            String name = advertisedDevice.getName().c_str();

            M5.Display.printf("Name: %s\nRSSI: %d \nMAC: %s\n",
            name.c_str(),
            advertisedDevice.getRSSI(),
            macAddress.c_str());
            recordFlipper(name, macAddress, deviceColor, isValidMac); // Passer le statut de validité de l'adresse MAC
        }

        std::string advData = advertisedDevice.getManufacturerData();
        if (!advData.empty()) {
            const uint8_t* payload = reinterpret_cast<const uint8_t*>(advData.data());
            size_t length = advData.length();

/*
            Serial.print("Raw Data: ");
            for (size_t i = 0; i < length; i++) {
                Serial.printf("%02X", payload[i]); // Afficher chaque octet en hexadécimal
            }
            Serial.println(); // Nouvelle ligne après les données brutes
*/

            for (auto& packet : forbiddenPackets) {
                if (matchPattern(packet.pattern, payload, length)) {
                    if (lineCount >= maxLines) {
                        M5.Display.fillRect(0, 58, 325, 185, BLACK); // Réinitialiser la zone d'affichage des paquets interdits
                        M5.Display.setCursor(0, 59);
                        lineCount = 0; // Réinitialiser si le maximum est atteint
                    }
                    M5.Display.printf("%s\n", packet.type);
                    lineCount++;
                    break;
                }
            }
        }
    }
};

EvilWoF::EvilWoF() {
    isBLEInitialized = false;
}

void EvilWoF::emptyWoFCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Stop");
}

void EvilWoF::showWoFApp() {
    // Set up application for first run
    if (!isAppRunning) {
        initializeBLEIfNeeded();
        toggleAppRunning();
    }

    // Run the app
    if (ui.clearScreenDelay()) {
        ui.writeMessageXY("Waiting for Flipper", 0, 10, false);
        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
        pBLEScan->setActiveScan(true);
        pBLEScan->start(1, false);
    }
}

void EvilWoF::closeWoFApp() {
    ui.waitAndReturnToMenu("Stop detection...");
    toggleAppRunning();
}

void EvilWoF::toggleAppRunning() {
    isAppRunning = !isAppRunning;
}

void EvilWoF::initializeBLEIfNeeded() {
    if (!isBLEInitialized) {
        BLEDevice::init("");
        isBLEInitialized = true;
        sendMessage("BLE initialized for scanning.");
    }
}
