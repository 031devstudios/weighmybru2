#include "BluetoothScale.h"
#include "Display.h"
#include <Arduino.h>
#include <stdexcept>
#include <esp_bt.h>

// UUIDs for WeighMyBru protocol - unique to avoid conflicts with Bookoo scales
const char* BluetoothScale::SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* BluetoothScale::WEIGHT_CHARACTERISTIC_UUID = "6E400004-B5A3-F393-E0A9-E50E24DCCA9E";  // Bean Conqueror (new UUID)
const char* BluetoothScale::GAGGIMATE_CHARACTERISTIC_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";  // GaggiMate (original UUID)
const char* BluetoothScale::COMMAND_CHARACTERISTIC_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

BluetoothScale::BluetoothScale() 
    : scale(nullptr), display(nullptr), server(nullptr), service(nullptr), 
      weightCharacteristic(nullptr), gaggiMateWeightCharacteristic(nullptr), 
      commandCharacteristic(nullptr), advertising(nullptr), deviceConnected(false), 
      oldDeviceConnected(false), lastHeartbeat(0), lastWeightSent(0), lastWeight(0.0f),
      connectionRSSI(-100), connectionHandle(0) {
}

BluetoothScale::~BluetoothScale() {
    end();
}

void BluetoothScale::begin(Scale* scaleInstance) {
    scale = scaleInstance;
    
    Serial.println("BluetoothScale: Starting BLE initialization...");
    
    // Check available memory before BLE initialization
    size_t freeHeap = ESP.getFreeHeap();
    Serial.println("BluetoothScale: Free heap before BLE: " + String(freeHeap) + " bytes");
    
    if (freeHeap < 50000) {  // Need at least 50KB for BLE
        Serial.println("BluetoothScale: Insufficient memory for BLE - disabling");
        scale = nullptr;
        return;
    }
    
    // Add comprehensive error handling for BLE initialization
    bool initializationSuccessful = false;
    
    try {
        Serial.println("BluetoothScale: Releasing Classic BT memory...");
        
        // Release Classic Bluetooth memory more carefully
        esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
        if (ret != ESP_OK) {
            Serial.println("BluetoothScale: Warning - could not release Classic BT memory: " + String(esp_err_to_name(ret)));
        }
        
        Serial.println("BluetoothScale: Initializing BLE device directly...");
        
        // Skip btStart() and go directly to BLE initialization
        // This avoids the problematic Bluetooth controller initialization
        initializeBLE();
        
        // Small delay before starting advertising
        delay(200);
        
        startAdvertising();
        
        Serial.println("BluetoothScale: Successfully started advertising as WeighMyBru");
        initializationSuccessful = true;
        
    } catch (const std::exception& e) {
        Serial.println("BluetoothScale: Exception during initialization: " + String(e.what()));
        scale = nullptr;
    } catch (...) {
        Serial.println("BluetoothScale: Unknown error during initialization - disabling Bluetooth");
        scale = nullptr;
    }
    
    if (!initializationSuccessful) {
        Serial.println("BluetoothScale: BLE initialization failed - scale will work without Bluetooth");
        // Clean up any partial initialization
        end();
    } else {
        Serial.println("BluetoothScale: BLE initialization completed successfully");
        Serial.println("BluetoothScale: Free heap after BLE: " + String(ESP.getFreeHeap()) + " bytes");
    }
}

void BluetoothScale::end() {
    if (server) {
        stopAdvertising();
        NimBLEDevice::deinit();
        server = nullptr;
        service = nullptr;
        weightCharacteristic = nullptr;
        commandCharacteristic = nullptr;
        advertising = nullptr;
    }
}

void BluetoothScale::initializeBLE() {
    Serial.println("BluetoothScale: Initializing BLE device...");
    Serial.printf("BluetoothScale: Free heap at start: %u bytes\n", ESP.getFreeHeap());
    
    // Reduce BLE power consumption during initialization to prevent voltage sag
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N0);      // Moderate advertising power (0dBm)
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_N0); // Moderate connection power (0dBm)
    
    // Initialize BLE Device with WeighMyBru name - this handles the low-level BLE stack
    NimBLEDevice::init("WeighMyBru");
    
    // Set moderate power to reduce current draw during boot while maintaining connectivity
    NimBLEDevice::setPower(ESP_PWR_LVL_N0);  // Moderate BLE power reduction (0dBm)
    
    // Small delay to let power settle
    delay(100);
    
    Serial.printf("BluetoothScale: Free heap after NimBLEDevice::init: %u bytes\n", ESP.getFreeHeap());
    
    Serial.println("BluetoothScale: Creating BLE server...");
    
    // Create BLE Server
    server = NimBLEDevice::createServer();
    if (!server) {
        throw std::runtime_error("Failed to create BLE server");
    }
    server->setCallbacks(this);
    
    Serial.println("BluetoothScale: Creating BLE service...");
    
    // Create BLE Service
    service = server->createService(SERVICE_UUID);
    if (!service) {
        throw std::runtime_error("Failed to create BLE service");
    }
    
    Serial.println("BluetoothScale: Creating characteristics...");
    
    // Create Weight Characteristic for GaggiMate (WeighMyBru protocol format) - Keep original UUID
    gaggiMateWeightCharacteristic = service->createCharacteristic(
        GAGGIMATE_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::NOTIFY |
        NIMBLE_PROPERTY::INDICATE
    );
    
    if (!gaggiMateWeightCharacteristic) {
        Serial.println("BluetoothScale: ERROR - Failed to create GaggiMate weight characteristic");
        throw std::runtime_error("Failed to create GaggiMate weight characteristic");
    }
    
    Serial.println("BluetoothScale: GaggiMate characteristic created successfully");
    
    // Note: NimBLE automatically creates 0x2902 descriptors for characteristics with NOTIFY/INDICATE properties
    
    // Create Weight Characteristic for Bean Conqueror (simple float format) - New UUID
    weightCharacteristic = service->createCharacteristic(
        WEIGHT_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::NOTIFY |
        NIMBLE_PROPERTY::INDICATE
    );
    
    if (!weightCharacteristic) {
        Serial.println("BluetoothScale: ERROR - Failed to create Bean Conqueror weight characteristic");
        throw std::runtime_error("Failed to create Bean Conqueror weight characteristic");
    }
    
    Serial.println("BluetoothScale: Bean Conqueror characteristic created successfully");
    
    // Note: NimBLE automatically creates 0x2902 descriptors for characteristics with NOTIFY/INDICATE properties
    
    // Create Command Characteristic (for receiving commands)
    commandCharacteristic = service->createCharacteristic(
        COMMAND_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::WRITE_NR |
        NIMBLE_PROPERTY::NOTIFY
    );
    
    if (!commandCharacteristic) {
        throw std::runtime_error("Failed to create command characteristic");
    }
    commandCharacteristic->setCallbacks(this);
    
    Serial.println("BluetoothScale: Starting service...");
    
    // Start the service
    service->start();
    
    Serial.println("BluetoothScale: Setting up advertising...");
    
    // Get advertising object
    advertising = NimBLEDevice::getAdvertising();
    if (!advertising) {
        throw std::runtime_error("Failed to get advertising object");
    }
    
    advertising->addServiceUUID(SERVICE_UUID);
    
    // Enable scan response to allow full device name in advertising
    advertising->setScanResponse(true);
    
    // Explicitly set the advertising name to ensure full "WeighMyBru" appears
    advertising->setName("WeighMyBru");
    
    advertising->setMinPreferred(0x0);
    
    Serial.println("BluetoothScale: BLE initialization completed successfully");
}

void BluetoothScale::startAdvertising() {
    if (advertising) {
        advertising->start();
    }
}

void BluetoothScale::stopAdvertising() {
    if (advertising) {
        advertising->stop();
    }
}

void BluetoothScale::update() {
    // Return early if initialization failed
    if (scale == nullptr) {
        return;
    }
    
    uint32_t now = millis();
    
    // Handle connection state changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack time to get ready
        server->startAdvertising();
        Serial.println("BluetoothScale: Start advertising after disconnect");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        Serial.println("BluetoothScale: Client connected");
        oldDeviceConnected = deviceConnected;
        lastHeartbeat = now;
        
        // Send initialization response for WeighMyBru client
        delay(100); // Give time for connection to stabilize
        sendNotificationRequest();
    }
    
    if (deviceConnected) {
        // Send weight updates - faster for GaggiMate brewing applications
        if (scale && (now - lastWeightSent >= WEIGHT_SEND_INTERVAL)) {
            float currentWeight = scale->getCurrentWeight();
            // Send all weight updates for real-time brewing feedback
            sendWeightNotification(currentWeight);
            lastWeight = currentWeight;
            lastWeightSent = now;
        }
        
        // Send heartbeat
        if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
            sendHeartbeat();
            lastHeartbeat = now;
        }
    }
}

bool BluetoothScale::isConnected() {
    return deviceConnected;
}

void BluetoothScale::sendWeightNotification(float weight) {
    if (!deviceConnected) {
        return;
    }
    
    // Send to GaggiMate first (WeighMyBru protocol format) - critical for backward compatibility
    sendGaggiMateWeight(weight);
    
    // Send to Bean Conqueror (simple float format)  
    sendBeanConquerorWeight(weight);
}

void BluetoothScale::sendBeanConquerorWeight(float weight) {
    if (!weightCharacteristic) {
        return;
    }
    
    try {
        // Bean Conqueror expects a simple 4-byte float in little-endian format
        union {
            float floatValue;
            uint8_t bytes[4];
        } weightData;
        
        weightData.floatValue = weight;
        
        // ESP32 is little-endian, so bytes are already in correct order for Bean Conqueror
        weightCharacteristic->setValue(weightData.bytes, 4);
        weightCharacteristic->notify();
        
        //Serial.printf("BluetoothScale: Sent Bean Conqueror weight %.2fg as 4-byte float\n", weight);
    } catch (const std::exception& e) {
        Serial.printf("BluetoothScale: ERROR sending Bean Conqueror weight: %s\n", e.what());
    }
}

void BluetoothScale::sendGaggiMateWeight(float weight) {
    if (!gaggiMateWeightCharacteristic) {
        Serial.println("BluetoothScale: WARNING - GaggiMate characteristic is null!");
        return;
    }
    
    try {
        // Convert weight to integer (grams * 100 for 0.01g precision)
        int32_t weightInt = (int32_t)(weight * 100);
        
        // Create weight message following WeighMyBru protocol
        uint8_t payload[PROTOCOL_LENGTH] = {0};
        
        // Header
        payload[0] = PRODUCT_NUMBER; // Product number
        payload[1] = static_cast<uint8_t>(WeighMyBruMessageType::WEIGHT); // Message type
        payload[2] = 0x00; // Reserved
        payload[3] = 0x00; // Reserved
        payload[4] = 0x00; // Reserved
        payload[5] = 0x00; // Reserved
        
        // Sign (positive = 43, negative = 45)
        payload[6] = (weightInt >= 0) ? 43 : 45;
        
        // Weight data (3 bytes, big endian)
        uint32_t absWeight = abs(weightInt);
        payload[7] = (absWeight >> 16) & 0xFF;
        payload[8] = (absWeight >> 8) & 0xFF;
        payload[9] = absWeight & 0xFF;
        
        // Fill remaining bytes with zeros
        for (int i = 10; i < PROTOCOL_LENGTH - 1; i++) {
            payload[i] = 0x00;
        }
        
        // Calculate and set checksum (last byte)
        payload[PROTOCOL_LENGTH - 1] = calculateChecksum(payload, PROTOCOL_LENGTH - 1);
        
        // Send notification
        gaggiMateWeightCharacteristic->setValue(payload, PROTOCOL_LENGTH);
        gaggiMateWeightCharacteristic->notify();
        
        //Serial.printf("BluetoothScale: Sent GaggiMate weight %.2fg as WeighMyBru protocol\n", weight);
    } catch (const std::exception& e) {
        Serial.printf("BluetoothScale: ERROR sending GaggiMate weight: %s\n", e.what());
    }
}

void BluetoothScale::sendHeartbeat() {
    if (!deviceConnected || !commandCharacteristic) return;
    
    // Send system heartbeat message
    uint8_t payload[] = {0x02, 0x00};
    sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
    
    Serial.println("BluetoothScale: Heartbeat sent");
}

void BluetoothScale::sendNotificationRequest() {
    if (!deviceConnected) return;
    
    // Send notification request for WeighMyBru initialization
    uint8_t payload[] = {0x06, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
    
    Serial.println("BluetoothScale: Notification request sent");
}

void BluetoothScale::sendMessage(WeighMyBruMessageType msgType, const uint8_t* payload, size_t length) {
    if (!deviceConnected || !commandCharacteristic) return;
    
    // Create message buffer
    uint8_t message[length + 1];
    
    // Copy payload
    memcpy(message, payload, length);
    
    // Calculate and append checksum
    message[length] = calculateChecksum(message, length);
    
    // Send via command characteristic
    commandCharacteristic->setValue(message, length + 1);
}

uint8_t BluetoothScale::calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = data[0];
    for (size_t i = 1; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

void BluetoothScale::handleTareCommand() {
    if (scale) {
        Serial.println("BluetoothScale: Executing tare command");
        scale->tare(10);
        
        // Send tare confirmation
        uint8_t payload[] = {0x03, 0x0a, 0x01, 0x00, 0x00};
        sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
    }
}

void BluetoothScale::handleTimerCommand(BeanConquerorCommand command) {
    if (!display) {
        Serial.println("BluetoothScale: Display not available for timer command");
        return;
    }
    
    switch (command) {
        case BeanConquerorCommand::TIMER_START:
            Serial.println("BluetoothScale: Starting timer");
            display->startTimer();
            // Send timer start confirmation
            {
                uint8_t payload[] = {0x03, 0x0a, 0x02, 0x01, 0x00};
                sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
            }
            break;
            
        case BeanConquerorCommand::TIMER_STOP:
            Serial.println("BluetoothScale: Stopping timer");
            display->stopTimer();
            // Send timer stop confirmation
            {
                uint8_t payload[] = {0x03, 0x0a, 0x03, 0x01, 0x00};
                sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
            }
            break;
            
        case BeanConquerorCommand::TIMER_RESET:
            Serial.println("BluetoothScale: Resetting timer");
            display->resetTimer();
            // Send timer reset confirmation
            {
                uint8_t payload[] = {0x03, 0x0a, 0x04, 0x01, 0x00};
                sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
            }
            break;
            
        default:
            Serial.printf("BluetoothScale: Unknown timer command: 0x%02X\n", static_cast<uint8_t>(command));
            break;
    }
}

void BluetoothScale::processIncomingMessage(uint8_t* data, size_t length) {
    if (length < 2) return;
    
    uint8_t productNumber = data[0];
    WeighMyBruMessageType messageType = static_cast<WeighMyBruMessageType>(data[1]);
    
    Serial.printf("BluetoothScale: Received message - Product: 0x%02X, Type: 0x%02X\n", 
                  productNumber, static_cast<uint8_t>(messageType));
    
    // Accept messages from GaggiMate (Product 0x02) and WeighMyBru (Product 0x03)
    if (productNumber != 0x02 && productNumber != PRODUCT_NUMBER) {
        Serial.printf("BluetoothScale: Ignoring message from unknown product: 0x%02X\n", productNumber);
        return;
    }
    
    if (messageType == WeighMyBruMessageType::SYSTEM && length >= 4) {
        BeanConquerorCommand command = static_cast<BeanConquerorCommand>(data[2]);
        
        switch (command) {
            case BeanConquerorCommand::TARE:
                if (data[3] == 0x01) { // Command trigger
                    handleTareCommand();
                }
                break;
                
            case BeanConquerorCommand::TIMER_START:
                if (data[3] == 0x01) { // Command trigger
                    handleTimerCommand(BeanConquerorCommand::TIMER_START);
                }
                break;
                
            case BeanConquerorCommand::TIMER_STOP:
                if (data[3] == 0x01) { // Command trigger
                    handleTimerCommand(BeanConquerorCommand::TIMER_STOP);
                }
                break;
                
            case BeanConquerorCommand::TIMER_RESET:
                if (data[3] == 0x01) { // Command trigger
                    handleTimerCommand(BeanConquerorCommand::TIMER_RESET);
                }
                break;
                
            default:
                Serial.printf("BluetoothScale: Unknown command: 0x%02X\n", static_cast<uint8_t>(command));
                break;
        }
    }
}

// BLE Server Callbacks
void BluetoothScale::onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    NimBLEDevice::stopAdvertising();
    Serial.println("BluetoothScale: Device connected");
}

void BluetoothScale::onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BluetoothScale: Device disconnected");
}

// BLE Characteristic Callbacks
void BluetoothScale::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
        uint8_t* data = (uint8_t*)value.data();
        size_t length = value.length();
        
        Serial.printf("BluetoothScale: Received %d bytes\n", length);
        processIncomingMessage(data, length);
    }
}

// Overloaded begin method for early initialization
void BluetoothScale::begin() {
    begin(nullptr);  // Initialize without scale reference
}

void BluetoothScale::setScale(Scale* scaleInstance) {
    scale = scaleInstance;
    Serial.println("BluetoothScale: Scale reference set");
}

void BluetoothScale::setDisplay(OledDisplay* displayInstance) {
    display = displayInstance;
    Serial.println("BluetoothScale: Display reference set");
}

// Get BLE signal strength (RSSI)
int BluetoothScale::getBluetoothSignalStrength() {
    if (!deviceConnected || !server) {
        return -100; // Return very weak signal if not connected
    }
    
    // Try to get RSSI from the BLE connection
    // Note: ESP32 BLE library doesn't directly expose RSSI for server connections
    // This is a limitation of the current BLE implementation
    return connectionRSSI; // Will be updated when available
}

// Get detailed BLE connection information
String BluetoothScale::getBluetoothConnectionInfo() {
    String info = "{";
    
    info += "\"connected\":" + String(deviceConnected ? "true" : "false") + ",";
    info += "\"advertising\":" + String((advertising != nullptr) ? "true" : "false") + ",";
    
    if (deviceConnected) {
        info += "\"signal_strength\":" + String(connectionRSSI) + ",";
        
        if (connectionRSSI >= -30) {
            info += "\"signal_quality\":\"Excellent\",";
        } else if (connectionRSSI >= -50) {
            info += "\"signal_quality\":\"Very Good\",";
        } else if (connectionRSSI >= -60) {
            info += "\"signal_quality\":\"Good\",";
        } else if (connectionRSSI >= -70) {
            info += "\"signal_quality\":\"Fair\",";
        } else if (connectionRSSI >= -80) {
            info += "\"signal_quality\":\"Weak\",";
        } else {
            info += "\"signal_quality\":\"Very Weak\",";
        }
        
        info += "\"connection_handle\":" + String(connectionHandle) + ",";
        info += "\"service_uuid\":\"" + String(SERVICE_UUID) + "\",";
        info += "\"device_name\":\"WeighMyBru\"";
    } else {
        info += "\"signal_strength\":null,";
        info += "\"signal_quality\":\"Disconnected\",";
        info += "\"connection_handle\":null,";
        info += "\"service_uuid\":\"" + String(SERVICE_UUID) + "\",";
        info += "\"device_name\":\"WeighMyBru\"";
    }
    
    info += "}";
    return info;
}
