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

#ifndef EVILDETECTOR_H
#define EVILDETECTOR_H

#include <set>

#include <M5StackMenuSystem.h>
#include <M5Unified.h>
#include <ArduinoJson.h>

#include "evil-ui.h"
#include "evil-util.h"
#include "evil-wireless.h"

// C Functions to support esp_wifi callback
extern "C" void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
extern "C" bool isAnEAPOLPacket(const wifi_promiscuous_pkt_t* packet);
extern "C" void saveToPCAPFile(const wifi_promiscuous_pkt_t* packet, bool haveBeacon);
extern "C" void writePCAPHeader(File &file);
extern "C" String convMACToStr(const uint8_t* addr);
extern unsigned int packetCount;  // Number of packets received
extern int nombreDeHandshakes; // Nombre de handshakes/PMKID capturés
extern int nombreDeDeauth;
extern int nombreDeEAPOL;
extern std::set<String> registeredBeacons;

// UI Variables
extern bool detectedDeauth;
extern String deauthRssi, deauthChannel, deauthAddr1, deauthAddr2;
extern bool detectedPwnagotchi;
extern String pwnagotchiName;
extern String pwnagotchiPwnd;
extern bool detectedEAPOL;

// Deauther esp_wifi callback
extern "C" void snifferCallbackDeauth(void* buf, wifi_promiscuous_pkt_type_t type);
extern "C" void sendDeauthPacket();
extern "C" void updateMacAddresses(uint8_t* bssid);

// Deauther Attack Variables
extern uint8_t source_mac[];   // MAC source
extern uint8_t receiver_mac[]; // MAC client - brodcast - should not work on some device that need to be mac spoofed
extern uint8_t ap_mac[];   // MAC access point
extern const uint8_t deauth_frame_default[];
extern String ssid;
extern uint8_t* bssid;

class EvilDetector {
  public:
    EvilDetector();
    void init();
    void showDetectorApp();
    void closeDetectorApp();
    void showDeautherApp();
    void closeDeautherApp();
    static void emptyDetectorCallback(CallbackMenuItem& menuItem);
    static void emptyDeautherCallback(CallbackMenuItem& menuItem);
    void toggleAppRunning();
    void toggleAutoChannelHop();
    bool getAutoChannelHop();
    void incrementChannel(int count);
    void toggleCfgSniffEAPOL();
    bool getCfgSniffEAPOL();

  private:
    EvilUI ui;
    EvilWireless wireless;
    bool isAppRunning;
    bool updateScreen;
    bool autoChannelHop; // Starts in auto mode
    int deauthCount;
    int currentChannelDeauth;
    int channel;
    String ssid;
    String macAddress;
    String channelType;
    bool lastDisplayedMode; // Initialize to the opposite to force the first update
    int maxChannelScanning;
    File pcapFile;
    bool cfg_sniffEAPOL;
};

#endif
