#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include <Preferences.h>
#include <functional>
#include <math.h>

class Display; // Forward declaration
class Scale; // Forward declaration
class PowerManager {
public:
    PowerManager(uint8_t sleepTouchPin, Display* display = nullptr, Scale* scale = nullptr);
    void begin();
    void update();
    void enterDeepSleep();
    void setSleepTouchThreshold(uint16_t threshold);
    bool isSleepTouchPressed();
    void setDisplay(Display* display);
    
    // Timer control for TIME mode
    void handleTimerControl();
    void resetTimerState(); // Reset timer state to sync with auto mode

    // Register callbacks that fire when the timer starts or stops
    // Used by main.cpp to forward events to SmbComms without a hard dependency
    void setRelayOnCallback(std::function<void()> cb)  { _relayOnCb  = cb; }
    void setRelayOffCallback(std::function<void()> cb) { _relayOffCb = cb; }

    // Auto-sleep feature
    void notifyWeight(float weight);
    void loadAutoSleepSettings();
    void saveAutoSleepSettings();
    void setAutoSleepEnabled(bool enabled) { _autoSleepEnabled = enabled; }
    void setAutoSleepTime(int seconds)     { _autoSleepTime    = seconds; }
    void setAutoSleepDrift(float grams)    { _autoSleepDrift   = grams;   }
    bool getAutoSleepEnabled() const       { return _autoSleepEnabled; }
    int  getAutoSleepTime()    const       { return _autoSleepTime;    }
    float getAutoSleepDrift()  const       { return _autoSleepDrift;   }
    
private:
    uint8_t sleepTouchPin;
    Display* displayPtr;
    Scale* scalePtr;
    uint16_t sleepTouchThreshold;
    bool lastSleepTouchState;
    unsigned long lastSleepTouchTime;
    unsigned long touchStartTime;
    unsigned long debounceDelay;
    unsigned long sleepCountdownStart;
    bool sleepCountdownActive;
    bool longPressDetected;
    bool cancelledRecently;
    unsigned long cancelTime;
    
    // Timer control state
    enum class TimerState {
        STOPPED = 0,
        RUNNING = 1,
        PAUSED = 2
    };
    TimerState timerState;
    unsigned long lastTimerControlTime;
    
    void handleSleepTouch();
    void showSleepCountdown(int seconds);

    std::function<void()> _relayOnCb;
    std::function<void()> _relayOffCb;

    // Auto-sleep state
    bool          _autoSleepEnabled;
    int           _autoSleepTime;        // seconds before sleeping
    float         _autoSleepDrift;       // grams of drift to ignore
    float         _autoSleepBaseline;    // weight at start of idle window
    unsigned long _autoSleepWindowStart; // millis() when idle window began
    bool          _autoSleepHasBaseline; // true once first weight received
};

#endif
