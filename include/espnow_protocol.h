#pragma once
#include <stdint.h>

// ============================================================
// StopMyBru / WeighMyBru — ESP-NOW packet protocol (shared)
//
// All packets start with a 1-byte type field.
// Structs are packed (no padding) for safe memcpy over the air.
// ============================================================

// ---- Packet type IDs ----------------------------------------
#define PKT_PAIRING_BEACON    0x01  // WMB → broadcast
#define PKT_PAIRING_RESPONSE  0x02  // SMB → WMB (reply to beacon)
#define PKT_PAIRING_CONFIRM   0x03  // WMB → SMB (finalise pairing)
#define PKT_RELAY_ON          0x10  // WMB → SMB (timer started)
#define PKT_RELAY_OFF         0x11  // WMB → SMB (manual stop)
#define PKT_WEIGHT_UPDATE     0x20  // WMB → SMB (live weight stream)
#define PKT_CONFIG_UPDATE     0x30  // WMB → SMB (setpoint / invert)
#define PKT_STATUS_ACK        0x40  // SMB → WMB (heartbeat / state)

#pragma pack(push, 1)

// WMB broadcasts this on its current WiFi channel during a pairing window
struct PktPairingBeacon {
    uint8_t type;       // PKT_PAIRING_BEACON
    uint8_t mac[6];     // WMB MAC address
    uint8_t channel;    // WMB current WiFi channel
};

// SMB replies to a beacon to announce itself
struct PktPairingResponse {
    uint8_t type;       // PKT_PAIRING_RESPONSE
    uint8_t mac[6];     // SMB MAC address
};

// WMB sends this to finalise the pairing handshake
struct PktPairingConfirm {
    uint8_t type;       // PKT_PAIRING_CONFIRM
    uint8_t mac[6];     // WMB MAC address
    uint8_t channel;    // WMB WiFi channel (SMB should lock to this)
};

// Simple relay command — no payload beyond the type byte
struct PktRelayCmd {
    uint8_t type;       // PKT_RELAY_ON or PKT_RELAY_OFF
};

// Live weight update from WMB (sent at ~20 Hz during a brew)
struct PktWeightUpdate {
    uint8_t type;       // PKT_WEIGHT_UPDATE
    float   weight;     // Weight in grams (IEEE 754 float32)
};

// Configuration push from WMB webserver to SMB
struct PktConfigUpdate {
    uint8_t type;       // PKT_CONFIG_UPDATE
    float   setpoint;   // Target cut-off weight in grams
    uint8_t invert;     // 0 = active-LOW default, 1 = active-HIGH
    uint8_t channel;    // WMB current WiFi channel (SMB re-syncs if changed)
};

// Periodic heartbeat from SMB back to WMB
struct PktStatusAck {
    uint8_t type;       // PKT_STATUS_ACK
    uint8_t relayOn;    // 1 = relay energised, 0 = relay off
    float   lastWeight; // Last weight value SMB received
    float   setpoint;   // SMB's current setpoint
};

#pragma pack(pop)
