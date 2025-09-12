#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>

// Select OLED controller:
// - Default: SSD1306 (leave this line commented)
// - SH1106:  uncomment the line below or pass -DUSE_SH1106 in build flags
// Hint: If begin() succeeds but the screen stays blank on a 128x64 panel,
//       you likely have an SH1106.
//
// #define USE_SH1106

#if defined(USE_SH1106)
  // SH1106 is also very common for unbranded displays
  #include <Adafruit_SH110X.h>
  using DisplayDriver = Adafruit_SH1106G;
#else
  // Default to SSD1306 if nothing specified
  #include <Adafruit_SSD1306.h>
  using DisplayDriver = Adafruit_SSD1306;
#endif

class Scale; // Forward declaration
class FlowRate; // Forward declaration
class BluetoothScale; // Forward declaration
class PowerManager; // Forward declaration
class BatteryMonitor; // Forward declaration

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
    
private:
    uint8_t sdaPin;
    uint8_t sclPin;
    Scale* scalePtr;
    FlowRate* flowRatePtr;
    BluetoothScale* bluetoothPtr;
    PowerManager* powerManagerPtr;
    BatteryMonitor* batteryPtr;
    class WiFiManager* wifiManagerPtr;
    DisplayDriver* display;     //  alias which covers both SSD1306 and SH1106
    bool displayConnected; // Track if display is actually connected
    
    static const uint8_t SCREEN_WIDTH = 128;
    static const uint8_t SCREEN_HEIGHT = 32;
    static const int8_t OLED_RESET = -1; // Reset pin not used
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

#endif
