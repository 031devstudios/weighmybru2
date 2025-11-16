#ifndef TOUCHSENSOR_H
#define TOUCHSENSOR_H

#include <Arduino.h>
#include <Display.h>

class Scale; // Forward declaration
class FlowRate; // Forward declaration

class TouchSensor {
public:
    TouchSensor(uint8_t touchPin, Scale* scale);
    void begin();
    void update();
    void setTouchThreshold(uint16_t threshold);
    uint16_t getTouchValue();
    bool isTouched();
    void setDisplay(OledDisplay* display); // Set display reference
    void setFlowRate(FlowRate* flowRate); // Set flow rate reference
    
private:
    uint8_t touchPin;
    Scale* scalePtr;
    OledDisplay* displayPtr;
    FlowRate* flowRatePtr;
    uint16_t touchThreshold;
    bool lastTouchState;
    unsigned long lastTouchTime;
    unsigned long touchStartTime;
    unsigned long debounceDelay;
    bool longPressDetected;
    
    // Delayed tare functionality for mounted touch sensors
    bool delayedTarePending;
    unsigned long delayedTareTime;
    static const unsigned long TARE_DELAY = 1500; // 1.5 seconds delay after touch release
    static const unsigned long WIFI_TOGGLE_DURATION = 5000; // 5 seconds for WiFi toggle (longer than status page)
    
    void handleTouch();
    void scheduleDelayedTare();
    void checkDelayedTare();
    void handleLongPress();
    void handleStatusPageToggle(); // Handle status page toggle on medium press
    void handleWiFiToggle(); // Handle WiFi toggle on long press (5 seconds)
};

#endif
