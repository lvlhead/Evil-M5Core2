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

#include <vector>

extern "C" {
    #include "esp_wifi.h"
    #include "esp_system.h"
}

#include "evil-monitor.h"

EvilMonitor::EvilMonitor() {
    oldStack = "";
    oldRamUsage = "";
    oldBatteryLevel = "";
    oldTemperature = "";
    lastUpdateTime = 0;
    updateInterval = 1000;
    monitorPage = 1;
}

void EvilMonitor::init() {
}

void EvilMonitor::emptyMonitorCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnASlot, "Next");
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Prev");
    menuItem.getMenu()->displaySoftKey(BtnCSlot, "Esc");
}

void EvilMonitor::clearMenuTextArea() {
    // TODO: Move this to the EvilUI class
    M5.Display.fillRect(0, 0, M5.Display.width(), 200, TFT_LIGHTGREY);
}

void EvilMonitor::showMonitorPage() {
    clearMenuTextArea();
    switch (monitorPage) {
        case 1:
            Page1();
            break;
        case 2:
            Page2();
            break;
        case 3:
            Page3();
            break;
    }
}

void EvilMonitor::nextPage() {
    if (monitorPage == 1) {
        monitorPage = 2;
    } else if (monitorPage == 2) {
        monitorPage = 3;
    } else if (monitorPage == 3) {
        monitorPage = 1;
    }
}

void EvilMonitor::prevPage() {
    if (monitorPage == 1) {
        monitorPage = 3;
    } else if (monitorPage == 2) {
        monitorPage = 1;
    } else if (monitorPage == 3) {
        monitorPage = 2;
    }
}

void EvilMonitor::Page1() {
    //Serial.println("EvilMonitor::Page1");
    oldNumClients = -1;
    oldNumPasswords = -1;

    int newNumClients = WiFi.softAPgetStationNum();
    //int newNumPasswords = countPasswordsInFile();
    int newNumPasswords = 0;

    std::vector<String> messages;
    messages.push_back("SSID: " + clonedSSID);
    messages.push_back("Portal: " + String(isCaptivePortalOn ? "On" : "Off"));
    messages.push_back("Page: " + selectedPortalFile.substring(7));
    messages.push_back("Bluetooth: " + String(bluetoothEnabled ? "On" : "Off"));
    ui.writeVectorMessage(messages, 10, 90);

    M5.Display.setCursor(10, 30);
    M5.Display.println("Clients: " + String(newNumClients));
    oldNumClients = newNumClients;

    M5.Display.setCursor(10, 60);
    M5.Display.println("Passwords: " + String(newNumPasswords));
    oldNumPasswords = newNumPasswords;
}

void EvilMonitor::Page2() {
    //Serial.println("EvilMonitor::Page2");
    updateConnectedMACs();
    std::vector<String> messages;
    if (macAddresses[0] == "") {
        ui.writeSingleMessage("No clients connected");
    } else {
        for (int i = 0; i < 10; i++) {
            //int y = 30 + i * 20;
            //if (y > M5.Display.height() - 20) break;
            messages.push_back(macAddresses[i]);
        }
        ui.writeVectorMessage(messages, 10, 50);
    }
}

void EvilMonitor::Page3() {
    //Serial.println("EvilMonitor::Page3");
    oldStack = getStack();
    oldRamUsage = getRamUsage();
    oldBatteryLevel = getBatteryLevel();
    oldTemperature = getTemperature();

    std::vector<String> messages;
    messages.push_back("Stack left: " + oldStack + " Kb");
    messages.push_back("RAM: " + oldRamUsage + " Mo");
    messages.push_back("Battery: " + oldBatteryLevel + "%");
    messages.push_back("Temperature: " + oldTemperature + "C");
    ui.writeVectorMessage(messages, 10, 30);
}

String EvilMonitor::getMonitoringStatus() {
    String status;
    int numClientsConnected = WiFi.softAPgetStationNum();
    //int numCredentials = countPasswordsInFile();
    int numCredentials = 0;

    status += "Clients: " + String(numClientsConnected) + "\n";
    status += "Credentials: " + String(numCredentials) + "\n";
    status += "SSID: " + String(clonedSSID) + "\n";
    status += "Portal: " + String(isCaptivePortalOn ? "On" : "Off") + "\n";
    status += "Page: " + String(selectedPortalFile.substring(7)) + "\n";
    status += "Bluetooth: " + String(bluetoothEnabled ? "ON" : "OFF") + "\n";
    updateConnectedMACs();
    status += "Connected MACs:\n";
    for (int i = 0; i < 10; i++) {
        if (macAddresses[i] != "") {
            status += macAddresses[i] + "\n";
        }
    }
    status += "Stack left: " + getStack() + " Kb\n";
    status += "RAM: " + getRamUsage() + " Mo\n";
    status += "Battery: " + getBatteryLevel() + "%\n"; // thx to kdv88 to pointing this correction
    status += "Temperature: " + getTemperature() + "C\n";
    return status;
}

void EvilMonitor::updateConnectedMACs() {
    wifi_sta_list_t stationList;
    tcpip_adapter_sta_list_t adapterList;
    esp_wifi_ap_get_sta_list(&stationList);
    tcpip_adapter_get_sta_list(&stationList, &adapterList);

    for (int i = 0; i < adapterList.num; i++) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 adapterList.sta[i].mac[0], adapterList.sta[i].mac[1], adapterList.sta[i].mac[2],
                 adapterList.sta[i].mac[3], adapterList.sta[i].mac[4], adapterList.sta[i].mac[5]);
        macAddresses[i] = String(macStr);
    }
}

String EvilMonitor::getBatteryLevel() {
    return String(M5.Power.getBatteryLevel());
}

String EvilMonitor::getTemperature() {
    float temperature;
    M5.Imu.getTemp(&temperature);
    int roundedTemperature = round(temperature);
    return String(roundedTemperature);
}

String EvilMonitor::getStack() {
    UBaseType_t stackWordsRemaining = uxTaskGetStackHighWaterMark(NULL);
    return String(stackWordsRemaining * 4 / 1024.0);
}

String EvilMonitor::getRamUsage() {
    float heapSizeInMegabytes = esp_get_free_heap_size() / 1048576.0;
    char buffer[10];
    sprintf(buffer, "%.2f", heapSizeInMegabytes);
    return String(buffer);
}


/*
// *** OLD CODE ***
int countPasswordsInFile() {
    File file = SD.open("/credentials.txt");
    if (!file) {
        sendMessage("Error opening credentials file for reading");
        return 0;
    }

    int passwordCount = 0;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.startsWith("-- Password --")) {
            passwordCount++;
        }
    }

    file.close();
    return passwordCount;
}


int oldNumClients = -1;
int oldNumPasswords = -1;
String isBluetoothEnabled;
d
String getMonitoringStatus() {
    String status;
    int numClientsConnected = WiFi.softAPgetStationNum();
    int numCredentials = countPasswordsInFile();

    status += "Clients: " + String(numClientsConnected) + "\n";
    status += "Credentials: " + String(numCredentials) + "\n";
    status += "SSID: " + String(clonedSSID) + "\n";
    status += "Portal: " + String(isCaptivePortalOn ? "On" : "Off") + "\n";
    status += "Page: " + String(selectedPortalFile.substring(7)) + "\n";
    status += "Bluetooth: " + String(bluetoothEnabled ? "ON" : "OFF") + "\n";
    updateConnectedMACs();
    status += "Connected MACs:\n";
    for (int i = 0; i < 10; i++) {
        if (macAddresses[i] != "") {
            status += macAddresses[i] + "\n";
        }
    }
    status += "Stack left: " + getStack() + " Kb\n";
    status += "RAM: " + getRamUsage() + " Mo\n";
    status += "Battery: " + getBatteryLevel() + "%\n"; // thx to kdv88 to pointing this correction
    status += "Temperature: " + getTemperature() + "C\n";
    return status;
}

void displayMonitorPage1() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE);

    M5.Display.setCursor(10, 90);
    M5.Display.println("SSID: " + clonedSSID);
    M5.Display.setCursor(10, 120);
    M5.Display.println("Portal: " + String(isCaptivePortalOn ? "On" : "Off"));
    M5.Display.setCursor(10, 150);
    M5.Display.println("Page: " + selectedPortalFile.substring(7));

    if (bluetoothEnabled) {
        isBluetoothEnabled = "ON";
    } else {
        isBluetoothEnabled = "OFF";
    }
    M5.Display.setCursor(10, 180);
    M5.Display.println("Bluetooth: " + isBluetoothEnabled);

    oldNumClients = -1;
    oldNumPasswords = -1;

    M5.Display.display();

    while (!inMenu) {
        M5.update();
        handleDnsRequestSerial();
        server.handleClient();

        int newNumClients = WiFi.softAPgetStationNum();
        int newNumPasswords = countPasswordsInFile();

        if (newNumClients != oldNumClients) {
            M5.Display.fillRect(10, 30, 200, 20, TFT_BLACK);
            M5.Display.setCursor(10, 30);
            M5.Display.println("Clients: " + String(newNumClients));
            oldNumClients = newNumClients;
        }

        if (newNumPasswords != oldNumPasswords) {
            M5.Display.fillRect(10, 60, 200, 20, TFT_BLACK);
            M5.Display.setCursor(10, 60);
            M5.Display.println("Passwords: " + String(newNumPasswords));
            oldNumPasswords = newNumPasswords;
        }

        if (M5.BtnA.wasPressed()) {
            displayMonitorPage3();
            break;
        } else if (M5.BtnC.wasPressed()) {
            displayMonitorPage2();
            break;
        } else if (M5.BtnB.wasPressed()) {
            inMenu = true;
            drawMenu();
            break;
        }

        delay(100);
    }
}

void updateConnectedMACs() {
    wifi_sta_list_t stationList;
    tcpip_adapter_sta_list_t adapterList;
    esp_wifi_ap_get_sta_list(&stationList);
    tcpip_adapter_get_sta_list(&stationList, &adapterList);

    for (int i = 0; i < adapterList.num; i++) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 adapterList.sta[i].mac[0], adapterList.sta[i].mac[1], adapterList.sta[i].mac[2],
                 adapterList.sta[i].mac[3], adapterList.sta[i].mac[4], adapterList.sta[i].mac[5]);
        macAddresses[i] = String(macStr);
    }
}

void displayMonitorPage2() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    updateConnectedMACs();
    if (macAddresses[0] == "") {
        M5.Display.setCursor(10, 30);
        M5.Display.println("No client connected");
        sendMessage("----Mac-Address----");
        sendMessage("No client connected");
        sendMessage("-------------------");
    } else {
        sendMessage("----Mac-Address----");
        for (int i = 0; i < 10; i++) {
            int y = 30 + i * 20;
            if (y > M5.Display.height() - 20) break;

            M5.Display.setCursor(10, y);
            M5.Display.println(macAddresses[i]);
            sendMessage(macAddresses[i]);
        }
        sendMessage("-------------------");
    }


    M5.Display.display();

    while (!inMenu) {
        M5.update();
        handleDnsRequestSerial();
        if (M5.BtnA.wasPressed()) {
            displayMonitorPage1();
            break;
        } else if (M5.BtnC.wasPressed()) {
            displayMonitorPage3();
            break;
        } else if (M5.BtnB.wasPressed()) {
            inMenu = true;
            drawMenu();
            break;
        }
    }
}

String oldStack = "";
String oldRamUsage = "";
String oldBatteryLevel = "";
String oldTemperature = "";

String getBatteryLevel() {
    return String(M5.Power.getBatteryLevel());
}

String getTemperature() {
    float temperature;
    M5.Imu.getTemp(&temperature);
    int roundedTemperature = round(temperature);
    return String(roundedTemperature);
}

String getStack() {
    UBaseType_t stackWordsRemaining = uxTaskGetStackHighWaterMark(NULL);
    return String(stackWordsRemaining * 4 / 1024.0);
}

String getRamUsage() {
    float heapSizeInMegabytes = esp_get_free_heap_size() / 1048576.0;
    char buffer[10];
    sprintf(buffer, "%.2f", heapSizeInMegabytes);
    return String(buffer);
}

unsigned long lastUpdateTime = 0;
const long updateInterval = 1000;

void displayMonitorPage3() {
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE);

    oldStack = getStack();
    oldRamUsage = getRamUsage();
    oldBatteryLevel = getBatteryLevel();
    oldTemperature = getTemperature();

    M5.Display.setCursor(10, 30);
    M5.Display.println("Stack left: " + oldStack + " Kb");
    M5.Display.setCursor(10, 60);
    M5.Display.println("RAM: " + oldRamUsage + " Mo");
    M5.Display.setCursor(10, 90);
    M5.Display.println("Battery: " + oldBatteryLevel + "%"); // thx to kdv88 to pointing mistranlastion
    M5.Display.setCursor(10, 120);
    M5.Display.println("Temperature: " + oldTemperature + "C");

    M5.Display.display();

    lastUpdateTime = millis();

    oldStack = "";
    oldRamUsage = "";
    oldBatteryLevel = "";
    oldTemperature = "";

    M5.Display.display();

    while (!inMenu) {
        M5.update();
        handleDnsRequestSerial();

        unsigned long currentMillis = millis();


        if (currentMillis - lastUpdateTime >= updateInterval) {
            String newStack = getStack();
            String newRamUsage = getRamUsage();
            String newBatteryLevel = getBatteryLevel();
            String newTemperature = getTemperature();

            if (newStack != oldStack) {
                M5.Display.fillRect(10, 30, 200, 20, TFT_BLACK);
                M5.Display.setCursor(10, 30);
                M5.Display.println("Stack left: " + newStack + " Kb");
                oldStack = newStack;
            }

            if (newRamUsage != oldRamUsage) {
                M5.Display.fillRect(10, 60, 200, 20, TFT_BLACK);
                M5.Display.setCursor(10, 60);
                M5.Display.println("RAM: " + newRamUsage + " Mo");
                oldRamUsage = newRamUsage;
            }

            if (newBatteryLevel != oldBatteryLevel) {
                M5.Display.fillRect(10, 90, 200, 20, TFT_BLACK);
                M5.Display.setCursor(10, 90);
                M5.Display.println("Battery: " + newBatteryLevel + "%");// thx to kdv88 to pointing mistranlastion
                oldBatteryLevel = newBatteryLevel;
            }

            if (newTemperature != oldTemperature) {
                M5.Display.fillRect(10, 120, 200, 20, TFT_BLACK);
                M5.Display.setCursor(10, 120);
                M5.Display.println("Temperature: " + newTemperature + "C");
                oldTemperature = newTemperature;
            }

            lastUpdateTime = currentMillis;
        }

        if (M5.BtnA.wasPressed()) {
            displayMonitorPage2();
            break;
        } else if (M5.BtnC.wasPressed()) {
            displayMonitorPage1();
            break;
        } else if (M5.BtnB.wasPressed()) {
            inMenu = true;
            drawMenu();
            break;
        }

        delay(100);
    }
}
*/
