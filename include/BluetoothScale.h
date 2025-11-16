#pragma once

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include "Scale.h"

class Display; // Forward declaration

enum class WeighMyBruMessageType : uint8_t {
  SYSTEM = 0x0A,
  WEIGHT = 0x0B
};

enum class BeanConquerorCommand : uint8_t {
  TARE = 0x01,
  TIMER_START = 0x02,
  TIMER_STOP = 0x03,
  TIMER_RESET = 0x04
};

class BluetoothScale : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
public:
    BluetoothScale();
    ~BluetoothScale();
    
    void begin(Scale* scale);
    void begin();  // Initialize without scale reference
    void setScale(Scale* scale);  // Set scale reference later
    void setDisplay(Display* display); // Set display reference for timer control
    void end();
    void update();
    bool isConnected();
    void sendWeight(float weight);
    void handleTareCommand();
    void handleTimerCommand(BeanConquerorCommand command);
    int getBluetoothSignalStrength(); // Get BLE signal strength (RSSI)
    String getBluetoothConnectionInfo(); // Get detailed BLE connection information
    
    // BLE Server callbacks
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    
    // BLE Characteristic callbacks
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;

private:
    Scale* scale;
    Display* display; // Reference to display for timer control
    NimBLEServer* server;
    NimBLEService* service;
    NimBLECharacteristic* weightCharacteristic;          // Bean Conqueror (simple float)
    NimBLECharacteristic* gaggiMateWeightCharacteristic; // GaggiMate (WeighMyBru protocol)
    NimBLECharacteristic* commandCharacteristic;
    NimBLEAdvertising* advertising;
    
    bool deviceConnected;
    bool oldDeviceConnected;
    uint32_t lastHeartbeat;
    uint32_t lastWeightSent;
    float lastWeight;
    int8_t connectionRSSI; // Store RSSI value for connected device
    uint16_t connectionHandle; // Store connection handle for RSSI queries
    
    // WeighMyBru protocol constants
    static const uint8_t PRODUCT_NUMBER = 0x03;
    static const size_t PROTOCOL_LENGTH = 20;
    static const uint32_t HEARTBEAT_INTERVAL = 2000; // 2 seconds
    static const uint32_t WEIGHT_SEND_INTERVAL = 50; // 50ms (20 updates/sec) - faster for GaggiMate
    
    // WeighMyBru UUIDs - unique to avoid conflicts with Bookoo scales
    static const char* SERVICE_UUID;
    static const char* WEIGHT_CHARACTERISTIC_UUID;        // Bean Conqueror (simple float)
    static const char* GAGGIMATE_CHARACTERISTIC_UUID;     // GaggiMate (WeighMyBru protocol)
    static const char* COMMAND_CHARACTERISTIC_UUID;
    
    void initializeBLE();
    void startAdvertising();
    void stopAdvertising();
    void sendMessage(WeighMyBruMessageType msgType, const uint8_t* payload, size_t length);
    void sendHeartbeat();
    void sendNotificationRequest();
    void processIncomingMessage(uint8_t* data, size_t length);
    uint8_t calculateChecksum(const uint8_t* data, size_t length);
    void sendWeightNotification(float weight);
    void sendBeanConquerorWeight(float weight);    // Send simple float format
    void sendGaggiMateWeight(float weight);        // Send WeighMyBru protocol format
};