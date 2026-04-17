#include "SmbComms.h"
#include <WiFi.h>
#include <Preferences.h>
#include "esp_wifi.h"

#define NVS_SMB_NS "smb"

SmbComms* SmbComms::_instance = nullptr;

// ============================================================
SmbComms::SmbComms() {
    _instance = this;
}

// ============================================================
void SmbComms::begin() {
    loadFromNVS();

    if (esp_now_init() != ESP_OK) {
        Serial.println("SMB ESP-NOW: init FAILED");
        return;
    }
    esp_now_register_recv_cb(SmbComms::onReceive);
    Serial.println("SMB ESP-NOW: Initialized");

    if (_hasPairedMac) {
        if (registerPeer(_smbMac)) {
            _peerRegistered = true;
            _state = SmbPairingState::PAIRED;
            Serial.printf("SMB ESP-NOW: Auto-paired (NVS restore) SMB=%02X:%02X:%02X:%02X:%02X:%02X\n",
                          _smbMac[0], _smbMac[1], _smbMac[2],
                          _smbMac[3], _smbMac[4], _smbMac[5]);
            // Push current config immediately so SMB gets setpoint/invert after reboot
            sendConfig(_setpoint, _invert);
        } else {
            Serial.println("SMB ESP-NOW: Peer registration failed — idle (pair again via web UI)");
        }
    } else {
        Serial.println("SMB ESP-NOW: No stored pairing — use web UI to pair");
    }
}

// ============================================================
void SmbComms::update() {
    if (_state != SmbPairingState::BROADCASTING) return;

    unsigned long now = millis();

    // Abort after timeout
    if (now - _pairingStartTime >= PAIRING_TIMEOUT_MS) {
        Serial.println("SMB ESP-NOW: Pairing broadcast timed out");
        _state = SmbPairingState::IDLE;
        return;
    }

    // Send a beacon every 500ms
    if (now - _lastBeaconTime >= BEACON_INTERVAL_MS) {
        PktPairingBeacon pkt;
        pkt.type    = PKT_PAIRING_BEACON;
        pkt.channel = getCurrentChannel();
        WiFi.macAddress(pkt.mac);

        // Broadcast address
        const uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        // Ensure broadcast peer exists
        if (!esp_now_is_peer_exist(broadcast)) {
            esp_now_peer_info_t peer = {};
            memcpy(peer.peer_addr, broadcast, 6);
            peer.channel = 0;
            peer.ifidx   = WIFI_IF_STA;  // WMB runs in STA mode
            peer.encrypt = false;
            esp_now_add_peer(&peer);
        }
        esp_now_send(broadcast, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
        _lastBeaconTime = now;
    }
}

// ============================================================
void SmbComms::startPairing() {
    // Remove any existing peer before starting a new pairing
    if (_hasPairedMac && _peerRegistered) {
        removePeer(_smbMac);
        _peerRegistered = false;
    }
    _hasPairedMac = false;

    _state            = SmbPairingState::BROADCASTING;
    _pairingStartTime = millis();
    _lastBeaconTime   = 0;
    Serial.printf("SMB ESP-NOW: Pairing broadcast started (%lus window, %lums interval)\n",
                  PAIRING_TIMEOUT_MS / 1000, BEACON_INTERVAL_MS);
}

// ============================================================
void SmbComms::unpair() {
    Serial.println("SMB ESP-NOW: Unpairing");
    if (_peerRegistered) {
        removePeer(_smbMac);
        _peerRegistered = false;
    }
    _hasPairedMac = false;
    memset(_smbMac, 0, 6);
    _state = SmbPairingState::IDLE;

    Preferences prefs;
    if (prefs.begin(NVS_SMB_NS, false)) {
        prefs.putBool("paired", false);
        prefs.end();
    }
}

// ============================================================
void SmbComms::sendRelayOn() {
    if (!_peerRegistered || _state != SmbPairingState::PAIRED) return;
    PktRelayCmd pkt;
    pkt.type = PKT_RELAY_ON;
    esp_now_send(_smbMac, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    Serial.println("SMB ESP-NOW: RELAY_ON sent");
}

void SmbComms::sendRelayOff() {
    if (!_peerRegistered || _state != SmbPairingState::PAIRED) return;
    PktRelayCmd pkt;
    pkt.type = PKT_RELAY_OFF;
    esp_now_send(_smbMac, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    Serial.println("SMB ESP-NOW: RELAY_OFF sent");
}

// ============================================================
void SmbComms::sendWeightUpdate(float weight) {
    if (!_peerRegistered || _state != SmbPairingState::PAIRED) return;
    PktWeightUpdate pkt;
    pkt.type   = PKT_WEIGHT_UPDATE;
    pkt.weight = weight;
    esp_now_send(_smbMac, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
}

// ============================================================
void SmbComms::sendConfig(float setpoint, bool invert) {
    if (!_peerRegistered || _state != SmbPairingState::PAIRED) return;
    PktConfigUpdate pkt;
    pkt.type     = PKT_CONFIG_UPDATE;
    pkt.setpoint = setpoint;
    pkt.invert   = invert ? 1 : 0;
    pkt.channel  = getCurrentChannel();
    esp_now_send(_smbMac, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    Serial.printf("SMB ESP-NOW: CONFIG_UPDATE sent (setpoint=%.1fg, invert=%d)\n",
                  setpoint, invert ? 1 : 0);
}

// ============================================================
void SmbComms::setSetpoint(float grams) {
    _setpoint = grams;
    saveToNVS();
    sendConfig(_setpoint, _invert);
}

void SmbComms::setInvert(bool invert) {
    _invert = invert;
    saveToNVS();
    sendConfig(_setpoint, _invert);
}

// ============================================================
// Static ESP-NOW receive callback (WiFi driver task)
// ============================================================
void SmbComms::onReceive(const uint8_t* mac, const uint8_t* data, int len) {
    if (_instance) {
        _instance->handleRecv(mac, data, len);
    }
}

void SmbComms::handleRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (len < 1) return;
    const uint8_t pktType = data[0];

    switch (pktType) {

    // ----------------------------------------------------------
    case PKT_PAIRING_RESPONSE: {
        if (_state != SmbPairingState::BROADCASTING) break;
        if (len < static_cast<int>(sizeof(PktPairingResponse))) break;

        Serial.printf("SMB ESP-NOW: PAIRING_RESPONSE from %02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // Register SMB as peer so we can send the CONFIRM
        if (!esp_now_is_peer_exist(mac)) {
            registerPeer(mac);
        }

        // Send PAIRING_CONFIRM
        PktPairingConfirm confirm;
        confirm.type    = PKT_PAIRING_CONFIRM;
        confirm.channel = getCurrentChannel();
        WiFi.macAddress(confirm.mac);

        esp_err_t err = esp_now_send(mac,
                                     reinterpret_cast<const uint8_t*>(&confirm),
                                     sizeof(confirm));
        if (err == ESP_OK) {
            memcpy(_smbMac, mac, 6);
            _hasPairedMac   = true;
            _peerRegistered = true;
            _state          = SmbPairingState::PAIRED;
            saveToNVS();
            Serial.printf("SMB ESP-NOW: PAIRED! SMB=%02X:%02X:%02X:%02X:%02X:%02X\n",
                          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        } else {
            Serial.printf("SMB ESP-NOW: PAIRING_CONFIRM send error: %s\n",
                          esp_err_to_name(err));
            removePeer(mac);
        }
        break;
    }

    // ----------------------------------------------------------
    case PKT_STATUS_ACK: {
        if (_state != SmbPairingState::PAIRED) break;
        if (len < static_cast<int>(sizeof(PktStatusAck))) break;

        const auto* pkt = reinterpret_cast<const PktStatusAck*>(data);
        _smbRelayOn       = pkt->relayOn != 0;
        _smbLastWeight    = pkt->lastWeight;
        _lastStatusAckTime = millis();
        break;
    }

    default:
        break;
    }
}

// ============================================================
// Peer helpers
// ============================================================
bool SmbComms::registerPeer(const uint8_t* mac) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;
    peer.ifidx   = WIFI_IF_STA;  // WMB runs in STA mode
    peer.encrypt = false;
    esp_err_t err = esp_now_add_peer(&peer);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
        Serial.printf("SMB ESP-NOW: esp_now_add_peer failed: %s\n", esp_err_to_name(err));
        return false;
    }
    return true;
}

void SmbComms::removePeer(const uint8_t* mac) {
    if (esp_now_is_peer_exist(mac)) {
        esp_now_del_peer(mac);
    }
}

uint8_t SmbComms::getCurrentChannel() const {
    uint8_t ch = 0;
    wifi_second_chan_t sec = WIFI_SECOND_CHAN_NONE;
    esp_wifi_get_channel(&ch, &sec);
    return ch;
}

// ============================================================
// NVS
// ============================================================
void SmbComms::loadFromNVS() {
    Preferences prefs;
    if (prefs.begin(NVS_SMB_NS, /*readOnly=*/true)) {
        _hasPairedMac = prefs.getBool("paired", false);
        if (_hasPairedMac) {
            size_t n = prefs.getBytes("smb_mac", _smbMac, 6);
            if (n != 6) _hasPairedMac = false;
        }
        _setpoint = prefs.getFloat("setpoint", 36.0f);
        _invert   = prefs.getBool ("invert",   false);
        prefs.end();
    }
}

void SmbComms::saveToNVS() {
    Preferences prefs;
    if (prefs.begin(NVS_SMB_NS, /*readOnly=*/false)) {
        prefs.putBool ("paired",   _hasPairedMac);
        if (_hasPairedMac) {
            prefs.putBytes("smb_mac", _smbMac, 6);
        }
        prefs.putFloat("setpoint", _setpoint);
        prefs.putBool ("invert",   _invert);
        prefs.end();
    }
}
