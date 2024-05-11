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

#include "evil-wardriving.h"

EvilWardrive::EvilWardrive() {
    isAppRunning = false;
    scanStarted = false;
    gpsDataAvailable = false;
    maxIndex = 0;
    // Déclaration des variables pour la latitude, la longitude et l'altitude
    lat = 0.0;
    lng = 0.0;
    alt = 0.0;
    accuracy = 0.0; // Déclaration de la variable pour la précision
}

void EvilWardrive::init() {
}

void EvilWardrive::emptyWardriveCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Stop");
}

void EvilWardrive::showWardriveApp() {
    // Set up application for first run
    if (!isAppRunning) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        sendMessage("-------------------");
        sendMessage("Starting Wardriving");
        sendMessage("-------------------");

        // TODO: What does the following code even do for us?
        // delay(1000);
        if (!SD.exists("/wardriving")) {
            SD.mkdir("/wardriving");
        }

/*
        while (true) {
            fileRoot = openFile("/wardriving", FILE_READ);
            File entry = fileRoot.openNextFile();
            if (!entry) break;
            String name = entry.name();
            int startIndex = name.indexOf('-') + 1;
            int endIndex = name.indexOf('.');
            if (startIndex > 0 && endIndex > startIndex) {
                int fileIndex = name.substring(startIndex, endIndex).toInt();
                if (fileIndex > maxIndex) {
                    maxIndex = fileIndex;
                }
            }
            entry.close();
        }   // while true
*/

        toggleAppRunning();
    }   // if !isAppRunning

    // Run the app
    if (ui.clearScreenDelay()) {
        wardrivingMode();
    }
}

void EvilWardrive::closeWardriveApp() {
    if (ui.confirmPopup("List Open Networks?", true)) {
        createKarmaList(maxIndex);
    }
    ui.waitAndReturnToMenu("Stopping Wardriving.");
    sendMessage("-------------------");
    sendMessage("Stopping Wardriving");
    sendMessage("-------------------");
    sendMessage("-------------------");
    sendMessage("Session Saved.");
    sendMessage("-------------------");
}

void EvilWardrive::toggleAppRunning() {
    isAppRunning = !isAppRunning;
}

void EvilWardrive::wardrivingMode() {
    int maxIndex = 0;

    std::vector<String> messages;
    messages.push_back("Scanning...");
    messages.push_back("No GPS Data");
    ui.writeVectorMessage(messages, 0, 10, 30);

    if (!scanStarted) {
        WiFi.scanNetworks(true, true);
        scanStarted = true;
    }

#if GPS_ENABLED
    while (Serial2.available() > 0 && !gpsDataAvailable) {
        if (gps.encode(Serial2.read())) {
            if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
                lat = gps.location.lat();
                lng = gps.location.lng();
                alt = gps.altitude.meters();
                accuracy = gps.hdop.value();
                gpsDataAvailable = true;

                // Affichage des informations GPS sur l'écran
                ui.writeMessageXY("Latitude:", 0, 40, false);
                ui.writeMessageXY(String(gps.location.lat(), 6), 0, 60, false);

                ui.writeMessageXY("Longitude:", 170, 40, false);
                ui.writeMessageXY(String(gps.location.lng(), 6), 170, 60, false);

                ui.writeMessageXY("Satellites:", 0, 90, false);
                ui.writeMessageXY(String(gps.satellites.value()), 0, 110, false);

                // Altitude
                ui.writeMessageXY("Altitude:", 170, 90, false);
                ui.writeMessageXY(String(gps.altitude.meters(), 2) + "m", 170, 110, false);

                // Date et Heure
                ui.writeMessageXY("Date/Time:", 0, 140, false);
                ui.writeMessageXY(String(formatTimeFromGPS()), 0, 160, false);
            }
        }
    }
#endif

    int n = WiFi.scanComplete();
    if (n > -1) {
        String currentTime = formatTimeFromGPS();
        String wifiData = "\n";
        for (int i = 0; i < n; ++i) {
            String line = WiFi.BSSIDstr(i) + ",";
            line += WiFi.SSID(i) + ",";
            line += getCapabilities(WiFi.encryptionType(i)) + ",";
            line += currentTime + ",";
            line += String(WiFi.channel(i)) + ",";
            line += String(WiFi.RSSI(i)) + ",";
            line += String(lat, 6) + "," + String(lng, 6) + ",";
            line += String(alt) + "," + String(accuracy) + ",";
            line += "WIFI";
            wifiData += line + "\n";
        }

        sendMessage("----------------------------------------------------");
        sendMessage("WiFi Networks: " + String(n));
        sendMessage(wifiData);
        sendMessage("----------------------------------------------------");

        String fileName = "/wardriving/wardriving-0" + String(maxIndex + 1) + ".csv";

        // Ouvrir le fichier en mode lecture pour vérifier s'il existe et sa taille
        File file = openFile(fileName, FILE_READ);
        bool isNewFile = !file || file.size() == 0;
        if (file) {
            file.close();
        }

        file = openFile(fileName, isNewFile ? FILE_WRITE : FILE_APPEND);

        if (file) {
            if (isNewFile) {
                file.println(createPreHeader());
                file.println(createHeader());
            }
            file.print(wifiData);
            file.close();
        }
    }

    scanStarted = false;
    ui.writeMessageXY("Near Wifi: " + n, 0, 10, false);
}

String EvilWardrive::formatTimeFromGPS() {
    if (gps.time.isValid() && gps.date.isValid()) {
        char dateTime[30];
        sprintf(dateTime, "%04d-%02d-%02d %02d:%02d:%02d", gps.date.year(), gps.date.month(), gps.date.day(),
                                                           gps.time.hour(), gps.time.minute(), gps.time.second());
        return String(dateTime);
    } else {
        return "0000-00-00 00:00:00";
    }
}

String EvilWardrive::getCapabilities(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN: return "[OPEN][ESS]";
        case WIFI_AUTH_WEP: return "[WEP][ESS]";
        case WIFI_AUTH_WPA_PSK: return "[WPA-PSK][ESS]";
        case WIFI_AUTH_WPA2_PSK: return "[WPA2-PSK][ESS]";
        case WIFI_AUTH_WPA_WPA2_PSK: return "[WPA-WPA2-PSK][ESS]";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "[WPA2-ENTERPRISE][ESS]";
        default: return "[UNKNOWN]";
    }
}

void EvilWardrive::createKarmaList(int maxIndex) {
    std::set<std::string> uniqueSSIDs;
    // Lire le contenu existant de KarmaList.txt et l'ajouter au set
    File karmaListRead = openFile("/KarmaList.txt", FILE_READ);
    if (karmaListRead) {
        while (karmaListRead.available()) {
            String ssid = karmaListRead.readStringUntil('\n');
            ssid.trim();
            if (ssid.length() > 0) {
                uniqueSSIDs.insert(ssid.c_str());
            }
        }
        karmaListRead.close();
    }

    File file = openFile("/wardriving/wardriving-0" + String(maxIndex + 1) + ".csv", FILE_READ);

    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (wireless.isNetworkOpen(line)) {
            String ssid = wireless.extractSSID(line);
            uniqueSSIDs.insert(ssid.c_str());
        }
    }
    file.close();

    // Écrire le set dans KarmaList.txt
    File karmaListWrite = openFile("/KarmaList.txt", FILE_WRITE);

    sendMessage("Writing to KarmaList.txt");
    for (const auto& ssid : uniqueSSIDs) {
        karmaListWrite.println(ssid.c_str());
        sendMessage("Writing SSID: " + String(ssid.c_str()));
    }
}


String EvilWardrive::createPreHeader() {
    String preHeader = "WigleWifi-1.4";
    preHeader += ",appRelease=" + String(APP_VERSION); // Remplacez [version] par la version de votre application
    preHeader += ",model=Core2";
    preHeader += ",release=" + String(APP_VERSION); // Remplacez [release] par la version de l'OS de l'appareil
    preHeader += ",device=" + String(APP_NAME); // Remplacez [device name] par un nom de périphérique, si souhaité
    preHeader += ",display=lvlhead/7h30th3r0n3"; // Ajoutez les caractéristiques d'affichage, si pertinent
    preHeader += ",board=M5Stack Core2";
    preHeader += ",brand=M5Stack";
    return preHeader;
}

String EvilWardrive::createHeader() {
    return "MAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type";
}
