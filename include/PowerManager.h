#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include "Display.h"  

class PowerManager {
public:
    PowerManager(uint8_t sleepTouchPin, OledDisplay* display = nullptr);
    void begin();
    void update();
    void enterDeepSleep();
    void setSleepTouchThreshold(uint16_t threshold);
    bool isSleepTouchPressed();
    void setDisplay(OledDisplay* display);
    
    // Timer control for TIME mode
    void handleTimerControl();
    void resetTimerState(); // Reset timer state to sync with auto mode
    
private:
    uint8_t sleepTouchPin;
    OledDisplay* displayPtr;
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
};

#endif
