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

#include "evil-detector.h"

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

EvilDetector::EvilDetector() {
    channelHopInterval = 5000;
    lastChannelHopTime = 0;
    currentChannelDeauth = 1;
    autoChannelHop = true; // Starts in auto mode
    lastDisplayedChannelDeauth = -1;
    lastDisplayedMode = !autoChannelHop; // Initialize to the opposite to force the first update
    lastScreenClearTime = 0; // To track the last screen clear
    maxChannelScanning = 13;
    nombreDeHandshakes = 0; // Nombre de handshakes/PMKID capturés
    nombreDeDeauth = 0;
    nombreDeEAPOL = 0;
    haveBeacon = false;
    lastTime = 0;  // Last time update
    packetCount = 0;  // Number of packets received
}

void EvilDetector::init() {
}

void EvilDetector::emptyDetectorCallback(CallbackMenuItem& menuItem) {
    M5.Display.clear();
    menuItem.getMenu()->displaySoftKey(BtnASlot, "Mode");
    menuItem.getMenu()->displaySoftKey(BtnBSlot, "Exit");
    menuItem.getMenu()->displaySoftKey(BtnCSlot, "Next");
}

void EvilDetector::showDetectorApp() {
    // Set up application for first run
    if (!isAppRunning) {
        wireless.ESP_BT.end();
        bluetoothEnabled = false;
        esp_wifi_set_promiscuous(false);
        esp_wifi_stop();
        esp_wifi_set_promiscuous_rx_cb(NULL);
        esp_wifi_deinit();
        delay(300); //petite pause
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        esp_wifi_start();
        WiFi.mode(WIFI_STA);
        esp_wifi_start();
        esp_wifi_set_promiscuous(true);
        //esp_wifi_set_promiscuous_rx_cb(snifferCallback);
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
    if (ui.clearScreenDelay()) {
        deauthDetect();
    }
}

void EvilDetector::closeApp() {
    esp_wifi_set_promiscuous(false);
    toggleAutoChannelHop();
    ui.waitAndReturnToMenu("Stop detection...");
}

void EvilDetector::toggleAppRunning() {
    isAppRunning = !isAppRunning;
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
    String channelType = autoChannelHop ? "Auto" : "Static";
    sendMessage(channelType + " Channel : " + String(currentChannelDeauth));
}

void EvilDetector::deauthDetect() {
        if (autoChannelHop) {
            unsigned long currentTime = millis();
            if (currentTime - lastChannelHopTime > channelHopInterval) {
                lastChannelHopTime = currentTime;
                incrementChannel(1);
            }
        }

        if (currentChannelDeauth != lastDisplayedChannelDeauth || autoChannelHop != lastDisplayedMode) {
            ui.writeMessageXY("Channel: " + currentChannelDeauth, 0, 16, false);
            lastDisplayedChannelDeauth = currentChannelDeauth;
        }

        if (autoChannelHop != lastDisplayedMode) {
            String channelType = autoChannelHop ? "Auto" : "Static";
            ui.writeMessageXY("Mode: " + channelType, 0, 37, false);
            lastDisplayedMode = autoChannelHop;
        }
}

bool EvilDetector::estUnPaquetEAPOL(const wifi_promiscuous_pkt_t* packet) {
    const uint8_t *payload = packet->payload;
    int len = packet->rx_ctrl.sig_len;

    // length check to ensure packet is large enough for EAPOL (minimum length)
    if (len < (24 + 8 + 4)) { // 24 bytes for the MAC header, 8 for LLC/SNAP, 4 for EAPOL minimum
        return false;
    }

    // check for LLC/SNAP header indicating EAPOL payload
    // LLC: AA-AA-03, SNAP: 00-00-00-88-8E for EAPOL
    if (payload[24] == 0xAA && payload[25] == 0xAA && payload[26] == 0x03 &&
        payload[27] == 0x00 && payload[28] == 0x00 && payload[29] == 0x00 &&
        payload[30] == 0x88 && payload[31] == 0x8E) {
        return true;
    }

    // handle QoS tagging which shifts the start of the LLC/SNAP headers by 2 bytes
    // check if the frame control field's subtype indicates a QoS data subtype (0x08)
    if ((payload[0] & 0x0F) == 0x08) {
        // Adjust for the QoS Control field and recheck for LLC/SNAP header
        if (payload[26] == 0xAA && payload[27] == 0xAA && payload[28] == 0x03 &&
            payload[29] == 0x00 && payload[30] == 0x00 && payload[31] == 0x00 &&
            payload[32] == 0x88 && payload[33] == 0x8E) {
            return true;
        }
    }

    return false;
}


void EvilDetector::ecrireEntetePCAP(File &file) {
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

void EvilDetector::enregistrerDansFichierPCAP(const wifi_promiscuous_pkt_t* packet, bool haveBeacon) {
    // Construire le nom du fichier en utilisant les adresses MAC de l'AP et du client
    const uint8_t *addr1 = packet->payload + 4;  // Adresse du destinataire (Adresse 1)
    const uint8_t *addr2 = packet->payload + 10; // Adresse de l'expéditeur (Adresse 2)
    const uint8_t *bssid = packet->payload + 16; // Adresse BSSID (Adresse 3)
    const uint8_t *apAddr;

    if (memcmp(addr1, bssid, 6) == 0) {
        apAddr = addr1;
    } else {
        apAddr = addr2;
    }

    char nomFichier[50];
    sprintf(nomFichier, "/handshakes/HS_%02X%02X%02X%02X%02X%02X.pcap",
    apAddr[0], apAddr[1], apAddr[2], apAddr[3], apAddr[4], apAddr[5]);

    // Vérifier si le fichier existe déjà
    bool fichierExiste = SD.exists(nomFichier);

    // Si probe est true et que le fichier n'existe pas, ignorer l'enregistrement
    if (haveBeacon && !fichierExiste) {
        return;
    }

    // Ouvrir le fichier en mode ajout si existant sinon en mode écriture
    File fichierPcap = SD.open(nomFichier, fichierExiste ? FILE_APPEND : FILE_WRITE);
    if (!fichierPcap) {
        Serial.println("Échec de l'ouverture du fichier PCAP");
        return;
    }

    if (!haveBeacon && !fichierExiste) {
        Serial.println("Écriture de l'en-tête global du fichier PCAP");
        ecrireEntetePCAP(fichierPcap);
    }

    if (haveBeacon && fichierExiste) {
        String bssidStr = String((char*)apAddr, 6);
        if (registeredBeacons.find(bssidStr) != registeredBeacons.end()) {
            return; // Beacon déjà enregistré pour ce BSSID
        }
        registeredBeacons.insert(bssidStr); // Ajouter le BSSID à l'ensemble
    }

    // Écrire l'en-tête du paquet et le paquet lui-même dans le fichier
    pcaprec_hdr_t pcap_packet_header;
    pcap_packet_header.ts_sec = packet->rx_ctrl.timestamp / 1000000;
    pcap_packet_header.ts_usec = packet->rx_ctrl.timestamp % 1000000;
    pcap_packet_header.incl_len = packet->rx_ctrl.sig_len;
    pcap_packet_header.orig_len = packet->rx_ctrl.sig_len;
    fichierPcap.write((const byte*)&pcap_packet_header, sizeof(pcaprec_hdr_t));
    fichierPcap.write(packet->payload, packet->rx_ctrl.sig_len);
    fichierPcap.close();
}

void EvilDetector::displayPwnagotchiDetails(const String& name, const String& pwndnb) {
    // Construire le texte à afficher
    String displayText = "Pwnagotchi: " + name + "      \npwnd: " + pwndnb + "   ";

    // Préparer l'affichage
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(0, 170);

    // Afficher les informations
    M5.Lcd.println(displayText);
}

void EvilDetector::printAddress(const uint8_t* addr) {
    for(int i = 0; i < 6; i++) {
        Serial.printf("%02X", addr[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
}

void EvilDetector::printAddressLCD(const uint8_t* addr) {
    // Utiliser sprintf pour formater toute l'adresse MAC en une fois
    sprintf(macBuffer, "%02X:%02X:%02X:%02X:%02X:%02X",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // Afficher l'adresse MAC
    M5.Lcd.print(macBuffer);
}

void EvilDetector::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    packetCount++;
    unsigned long currentTime = millis();
    if (currentTime - lastTime >= 1000) {
        if (packetCount < 100) {
            M5.Lcd.setCursor(224, 0);
        } else {
            M5.Lcd.setCursor(212, 0);
        }
        M5.Lcd.printf(" PPS:%d ", packetCount);

        lastTime = currentTime;
        packetCount = 0;
    }

    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
    const uint8_t *frame = pkt->payload;
    const uint16_t frameControl = (uint16_t)frame[0] | ((uint16_t)frame[1] << 8);

    // Extraire le type et le sous-type de la trame
    const uint8_t frameType = (frameControl & 0x0C) >> 2;
    const uint8_t frameSubType = (frameControl & 0xF0) >> 4;

    if (estUnPaquetEAPOL(pkt)) {
        Serial.println("EAPOL Detected !!!!");
        // Extraire les adresses MAC
        const uint8_t *receiverAddr = frame + 4;  // Adresse 1
        const uint8_t *senderAddr = frame + 10;  // Adresse 2
        // Affichage sur le port série
        Serial.print("Address MAC destination: ");
        printAddress(receiverAddr);
        Serial.print("Address MAC expedition: ");
        printAddress(senderAddr);

        enregistrerDansFichierPCAP(pkt, false);
        nombreDeEAPOL++;
        M5.Lcd.setCursor(260, 18);
        M5.Lcd.printf("H:");
        M5.Lcd.print(nombreDeHandshakes);
        if (nombreDeEAPOL < 999) {
            M5.Lcd.setCursor(212, 36);
        } else {
            M5.Lcd.setCursor(202, 36);
        }
        M5.Lcd.printf("EAPOL:");
        M5.Lcd.print(nombreDeEAPOL);
    }

    if (frameType == 0x00 && frameSubType == 0x08) {
        const uint8_t *senderAddr = frame + 10; // Adresse source dans la trame beacon

        // Convertir l'adresse MAC en chaîne de caractères pour la comparaison
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        senderAddr[0], senderAddr[1], senderAddr[2], senderAddr[3], senderAddr[4], senderAddr[5]);

        if (strcmp(macStr, "DE:AD:BE:EF:DE:AD") == 0) {
            sendMessage("-------------------");
            sendMessage("Pwnagotchi Detected !!!");
            sendMessage("CH: " + String(ctrl.channel));
            sendMessage("RSSI: " + String(ctrl.rssi));
            sendMessage("MAC: " + String(macStr));
            sendMessage("-------------------");

            String essid = ""; // Préparer la chaîne pour l'ESSID
            int essidMaxLength = 700; // longueur max
            for (int i = 0; i < essidMaxLength; i++) {
                if (frame[i + 38] == '\0') break; // Fin de l'ESSID

                if (isAscii(frame[i + 38])) {
                    essid.concat((char)frame[i + 38]);
                }
            }

            int jsonStart = essid.indexOf('{');
            if (jsonStart != -1) {
                String cleanJson = essid.substring(jsonStart); // Nettoyer le JSON

                DynamicJsonDocument json(4096); // Augmenter la taille pour l'analyse
                DeserializationError error = deserializeJson(json, cleanJson);

                if (!error) {
                    sendMessage("Successfully parsed json");
                    String name = json["name"].as<String>(); // Extraire le nom
                    String pwndnb = json["pwnd_tot"].as<String>(); // Extraire le nombre de réseaux pwned
                    sendMessage("Name: " + name); // Afficher le nom
                    sendMessage("pwnd: " + pwndnb); // Afficher le nombre de réseaux pwned

                    // affichage
                    displayPwnagotchiDetails(name, pwndnb);
                } else {
                    sendMessage("Could not parse Pwnagotchi json");
                }
            } else {
                sendMessage("JSON start not found in ESSID");
            }
        } else {
            pkt->rx_ctrl.sig_len -= 4;  // Réduire la longueur du signal de 4 bytes
            enregistrerDansFichierPCAP(pkt, true);  // Enregistrer le paquet
        }
    }

    // Vérifier si c'est un paquet de désauthentification
    if (frameType == 0x00 && frameSubType == 0x0C) {
        // Extraire les adresses MAC
        const uint8_t *receiverAddr = frame + 4;  // Adresse 1
        const uint8_t *senderAddr = frame + 10;  // Adresse 2
        // Affichage sur le port série
        Serial.println("-------------------");
        Serial.println("Deauth Packet detected !!! :");
        Serial.print("CH: ");
        Serial.println(ctrl.channel);
        Serial.print("RSSI: ");
        Serial.println(ctrl.rssi);
        Serial.print("Sender: "); printAddress(senderAddr);
        Serial.print("Receiver: "); printAddress(receiverAddr);
        Serial.println();

        // Affichage sur l'écran
        M5.Lcd.setTextColor(WHITE, BLACK);
        M5.Lcd.setCursor(0, 64);
        M5.Lcd.printf("Deauth Detected!");
        M5.Lcd.setCursor(0, 85);
        M5.Lcd.printf("CH: %d RSSI: %d  ", ctrl.channel, ctrl.rssi);
        M5.Lcd.setCursor(0, 106);
        M5.Lcd.print("Send: "); printAddressLCD(senderAddr);
        M5.Lcd.setCursor(0, 127);
        M5.Lcd.print("Receive: "); printAddressLCD(receiverAddr);
        nombreDeDeauth++;
        if (nombreDeDeauth < 999) {
            M5.Lcd.setCursor(200, 54);
        } else {
            M5.Lcd.setCursor(188, 54);
        }
        M5.Lcd.printf("DEAUTH:");
        M5.Lcd.print(nombreDeDeauth);
    }
}
