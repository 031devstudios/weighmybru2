#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include "espnow_protocol.h"

// ============================================================
// SmbPairingState — FSM states for WMB side of the pairing
// ============================================================
enum class SmbPairingState : uint8_t {
    IDLE,              // No active pairing attempt
    BROADCASTING,      // Sending PAIRING_BEACONs, waiting for SMB response
    AWAITING_CONFIRM,  // Received PAIRING_RESPONSE, sent PAIRING_CONFIRM, listening for STATUS_ACK
    PAIRED             // Active pairing; relay comms operational
};

// ============================================================
// SmbComms — WeighMyBru side of the ESP-NOW link
//
// Responsibilities:
//   - Broadcast PAIRING_BEACON on current channel when requested
//   - Receive PAIRING_RESPONSE from SMB, send PAIRING_CONFIRM
//   - Send RELAY_ON / RELAY_OFF commands (timer start/stop events)
//   - Send WEIGHT_UPDATE every 50ms while paired
//   - Send CONFIG_UPDATE when setpoint or invert changes
//   - Receive STATUS_ACK heartbeats from SMB and cache last state
//   - Persist SMB MAC, channel, setpoint, and invert to NVS ("smb")
// ============================================================

class SmbComms {
public:
    SmbComms();

    // Call once in setup(), after WiFi is initialised
    void begin();

    // Drive state machine — call from every loop() iteration
    void update();

    // Broadcast PAIRING_BEACONs for PAIRING_TIMEOUT_MS (30 s)
    void startPairing();

    // Clear stored pairing and stop all comms
    void unpair();

    // ---- Relay commands ----
    void sendRelayOn();
    void sendRelayOff();

    // ---- Data commands ----
    // Should be called every 50ms while paired (piggyback on weight loop)
    void sendWeightUpdate(float weight);

    // Push current setpoint + invert flag to SMB; also persists to NVS
    void sendConfig(float setpoint, bool invert);

    // ---- Setters that persist to NVS and push CONFIG_UPDATE if paired ----
    void setSetpoint(float grams);
    void setInvert(bool invert);

    // ---- Accessors (used by WebServer route handlers) ----
    bool             isPaired()         const { return _state == SmbPairingState::PAIRED; }
    SmbPairingState  getPairingState()  const { return _state; }
    const uint8_t*   getSmbMac()        const { return _smbMac; }
    float            getSetpoint()      const { return _setpoint; }
    bool             isInverted()       const { return _invert; }

    // SMB-reported state (from last STATUS_ACK)
    bool   getSmbRelayOn()     const { return _smbRelayOn; }
    float  getSmbLastWeight()  const { return _smbLastWeight; }
    unsigned long getLastSeenMs() const { return _lastStatusAckTime; }

    // ---- NVS ----
    void loadFromNVS();
    void saveToNVS();

private:
    SmbPairingState _state             = SmbPairingState::IDLE;
    uint8_t         _smbMac[6]        = {0};
    bool            _hasPairedMac     = false;
    bool            _peerRegistered   = false;

    // Pairing broadcast timing
    unsigned long   _pairingStartTime  = 0;
    unsigned long   _lastBeaconTime    = 0;
    static constexpr unsigned long BEACON_INTERVAL_MS   = 500;
    static constexpr unsigned long PAIRING_TIMEOUT_MS   = 30000;

    // Config
    float   _setpoint  = 36.0f;
    bool    _invert    = false;

    // Cached SMB-reported state (populated by STATUS_ACK)
    bool          _smbRelayOn        = false;
    float         _smbLastWeight     = 0.0f;
    unsigned long _lastStatusAckTime = 0;

    // Static singleton for C-style callback
    static SmbComms* _instance;

    static void onReceive(const uint8_t* mac, const uint8_t* data, int len);
    void handleRecv(const uint8_t* mac, const uint8_t* data, int len);

    bool registerPeer(const uint8_t* mac);
    void removePeer(const uint8_t* mac);

    uint8_t getCurrentChannel() const;
};
