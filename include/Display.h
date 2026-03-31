#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Scale; // Forward declaration
class FlowRate; // Forward declaration
class BluetoothScale; // Forward declaration
class PowerManager; // Forward declaration
class BatteryMonitor; // Forward declaration

class Display {
public:
    enum class TimerState { IDLE, RUNNING, STOPPED };
    
    Display(uint8_t sdaPin, uint8_t sclPin, Scale* scale, FlowRate* flowRate);
    bool begin();
    bool isConnected() const { return displayConnected; } // Check if display is available
    void update();
    void showWeight(float weight);
    void showMessage(const String& message, int duration = 2000);
    void showBatteryLowMessage(float voltage, int duration = 3000);
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
    TimerState getTimerState() const { return timerState; }
    
    // Auto Brew Timer management
    void setAutoBrewTimerEnabled(bool enabled);
    bool isAutoBrewTimerEnabled() const;
    void setAutoBrewStartThreshold(float threshold);
    float getAutoBrewStartThreshold() const;
    void setAutoBrewSlopeThreshold(float threshold);
    float getAutoBrewSlopeThreshold() const;
    void showAutoBrewStatusMessage(bool isEnabled);
    
    // Called from main loop to update flow-based detection
    void updateAutoBrewFlowDetection(float flowRate);
    
    // Called from tare handler to reset Auto Brew detection state
    void resetAutoBrewDetection();
    
    // Dual-button toggle for Auto Brew Timer
    // Returns true if tare should proceed, false if dual-button toggle was handled
    bool onTareButtonShortPress();
    // GPIO3 short press handler - returns true if timer control was fully handled (don't call handleTimerControl)
    bool onSleepButtonShortPress();
    
    // Timer duration for progress bar (in milliseconds)
    int getTimerDuration() const { return timerDuration; }
    void setTimerDuration(int duration);
    
private:
    uint8_t sdaPin;
    uint8_t sclPin;
    Scale* scalePtr;
    FlowRate* flowRatePtr;
    BluetoothScale* bluetoothPtr;
    PowerManager* powerManagerPtr;
    BatteryMonitor* batteryPtr;
    class WiFiManager* wifiManagerPtr;
    Adafruit_SSD1306* display;
    bool displayConnected; // Track if display is actually connected
    
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
    bool timerWasStarted;
    int timerDuration; // Duration in ms for progress bar (default 30000)
    float lastFlowRate; // Store last flow rate for comparison
    TimerState timerState; // IDLE, RUNNING, or STOPPED
    
    // Auto Brew Timer system - flow-based detection
    bool autoBrewTimerEnabled;
    bool autoBrewTimerActive;              // Timer was auto-started
    float autoBrewStartThreshold;           // Flow rate threshold to trigger (default 0.7 g/s)
    float autoBrewSlopeThreshold;           // Max slope for stable flow (default 0.5 g/s²)
    bool autoBrewWaitingForStart;           // Waiting for flow to exceed threshold
    bool autoBrewWaitingForStop;            // Waiting for flow to stop
    float autoBrewFlowSamples[6];           // Last 6 flow rate samples for slope calc
    unsigned long autoBrewFlowTimestamps[6]; // Timestamps for those samples
    int autoBrewFlowSampleIndex;            // Circular buffer index
    int autoBrewFlowSampleCount;            // Number of valid samples
    
    // Auto-brew pulse indicator state
    bool autoBrewPulseState;               // Current pulse visibility state
    unsigned long lastPulseToggleTime;     // Last pulse toggle timestamp
    static const unsigned long PULSE_INTERVAL = 500; // Pulse toggle interval (ms)
    
    // Dual-button detection for Auto Brew Timer toggle
    unsigned long lastTareButtonPressTime;
    unsigned long lastSleepButtonPressTime;
    static const unsigned long DUAL_BUTTON_WINDOW = 300; // 300ms window for dual press
    
    // Status page system
    bool showingStatusPage;
    unsigned long statusPageStartTime;
    static const unsigned long STATUS_PAGE_TIMEOUT = 10000; // 10 seconds timeout
    
    void drawWeight(float weight);
    void showWeightWithFlowAndTimer(float weight); // Main display showing weight, flow rate, and timer
    void setupDisplay();
    void drawBluetoothStatus(); // Draw Bluetooth connection status icon
    void drawBatteryStatus(); // Draw battery status with 3-segment indicator
    float calculateAutoBrewFlowSlope(); // Calculate slope from sample buffer
    void clearAutoBrewFlowSamples(); // Clear flow sample buffer
    void addAutoBrewFlowSample(float flowRate); // Add sample to buffer
};

#endif
