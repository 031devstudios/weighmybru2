#include "PowerManager.h"
#include "Display.h"

PowerManager::PowerManager(uint8_t sleepTouchPin, Display* display) 
    : sleepTouchPin(sleepTouchPin), displayPtr(display), sleepTouchThreshold(0),
      lastSleepTouchState(false), lastSleepTouchTime(0), touchStartTime(0),
      debounceDelay(200), sleepCountdownStart(0), sleepCountdownActive(false),
      longPressDetected(false), cancelledRecently(false), cancelTime(0),
      timerState(TimerState::STOPPED), lastTimerControlTime(0) {
}

void PowerManager::begin() {
    // Set up the pin as digital input for the digital touch sensor module
    pinMode(sleepTouchPin, INPUT);
    
    // Configure external wake-up on the touch pin
    // Wake up when pin goes HIGH (touch sensor outputs HIGH when touched)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)sleepTouchPin, 1);
    
    Serial.println("Power Manager initialized. Sleep touch sensor on GPIO" + String(sleepTouchPin));
    Serial.println("Using EXT0 wake-up (digital touch sensor)");
    Serial.println("Device will wake up when touch sensor outputs HIGH");
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
                    // Check current mode to determine behavior
                    if (displayPtr != nullptr && (displayPtr->getMode() == ScaleMode::TIME || displayPtr->getMode() == ScaleMode::AUTO)) {
                        // In TIME or AUTO mode - just record touch start time, handle timer on release
                        touchStartTime = currentTime;
                        longPressDetected = false;
                        Serial.println("Timer control touch started");
                    } else {
                        // In FLOW mode - start timing for long press (sleep functionality)
                        touchStartTime = currentTime;
                        longPressDetected = false;
                        Serial.println("Sleep touch started");
                    }
                }
            } else {
                // Touch ended
                if (!sleepCountdownActive && !longPressDetected && !cancelledRecently) {
                    // Check if we're in TIME or AUTO mode for timer control
                    if (displayPtr != nullptr && (displayPtr->getMode() == ScaleMode::TIME || displayPtr->getMode() == ScaleMode::AUTO)) {
                        handleTimerControl();
                    } else {
                        Serial.println("Sleep touch released (short press - no action in FLOW mode)");
                    }
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
    
    // Check for long press (1 second) - works in all modes for sleep functionality
    if (currentSleepTouchState && !longPressDetected && !sleepCountdownActive && !cancelledRecently) {
        if (currentTime - touchStartTime >= 1000) {
            longPressDetected = true;
            handleSleepTouch();
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
    }
    
    // Print wake-up configuration for debugging
    Serial.println("Wake-up configured for EXT0 on GPIO" + String(sleepTouchPin));
    Serial.println("Will wake when pin goes HIGH");
    
    // Flush serial output
    Serial.flush();
    
    // Enter deep sleep - will wake up on external signal
    esp_deep_sleep_start();
}

void PowerManager::setSleepTouchThreshold(uint16_t threshold) {
    sleepTouchThreshold = threshold;
    Serial.println("Sleep touch threshold set to: " + String(sleepTouchThreshold));
}

bool PowerManager::isSleepTouchPressed() {
    // For digital touch sensor modules, check if the pin is HIGH
    return digitalRead(sleepTouchPin) == HIGH;
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
    
    switch (timerState) {
        case TimerState::STOPPED:
            // First tap - start timer
            displayPtr->startTimer();
            timerState = TimerState::RUNNING;
            Serial.println("Timer started");
            break;
            
        case TimerState::RUNNING:
            // Second tap - stop/pause timer
            displayPtr->stopTimer();
            timerState = TimerState::PAUSED;
            Serial.println("Timer stopped/paused");
            break;
            
        case TimerState::PAUSED:
            // Third tap - reset timer
            displayPtr->resetTimer();
            timerState = TimerState::STOPPED;
            Serial.println("Timer reset");
            break;
    }
}
