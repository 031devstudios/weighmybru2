#pragma once

#include <Wire.h>
#include "Ssd1306Driver.h" 
#include "DisplayLayout.h"

class Scale; // Forward declaration
class FlowRate; // Forward declaration
class BluetoothScale; // Forward declaration
class PowerManager; // Forward declaration
class BatteryMonitor; // Forward declaration

template<typename Driver>
class Display {
public:
    Display(uint8_t sdaPin, uint8_t sclPin, Scale* scale, FlowRate* flowRate);
    bool begin();
    bool isConnected() const { return displayConnected; } // Check if display is available
    void update();
    void showWeight(float weight);
    void showMessage(const String& message, int duration = 2000);
    void showSleepCountdown(int seconds); // Show sleep countdown in large format
    void showSleepMessage(); // Show initial sleep message with big/small text format
    void showGoingToSleepMessage(); // Show "Touch To / Wake Up" message like WeighMyBru Ready
    void showSleepCancelledMessage(); // Show "Sleep / Cancelled" message like WeighMyBru Ready
    void showTaringMessage(); // Show "Taring..." message like WeighMyBru Ready
    void showTaredMessage(); // Show "Tared!" message like WeighMyBru Ready
    void showWiFiStatusMessage(bool isEnabled); // Show WiFi status message like WeighMyBru Ready
    void clearMessageState(); // Clear message state to return to weight display
    void showIPAddresses(); // Show startup ready message
    void showStatusPage(); // Show status page with battery, BLE, WiFi, and scale status
    void toggleStatusPage(); // Toggle between main display and status page
    void clear();
    void setBrightness(uint8_t brightness);
    
    // Bluetooth connection status
    void setBluetoothScale(BluetoothScale* bluetooth);
    
    // Power manager reference for timer state synchronization
    void setPowerManager(PowerManager* powerManager);
    
    // Battery monitor reference for battery status display
    void setBatteryMonitor(BatteryMonitor* battery);
    
    // WiFi manager reference for network status display  
    void setWiFiManager(class WiFiManager* wifi);
    
    // Timer management
    void startTimer();
    void stopTimer();
    void resetTimer();
    bool isTimerRunning() const;
    float getTimerSeconds() const;
    unsigned long getElapsedTime() const; // Get current elapsed time in milliseconds

    void setLayout(IDisplayLayout<Driver>* layout) {
        if (layout) {
            currentLayout = layout;
        }
    }


    
private:
    uint8_t sdaPin;
    uint8_t sclPin;
    Scale* scalePtr;
    FlowRate* flowRatePtr;
    BluetoothScale* bluetoothPtr;
    PowerManager* powerManagerPtr;
    BatteryMonitor* batteryPtr;
    class WiFiManager* wifiManagerPtr;
    Ssd1306Driver* display;
    bool displayConnected; // Track if display is actually connected
    
    ClassicLayout<Driver> classicLayout;
    IDisplayLayout<Driver>* currentLayout;

    static const uint8_t SCREEN_WIDTH = 128;
    static const uint8_t SCREEN_HEIGHT = 32;
    static const uint8_t OLED_RESET = -1; // Reset pin not used
    static const uint8_t SCREEN_ADDRESS = 0x3C; // Common I2C address for SSD1306
    
    unsigned long messageStartTime;
    int messageDuration; // Store the duration for each message
    bool showingMessage;
    String currentMessage;
    
    // Timer system
    unsigned long timerStartTime;
    unsigned long timerPausedTime;
    bool timerRunning;
    bool timerPaused;
    float lastFlowRate; // Store last flow rate for comparison
    
    // Status page system
    bool showingStatusPage;
    unsigned long statusPageStartTime;
    static const unsigned long STATUS_PAGE_TIMEOUT = 10000; // 10 seconds timeout
    
    void drawWeight(float weight);
    void showWeightWithFlowAndTimer(float weight); // Main display showing weight, flow rate, and timer
    void setupDisplay();
    void drawBluetoothStatus(); // Draw Bluetooth connection status icon
    void drawBatteryStatus(); // Draw battery status with 3-segment indicator
};
using OledDisplay = Display<Ssd1306Driver>;

