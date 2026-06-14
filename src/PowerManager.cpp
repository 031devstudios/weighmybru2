#include "PowerManager.h"
#include "Display.h"
#include "Scale.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"


PowerManager::PowerManager(uint8_t sleepTouchPin, uint8_t clockPin, Display* display) 
    : sleepTouchPin(sleepTouchPin), clockPin(clockPin), displayPtr(display), sleepTouchThreshold(0),
      lastSleepTouchState(false), lastSleepTouchTime(0), touchStartTime(0),
      debounceDelay(200), sleepCountdownStart(0), sleepCountdownActive(false),
      longPressDetected(false), cancelledRecently(false), cancelTime(0),
      timerState(TimerState::STOPPED), lastTimerControlTime(0),
      _autoSleepEnabled(false), _autoSleepTime(300), _autoSleepDrift(1.0f),
      _autoSleepBaseline(0.0f), _autoSleepWindowStart(0), _autoSleepHasBaseline(false) {
}

void PowerManager::begin() {
    // Set up the pin as digital input with pull-down resistor for the digital touch sensor module
    // This prevents false triggers when no touch sensor is connected
    pinMode(sleepTouchPin, INPUT_PULLDOWN);
    Serial.println("Power Manager initialized");
}

void PowerManager::update() {
    bool currentSleepTouchState = isSleepTouchPressed();
    unsigned long currentTime = millis();
    
    // Clear recent cancellation flag after 1 second
    if (cancelledRecently && (currentTime - cancelTime) > 1000) {
        cancelledRecently = false;
    }
    
    // Handle sleep countdown
    if (sleepCountdownActive) {
        unsigned long elapsed = currentTime - sleepCountdownStart;
        
        if (elapsed < 4000) { // Total 4 seconds: 1 sec message + 3 sec countdown
            // Show countdown every second, but only after the initial message has been shown
            if (elapsed > 1500) { // Start countdown after 1.5 seconds
                int countdownElapsed = (elapsed - 1500) / 1000; // Countdown time since 1.5s mark
                int remainingSeconds = 3 - countdownElapsed;
                
                if (remainingSeconds > 0 && (elapsed - 1500) % 1000 < 100) {
                    showSleepCountdown(remainingSeconds);
                }
            }
        } else {
            // Countdown finished, go to sleep
            enterDeepSleep();
        }
    }
    
    // Handle touch state changes with long press detection
    if (currentSleepTouchState != lastSleepTouchState) {
        if (currentTime - lastSleepTouchTime > debounceDelay) {
            if (currentSleepTouchState) {
                // Touch started
                if (sleepCountdownActive) {
                    // Touch during countdown - cancel sleep
                    sleepCountdownActive = false;
                    longPressDetected = false;
                    cancelledRecently = true;
                    cancelTime = currentTime;
                    Serial.println("Sleep cancelled - touch pressed during countdown");
                    if (displayPtr != nullptr) {
                        displayPtr->showSleepCancelledMessage();
                    }
                } else if (!cancelledRecently) {
                    // Handle timer control
                    touchStartTime = currentTime;
                    longPressDetected = false;
                    Serial.println("Timer control touch started");
                }
            } else {
                // Touch ended
                if (!sleepCountdownActive && !longPressDetected && !cancelledRecently) {
                    // Handle timer control
                    Serial.println("Timer control executed");
                    handleTimerControl();
                }
                // Don't reset longPressDetected here if countdown is active
                if (!sleepCountdownActive) {
                    longPressDetected = false;
                }
            }
            lastSleepTouchState = currentSleepTouchState;
            lastSleepTouchTime = currentTime;
        }
    }
    
    // Check for long press (1 second) for sleep functionality
    if (currentSleepTouchState && !longPressDetected && !sleepCountdownActive && !cancelledRecently) {
        if (currentTime - touchStartTime >= 1000) {
            longPressDetected = true;
            Serial.println("Sleep control executed");
            handleSleepTouch();
        }
    }

    // Auto-sleep timer check
    if (_autoSleepEnabled && _autoSleepHasBaseline && !sleepCountdownActive) {
        if (currentTime - _autoSleepWindowStart >= (unsigned long)_autoSleepTime * 1000UL) {
            Serial.println("Auto-sleep: idle timeout reached - initiating sleep countdown");
            sleepCountdownActive  = true;
            sleepCountdownStart   = currentTime;
            _autoSleepHasBaseline = false; // Re-arm after wake
        }
    }
}

void PowerManager::enterDeepSleep() {
    Serial.println("Entering deep sleep mode...");
    
    if (displayPtr != nullptr) {
        displayPtr->clearMessageState(); // Clear countdown message state
        displayPtr->showGoingToSleepMessage();
        delay(2000);
        displayPtr->clear();
        displayPtr->setPowerSave(1);
    }
    
    // Print wake-up configuration for debugging
    Serial.println("Wake-up configured for EXT1 on GPIO" + String(sleepTouchPin));
    Serial.println("Will wake when pin goes HIGH");
    
    // Flush serial output
    Serial.flush();

    // Hold the scales clock pin high to turn it off
    digitalWrite(clockPin, HIGH);
    gpio_hold_en((gpio_num_t) clockPin);

    // Configure touch wake up
    esp_sleep_enable_ext1_wakeup(1ULL << sleepTouchPin, ESP_EXT1_WAKEUP_ANY_HIGH);
    
    // Enter deep sleep - will wake up on external signal
    esp_deep_sleep_start();
}

void PowerManager::setSleepTouchThreshold(uint16_t threshold) {
    sleepTouchThreshold = threshold;
    Serial.println("Sleep touch threshold set to: " + String(sleepTouchThreshold));
}

bool PowerManager::isSleepTouchPressed() {
    // For digital touch sensor modules, check if the pin is HIGH
    bool pressed = digitalRead(sleepTouchPin) == HIGH;
    
    // Debug: log unexpected HIGH readings when no sensor should be connected
    static unsigned long lastDebugTime = 0;
    if (pressed && millis() - lastDebugTime > 5000) { // Log every 5 seconds max
        Serial.println("DEBUG: Sleep touch pin GPIO" + String(sleepTouchPin) + " reading HIGH - check for floating pin or connected sensor");
        lastDebugTime = millis();
    }
    
    return pressed;
}

void PowerManager::setDisplay(Display* display) {
    displayPtr = display;
}

void PowerManager::handleSleepTouch() {
    // Only called after long press detection
    sleepCountdownActive = true;
    sleepCountdownStart = millis();
    Serial.println("Long press detected! Starting 3-second sleep countdown...");
    if (displayPtr != nullptr) {
        displayPtr->showSleepMessage();
    }
}

void PowerManager::showSleepCountdown(int seconds) {
    if (displayPtr != nullptr) {
        displayPtr->showSleepCountdown(seconds);
    }
}

void PowerManager::handleTimerControl() {
    if (displayPtr == nullptr) return;
    
    // Prevent rapid successive timer control actions (minimum 300ms between actions)
    unsigned long currentTime = millis();
    if (currentTime - lastTimerControlTime < 300) {
        Serial.println("Timer control ignored - too soon after last action");
        return;
    }
    
    lastTimerControlTime = currentTime;
    Serial.println("Timer control triggered");
    
    // Unified mode timer control
    switch (timerState) {
        case TimerState::STOPPED:
            // First tap - start timer
            displayPtr->startTimer();
            timerState = TimerState::RUNNING;
            Serial.println("Timer started");
            if (_relayOnCb) _relayOnCb();   // Notify StopMyBru relay ON
            break;
            
        case TimerState::RUNNING:
            // Second tap - stop/pause timer
            displayPtr->stopTimer();
            timerState = TimerState::PAUSED;
            Serial.println("Timer stopped/paused");
            if (_relayOffCb) _relayOffCb(); // Notify StopMyBru relay OFF
            break;
            
        case TimerState::PAUSED:
            // Third tap - reset timer
            displayPtr->resetTimer();
            timerState = TimerState::STOPPED;
            Serial.println("Timer reset");
            if (_relayOffCb) _relayOffCb(); // Notify StopMyBru relay OFF
            break;
    }
}

void PowerManager::resetTimerState() {
    timerState = TimerState::STOPPED;
    Serial.println("PowerManager timer state reset");
}

// ── Auto-sleep ────────────────────────────────────────────────────────────────

void PowerManager::notifyWeight(float weight) {
    if (!_autoSleepEnabled) return;

    if (!_autoSleepHasBaseline) {
        _autoSleepBaseline     = weight;
        _autoSleepWindowStart  = millis();
        _autoSleepHasBaseline  = true;
        return;
    }

    // If weight has moved beyond the allowed drift, reset the idle window
    if (fabsf(weight - _autoSleepBaseline) > _autoSleepDrift) {
        _autoSleepBaseline    = weight;
        _autoSleepWindowStart = millis();
    }
}

void PowerManager::loadAutoSleepSettings() {
    Preferences prefs;
    if (prefs.begin("autosleep", true)) {
        _autoSleepEnabled = prefs.getBool("enabled",     false);
        _autoSleepTime    = prefs.getInt("timeToSleep",  300);
        _autoSleepDrift   = prefs.getFloat("driftIgnore", 1.0f);
        prefs.end();
    }
    Serial.printf("Auto-sleep loaded: enabled=%d, time=%ds, drift=%.1fg\n",
                  _autoSleepEnabled, _autoSleepTime, _autoSleepDrift);
}

void PowerManager::saveAutoSleepSettings() {
    Preferences prefs;
    if (prefs.begin("autosleep", false)) {
        prefs.putBool("enabled",     _autoSleepEnabled);
        prefs.putInt("timeToSleep",  _autoSleepTime);
        prefs.putFloat("driftIgnore", _autoSleepDrift);
        prefs.end();
    }
    Serial.printf("Auto-sleep saved: enabled=%d, time=%ds, drift=%.1fg\n",
                  _autoSleepEnabled, _autoSleepTime, _autoSleepDrift);
}
