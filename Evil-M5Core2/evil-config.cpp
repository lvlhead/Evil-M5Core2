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

#include "evil-config.h"

EvilConfig::EvilConfig() {
    configFolderPath = "/config";
    configFilePath = "/config/config.txt";
    defaultBrightness = 255 * 0.35; //  35% default Brightness
}

void EvilConfig::restoreConfigParameter(String key) {
    if (SD.exists(configFilePath)) {
        File configFile = SD.open(configFilePath, FILE_READ);
        if (configFile) {
            String line;
            int value = defaultBrightness;
            while (configFile.available()) {
                line = configFile.readStringUntil('\n');
                if (line.startsWith(key + "=")) {
                    value = line.substring(line.indexOf('=') + 1).toInt();
                    break;
                }
            }
            configFile.close();
            if (key == "brightness") {
                M5.Display.setBrightness(value);
                //sendMessage("Brightness restored to " + String(value));
            }
        } else {
            //sendMessage("Error when opening config.txt");
        }
    } else {
        //sendMessage("Config file not found, using default value");
        if (key == "brightness") {
            M5.Display.setBrightness(defaultBrightness);
        }
    }
}

bool EvilConfig::saveConfigParameter(String key, int value) {
    if (!SD.exists(configFolderPath)) {
        SD.mkdir(configFolderPath);
    }

    String content = "";
    File configFile = SD.open(configFilePath, FILE_READ);
    if (configFile) {
        while (configFile.available()) {
            content += configFile.readStringUntil('\n') + '\n';
        }
        configFile.close();
    } else {
        //sendMessage("Error when opening config.txt for reading");
        return false;
    }

    int startPos = content.indexOf(key + "=");
    if (startPos != -1) {
        int endPos = content.indexOf('\n', startPos);
        String oldValue = content.substring(startPos, endPos);
        content.replace(oldValue, key + "=" + String(value));
    } else {
        content += key + "=" + String(value) + "\n";
    }

    configFile = SD.open(configFilePath, FILE_WRITE);
    if (configFile) {
        configFile.print(content);
        configFile.close();
        //sendMessage(key + " saved!");
        return true;
    } else {
        //sendMessage("Error when opening config.txt for writing");
        return false;
    }
}

bool EvilConfig::readConfigFile() {
    whitelist.clear();
    File configFile = SD.open(configFilePath);
    if (!configFile) {
        //sendMessage("Failed to open config file");
        return false;
    }

    while (configFile.available()) {
        String line = configFile.readStringUntil('\n');
        if (line.startsWith("KarmaAutoWhitelist=")) {
            int startIndex = line.indexOf('=') + 1;
            String ssidList = line.substring(startIndex);
            if (!ssidList.length()) {
                break;
            }
            int lastIndex = 0, nextIndex;
            while ((nextIndex = ssidList.indexOf(',', lastIndex)) != -1) {
                whitelist.push_back(ssidList.substring(lastIndex, nextIndex).c_str());
                lastIndex = nextIndex + 1;
            }
            whitelist.push_back(ssidList.substring(lastIndex).c_str());
        }
    }
    configFile.close();
    return true;
}

std::vector<String> EvilConfig::readCustomProbes() {
    File file = SD.open(configFilePath, FILE_READ);
    std::vector<String> customProbes;

    if (!file) {
        //sendMessage("Failed to open file for reading");
        return customProbes;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.startsWith("CustomProbes=")) {
            String probesStr = line.substring(String("CustomProbes=").length());
            int idx = 0;
            while ((idx = probesStr.indexOf(',')) != -1) {
                customProbes.push_back(probesStr.substring(0, idx));
                probesStr = probesStr.substring(idx + 1);
            }
            if (probesStr.length() > 0) {
                customProbes.push_back(probesStr);
            }
            break;
        }
    }
    file.close();
    return customProbes;
}

bool EvilConfig::isSSIDWhitelisted(const char* ssid) {
    for (const auto& wssid : whitelist) {
        if (wssid == ssid) {
            return true;
        }
    }
    return false;
}

void EvilConfig::drawImage(const char *filepath) {
    fs::File file = SD.open(filepath);
    M5.Display.drawJpgFile(SD, filepath);
    file.close();
}
