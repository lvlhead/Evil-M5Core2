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

#include "evil-wireless.h"

EvilWireless::EvilWireless() {
    currentChannel = 1;
    originalChannel = 1;
    numSsid = 0;
}

String EvilWireless::generateRandomSSID(int length) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    String randomString;
    for (int i = 0; i < length; i++) {
        int index = random(0, sizeof(charset) - 1);
        randomString += charset[index];
    }
    return randomString;
}

String EvilWireless::getMACAddress() {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void EvilWireless::setRandomMAC_APKarma() {
    esp_wifi_stop();

    wifi_mode_t currentMode;
    esp_wifi_get_mode(&currentMode);
    if (currentMode != WIFI_MODE_AP && currentMode != WIFI_MODE_APSTA) {
        esp_wifi_set_mode(WIFI_MODE_AP);
    }

    String macKarma = generateRandomMAC(true);
    uint8_t macArrayKarma[6];
    sscanf(macKarma.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &macArrayKarma[0], &macArrayKarma[1], &macArrayKarma[2], &macArrayKarma[3], &macArrayKarma[4], &macArrayKarma[5]);

    esp_err_t ret = esp_wifi_set_mac(WIFI_IF_AP, macArrayKarma);
    if (ret != ESP_OK) {
        sendMessage("Error setting MAC: " + String(ret));
        esp_wifi_set_mode(currentMode);
        return;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        sendMessage("Error starting WiFi: " + String(ret));
        esp_wifi_set_mode(currentMode);
        return;
    }
}

void EvilWireless::restoreOriginalWiFiSettings() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_deinit();
    delay(300); // Short break to make sure everything is finished
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_start();
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    restoreOriginalMAC();
    WiFi.mode(WIFI_STA);
}

String EvilWireless::generateRandomMAC(bool karmaMode) {
    uint8_t mac[6];
    for (int i = 0; i < 6; ++i) {
        mac[i] = random(0x00, 0xFF);
    }

    if (karmaMode) {
        // Force unicast byte
        mac[0] &= 0xFE;
    }

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void EvilWireless::saveOriginalMAC() {
    esp_wifi_get_mac(WIFI_IF_STA, originalMAC);
}

void EvilWireless::restoreOriginalMAC() {
    esp_wifi_set_mac(WIFI_IF_STA, originalMAC);
}

void EvilWireless::setNextWiFiChannel() {
    currentChannel++;
    if (currentChannel > 14) {
        currentChannel = 1;
    }
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
}

void EvilWireless::setRandomMAC_STA() {
    String mac = generateRandomMAC(false);
    uint8_t macArray[6];
    sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &macArray[0], &macArray[1], &macArray[2], &macArray[3], &macArray[4], &macArray[5]);
    esp_wifi_set_mac(WIFI_IF_STA, macArray);
    delay(50);
}

String EvilWireless::getWifiSecurity(int networkIndex) {
    switch (WiFi.encryptionType(networkIndex)) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2_ENTERPRISE";
        default:
            return "Unknown";
    }
}

void EvilWireless::onOffBleSerial() {
    if (bluetoothEnabled) {
        ESP_BT.end();
    } else {
        WiFi.mode(WIFI_OFF);
        WiFi.disconnect(true);
        delay(500);
        esp_wifi_set_promiscuous(false);
        esp_wifi_stop();
        esp_wifi_deinit();
        ESP_BT.begin(BLUETOOTH_NAME);
        ESP_BT.setPin("730303"); // NOT WORKING // WORK ONLY WITH esp v1.0.1 // workaround password in serial
    }
    bluetoothEnabled = !bluetoothEnabled;
}

int EvilWireless::getNumSSID() {
    return numSsid;
}

int EvilWireless::scanAvailableSSID() {
    // Scan for Wifi networks, store all found SSID to ssidList and
    // return number of SSID found.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    unsigned long startTime = millis();
    int n;

    while (millis() - startTime < 2000) {
        n = WiFi.scanNetworks();
        if (n != WIFI_SCAN_RUNNING) break;
    }

    numSsid = min(n, 100);
    for (int i = 0; i < numSsid; i++) {
        ssidList[i] = WiFi.SSID(i);
    }

    return n;
}

void EvilWireless::firstScanWifiNetworks() {
    // Run first scan of available SSID and output
    int n = scanAvailableSSID();

    if (n == 0) {
        sendMessage("No network found ...");
    } else {
        sendMessage(" Near Wifi Networks : ");
        sendMessage("-------------------");
        for (int i = 0; i < numSsid; i++) {
            sendMessage(String(i) + ": " + ssidList[i]);
        }
        sendMessage("-------------------");
    }
}

bool EvilWireless::isNetworkOpen(const String& line) {
    int securityTypeStart = nthIndexOf(line, ',', 1) + 1;
    int securityTypeEnd = nthIndexOf(line, ',', 2);
    String securityType = line.substring(securityTypeStart, securityTypeEnd);
    return securityType.indexOf("[OPEN][ESS]") != -1;
}

String EvilWireless::extractSSID(const String& line) {
    int ssidStart = nthIndexOf(line, ',', 0) + 1;
    int ssidEnd = nthIndexOf(line, ',', 1);
    String ssid = line.substring(ssidStart, ssidEnd);
    return ssid;
}

int EvilWireless::nthIndexOf(const String& str, char toFind, int nth) {
    int found = 0;
    int index = -1;
    while (found <= nth && index < (int) str.length()) {
        index = str.indexOf(toFind, index + 1);
        if (index == -1) break;
        found++;
    }
    return index;
}