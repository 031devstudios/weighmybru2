#include "Display.h"
#include "Scale.h"
#include "FlowRate.h"
#include "BluetoothScale.h"
#include "PowerManager.h"
#include "BatteryMonitor.h"
#include <WiFi.h>
#include <math.h>

#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSans12pt7b.h"

#if defined(USE_SH1106)
  static constexpr uint16_t OLED_WHITE = SH110X_WHITE;
  static constexpr uint16_t OLED_BLACK = SH110X_BLACK;
#else
  static constexpr uint16_t OLED_WHITE = SSD1306_WHITE;
  static constexpr uint16_t OLED_BLACK = SSD1306_BLACK;
#endif


Display::Display(uint8_t sdaPin, uint8_t sclPin, Scale* scale, FlowRate* flowRate)
    : sdaPin(sdaPin), sclPin(sclPin), scalePtr(scale), flowRatePtr(flowRate), bluetoothPtr(nullptr), powerManagerPtr(nullptr), batteryPtr(nullptr), wifiManagerPtr(nullptr),
      messageStartTime(0), messageDuration(2000), showingMessage(false), 
      timerStartTime(0), timerPausedTime(0), timerRunning(false), timerPaused(false),
      lastFlowRate(0.0), showingStatusPage(false), statusPageStartTime(0) {
    // display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    display = new DisplayDriver(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
}

bool Display::begin() {
    Serial.println("Initializing display...");
    
    // Initialize I2C with custom pins
    Wire.begin(sdaPin, sclPin);
    
    // Test I2C connection first with timeout
    Serial.println("Testing I2C connection to display...");
    unsigned long startTime = millis();
    const unsigned long I2C_TIMEOUT = 3000; // 3 second timeout
    
    bool i2cResponding = false;
    Wire.beginTransmission(SCREEN_ADDRESS);
    
    // Wait for I2C response with timeout
    while (millis() - startTime < I2C_TIMEOUT) {
        if (Wire.endTransmission() == 0) {
            i2cResponding = true;
            Serial.println("I2C device found at display address");
            break;
        }
        delay(100);
        Wire.beginTransmission(SCREEN_ADDRESS);
    }
    
    if (!i2cResponding) {
        Serial.println("ERROR: No I2C device found at display address");
        Serial.println("Display will be disabled - running headless mode");
        Serial.println("Check connections:");
        Serial.printf("- SDA to GPIO %d\n", sdaPin);
        Serial.printf("- SCL to GPIO %d\n", sclPin);
        Serial.println("- VCC to 3.3V");
        Serial.println("- GND to GND");
        displayConnected = false;
        return false;
    }
    
    bool displayInitialized = false;
    // Initialize the display
#if defined(USE_SH1106)
    displayInitialized = display->begin(SCREEN_ADDRESS, true);
#else
    displayInitialized = display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
#endif
    if (!displayInitialized) {
        Serial.println("ERROR: SSD1306/SH1106 initialization failed");
        Serial.println("Display will be disabled - running headless mode");
        displayConnected = false;
        return false;
    }
    
    Serial.println("Display connected and initialized successfully");
    displayConnected = true;
    setupDisplay();
    
    // Show startup message in same format as welcome message
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(OLED_WHITE);
    

    showCenteredText("WeighMyBru", "Starting");
    
    Serial.println("SSD1306 display initialized on SDA:" + String(sdaPin) + " SCL:" + String(sclPin));
    
    return true;
}

void Display::setupDisplay() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    display->clearDisplay();
    display->setTextColor(OLED_WHITE);
    display->cp437(true); // Use full 256 char 'Code Page 437' font
}

void Display::update() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Check if status page timeout has elapsed
    if (showingStatusPage && millis() - statusPageStartTime > STATUS_PAGE_TIMEOUT) {
        showingStatusPage = false;
        Serial.println("Status page timeout, returning to main display");
    }
    
    // Check if message duration has elapsed
    if (showingMessage) {
        int effectiveDuration = messageDuration;
        // Use shorter duration for tared message
        if (currentMessage == "Tared message") {
            effectiveDuration = 1000; // Half of default duration for quick feedback
        }
        
        if (millis() - messageStartTime > effectiveDuration) {
            showingMessage = false;
            Serial.println("Message cleared, returning to main display");
        }
    }
    
    // Show status page if active
    if (showingStatusPage) {
        showStatusPage();
    }
    // Show normal weight display when not showing message or status page
    else if (!showingMessage && scalePtr != nullptr) {
        float weight = scalePtr->getCurrentWeight();
        showWeightWithFlowAndTimer(weight);
    }
}

void Display::showWeight(float weight) {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }

    if (showingMessage) return; // Don't override messages
    
    // Use the unified display showing weight, flow rate, and timer
    showWeightWithFlowAndTimer(weight);
}

void Display::showMessage(const String& message, int duration) {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    currentMessage = message;
    messageStartTime = millis();
    messageDuration = duration; // Store the duration
    showingMessage = true;
    
    display->clearDisplay();
    display->setTextSize(1);
    display->setCursor(0, 0);
    
    // Word wrap for longer messages
    int lineHeight = 8;
    int maxCharsPerLine = 21; // For 128px width
    int currentLine = 0;
    
    for (int i = 0; i < message.length() && currentLine < 4; i += maxCharsPerLine) {
        String line = message.substring(i, min(i + maxCharsPerLine, (int)message.length()));
        display->setCursor(0, currentLine * lineHeight);
        display->print(line);
        currentLine++;
    }
    
    display->display();
    
    // Update duration for this message
    if (duration > 0) {
        // We'll check this in update() method
    }
}

void Display::showSleepCountdown(int seconds) {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Set message state to prevent weight display interference
    currentMessage = "Sleep countdown active";
    messageStartTime = millis();
    showingMessage = true;

    showCenteredText("Sleep in", String(seconds) + "...");
}

void Display::showSleepMessage() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Set message state to prevent weight display interference
    currentMessage = "Sleep message active";
    messageStartTime = millis();
    showingMessage = true;
    
    showCenteredText("Sleeping..", "Touch to cancel", 2, 1);
}

void Display::showGoingToSleepMessage() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Set message state to prevent weight display interference
    currentMessage = "Going to sleep message";
    messageStartTime = millis();
    showingMessage = true;
    
    // Show "Touch To / Wake Up" in same format as WeighMyBru Ready
    showCenteredText("Touch To", "Wake Up");
}

void Display::showSleepCancelledMessage() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Set message state to prevent weight display interference
    currentMessage = "Sleep cancelled message";
    messageStartTime = millis();
    showingMessage = true;
    
    // Show "Sleep / Cancelled" in same format as WeighMyBru Ready
    showCenteredText("Sleep", "Cancelled");
}

void Display::showTaringMessage() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Set message state to prevent weight display interference
    currentMessage = "Taring message";
    messageStartTime = millis();
    showingMessage = true;
    
    // Show "Taring..." in same format as WeighMyBru Ready
    showCenteredText("Taring", "...");

}

void Display::showTaredMessage() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Set message state to prevent weight display interference
    currentMessage = "Tared message";
    messageStartTime = millis();
    showingMessage = true;
    
    // Show "Tared!" in same format as WeighMyBru Ready
    showCenteredText("Scale", "Tared!");
}

void Display::clearMessageState() {
    showingMessage = false;
    currentMessage = "";
    messageStartTime = 0;
}

void Display::showIPAddresses() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Show the WeighMyBru Ready message for 3 seconds
    showCenteredText("WeighMyBru", "Ready");
    delay(1000); // Show ready message for 1 second, then continue to normal display
}

void Display::clear() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    display->clearDisplay();
    display->display();
}

void Display::setBrightness(uint8_t brightness) {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
#if defined(USE_SH1106)
    display->setContrast(brightness);
#else
    // SSD1306 doesn't have brightness control, but we can simulate with contrast
    display->ssd1306_command(SSD1306_SETCONTRAST);
    display->ssd1306_command(brightness);
#endif
}

void Display::setBluetoothScale(BluetoothScale* bluetooth) {
    bluetoothPtr = bluetooth;
}

void Display::setPowerManager(PowerManager* powerManager) {
    powerManagerPtr = powerManager;
}

void Display::setBatteryMonitor(BatteryMonitor* battery) {
    batteryPtr = battery;
}

void Display::setWiFiManager(WiFiManager* wifi) {
    wifiManagerPtr = wifi;
}

void Display::drawBluetoothStatus() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // Draw "BT" text with rectangle border when connected
    if (bluetoothPtr) {
        display->setTextSize(1);
        display->setCursor(115, 0); // Position at top right
        display->print("BT");
        
        // If connected, draw rectangle around "BT"
        if (bluetoothPtr->isConnected()) {
            display->drawRect(113, -1, 16, 10, OLED_WHITE); // Rectangle around "BT"
        }
    }
}

void Display::drawBatteryStatus() {
    // Return early if display is not connected or no battery monitor
    if (!displayConnected || !batteryPtr) {
        return;
    }
    
    // Get battery percentage and critical status
    int batteryPercentage = batteryPtr->getBatteryPercentage();
    bool isCritical = batteryPtr->isCriticalBattery();
    
    // Format percentage string
    String percentStr = String(batteryPercentage) + "%";
    
    // Set small text size for percentage display
    display->setTextSize(1);
    
    // For critical battery, make it flash (every 500ms)
    if (isCritical && (millis() % 1000 < 500)) {
        // Flash state - draw text with inverted colors (black text on white background)
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        display->getTextBounds(percentStr, 0, 0, &x1, &y1, &textWidth, &textHeight);
        
        // Fill background white and draw black text
        display->fillRect(0, 0, textWidth + 2, textHeight + 2, OLED_WHITE);
        display->setTextColor(OLED_BLACK);
        display->setCursor(1, 1);
        display->print(percentStr);
        display->setTextColor(OLED_WHITE); // Reset text color
    } else {
        // Normal percentage display in top-left corner
        display->setCursor(0, 0);
        display->print(percentStr);
    }
}

void Display::drawWeight(float weight) {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    display->clearDisplay();
    
    // Apply deadband to prevent flickering between 0.0g and -0.0g
    // Show 0.0g (without negative sign) when weight is between -0.1g and +0.1g
    float displayWeight = weight;
    if (weight >= -0.1 && weight <= 0.1) {
        displayWeight = 0.0; // Force to exactly 0.0 to avoid negative sign
    }
    
    // Format weight string with consistent spacing (without "g" unit)
    String weightStr;
    if (displayWeight < 0) {
        weightStr = String(displayWeight, 1); // Keep negative sign for values below -0.1g
    } else {
        weightStr = " " + String(displayWeight, 1); // Add space where negative sign would be
    }
    
    // Calculate text width for centering weight
    display->setTextSize(2);
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display->getTextBounds(weightStr, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    // Center the weight text horizontally
    int centerX = (SCREEN_WIDTH - textWidth) / 2;
    
    // Large weight display - centered at top
    display->setCursor(centerX, 0);
    display->print(weightStr);
    
    // Get flow rate and format it
    float currentFlowRate = 0.0;
    if (flowRatePtr != nullptr) {
        currentFlowRate = flowRatePtr->getFlowRate();
    }
    
    // Apply deadband to prevent flickering between 0.0g/s and -0.0g/s
    // Show 0.0g/s (without negative sign) when flow rate is between -0.1g/s and +0.1g/s
    float displayFlowRate = currentFlowRate;
    if (currentFlowRate >= -0.1 && currentFlowRate <= 0.1) {
        displayFlowRate = 0.0; // Force to exactly 0.0 to avoid negative sign
    }
    
    // Format flow rate string with consistent spacing (shorter format like Auto mode)
    String flowRateStr = "";
    if (displayFlowRate < 0) {
        flowRateStr += String(displayFlowRate, 1); // Keep negative sign for values below -0.1g/s
    } else {
        flowRateStr += String(displayFlowRate, 1); // No extra space needed for shorter format
    }
    flowRateStr += "g/s";

    // Small flow rate text at bottom left (same as Auto mode)
    display->setTextSize(1);
    display->setCursor(0, 24);
    display->print(flowRateStr);
    
    // Draw Bluetooth status if connected
    drawBluetoothStatus();
    
    // Draw battery status
    drawBatteryStatus();
    
    display->display();
}

/*
void Display::showWeightWithTimer(float weight) - REMOVED FUNCTION
Function removed as part of mode simplification - unified into showWeightWithFlowAndTimer()
*/

void Display::showWeightWithFlowAndTimer(float weight) {
    // Return early if display is not connected / showing message
    if (!displayConnected || showingMessage) {
        return;
    }
    
    // Apply deadband to prevent flickering between 0.0g and -0.0g
    float displayWeight = weight;
    if (weight >= -0.1 && weight <= 0.1) {
        displayWeight = 0.0;
    }
    
    // Right side: Timer and flow rate stacked (size 2)
    float currentTime = getTimerSeconds();

    float currentFlowRate = 0.0;
    if (flowRatePtr != nullptr) {
        currentFlowRate = flowRatePtr->getFlowRate();
    }
     // Apply deadband to flow rate
    float displayFlowRate = currentFlowRate;
    if (currentFlowRate >= -0.1 && currentFlowRate <= 0.1) {
        displayFlowRate = 0.0;
    }

    display->clearDisplay();
    display->setTextSize(1);
    
    if (SCREEN_HEIGHT < 64) {
        display->setFont(&FreeSans12pt7b);
        layoutAndPrintFloat(weight, 1, 48, 23);

        display->setFont(&FreeSans9pt7b);
        layoutAndPrintFloat(currentTime, 1, 105, 12);
        layoutAndPrintFloat(displayFlowRate, 1, 105, 28);
    } else {
        display->setFont(&FreeSans18pt7b);
        layoutAndPrintFloat(weight, 2, 73, 28);

        display->setFont(&FreeSans9pt7b);
        layoutAndPrintFloat(currentTime, 1, 46, 56);
        layoutAndPrintFloat(displayFlowRate, 1, 105, 56);
        display->drawLine(0, 36, 128, 36, 1);
        display->drawLine(68, 36, 68, 64, 1);
    }
    display->display();
    display->setFont(NULL);
}

// Timer management methods
void Display::startTimer() {
    if (!timerRunning) {
        // Fresh start
        timerStartTime = millis();
        timerRunning = true;
        timerPaused = false;
        
        // Start flow rate averaging when timer starts
        if (flowRatePtr != nullptr) {
            flowRatePtr->startTimerAveraging();
        }
    } else if (timerPaused) {
        // Resume from paused state
        timerStartTime = millis() - timerPausedTime;
        timerPaused = false;
        
        // Resume flow rate averaging when timer resumes
        if (flowRatePtr != nullptr) {
            flowRatePtr->startTimerAveraging();
        }
    }
    // If timer is already running and not paused, do nothing
}

void Display::stopTimer() {
    if (timerRunning && !timerPaused) {
        timerPausedTime = millis() - timerStartTime;
        timerPaused = true;
        
        // Stop flow rate averaging when timer stops
        if (flowRatePtr != nullptr) {
            flowRatePtr->stopTimerAveraging();
        }
    }
}

void Display::resetTimer() {
    timerStartTime = 0;
    timerPausedTime = 0;
    timerRunning = false;
    timerPaused = false;
    
    // Reset flow rate averaging when timer is reset
    if (flowRatePtr != nullptr) {
        flowRatePtr->resetTimerAveraging();
    }
}

bool Display::isTimerRunning() const {
    return timerRunning && !timerPaused;
}

float Display::getTimerSeconds() const {
    if (!timerRunning) {
        return 0.0;
    } else if (timerPaused) {
        return timerPausedTime / 1000.0;
    } else {
        return (millis() - timerStartTime) / 1000.0;
    }
}

void Display::showStatusPage() {
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }

    display->clearDisplay();
    display->setTextColor(OLED_WHITE);
    
    // Top line: Battery %, Scale icon, BLE icon
    display->setTextSize(1);
    
    // Battery percentage (left) - without "BAT:" prefix
    if (batteryPtr != nullptr) {
        int batteryPercent = batteryPtr->getBatteryPercentage();
        display->setCursor(0, 0);
        display->print(batteryPercent);
        display->print("%");
    } else {
        display->setCursor(0, 0);
        display->print("N/A");
    }
    
    // Scale status (center) - HX711 text with rectangle border when connected
    bool scaleConnected = (scalePtr != nullptr && scalePtr->isHX711Connected());
    display->setCursor(50, 0);
    display->print("HX711");
    if (scaleConnected) {
        // Draw rectangle around "HX711" when connected (with proper spacing)
        display->drawRect(48, -1, 34, 10, OLED_WHITE); // Rectangle around "HX711"
    }
    
    // Bluetooth status (right) - BT text with rectangle border when connected
    display->setCursor(110, 0);
    display->print("BT");
    if (bluetoothPtr != nullptr && bluetoothPtr->isConnected()) {
        // Draw rectangle around "BT" when connected (with proper spacing)
        display->drawRect(108, -1, 16, 10, OLED_WHITE); // Rectangle around "BT"
    }
    
    // Bottom line: WiFi mode and IP address (moved to very bottom)
    display->setTextSize(1);
    
    // Check WiFi connection status and show simplified format at bottom
    if (WiFi.status() == WL_CONNECTED) {
        display->setCursor(0, 24);  // Bottom of 32-pixel display
        display->print("STA: ");
        display->print(WiFi.localIP().toString());
    } else {
        // AP mode is active
        display->setCursor(0, 24);  // Bottom of 32-pixel display
        display->print("AP: ");
        display->print(WiFi.softAPIP().toString());
    }
    
    display->display();
}

void Display::toggleStatusPage() {
    showingStatusPage = !showingStatusPage;
    if (showingStatusPage) {
        statusPageStartTime = millis();
        showingMessage = false; // Clear any active message
        Serial.println("Showing status page");
    } else {
        Serial.println("Returning to main display");
    }
}

unsigned long Display::getElapsedTime() const {
    if (!timerRunning) {
        return 0;
    } else if (timerPaused) {
        return timerPausedTime;
    } else {
        return millis() - timerStartTime;
    }
}

void Display::showCenteredText(const String& line1, const String& line2, 
    uint8_t size1, uint8_t size2) {
    
    display->clearDisplay();
    display->setTextColor(OLED_WHITE);
    
    int16_t x1, y1;
    uint16_t w1, h1, w2, h2;
    
    // Get text bounds for both lines
    display->setTextSize(size1);
    // use 16 as x position for better compatibility with font baselines
    display->getTextBounds(line1, 16, 0, &x1, &y1, &w1, &h1);
    display->setTextSize(size2);
    display->getTextBounds(line2, 16, 0, &x1, &y1, &w2, &h2);
 
    // Calculate centered positions
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    int centerX2 = (SCREEN_WIDTH - w2) / 2;

    // Position lines to fit in 32 pixels
    int line1Y = 0;  // Start at top
    int line2Y = 16; // Second line at pixel 16
    if (size2 == 1) {
        // Position small text at bottom (24 pixels from top gives us 8 pixels for the text)
        line2Y = 24;
    }
    
    // Display first line
    display->setTextSize(size1);
    display->setCursor(centerX1, line1Y);
    display->print(line1);
    
    // Display second line
    display->setTextSize(size2);
    display->setCursor(centerX2, line2Y);
    display->print(line2);
    
    display->display();
}

void Display::splitFloat(float fval, String &intStr, String &decStr, int decimals) {
    
    if (decimals > 2) decimals = 2;

    // scale and round to requested precision
    long pow10 = (decimals == 1) ? 10L : 100L;
    long scaled = lroundf(fval * pow10);

    // ignore sign
    long absScaled = labs(scaled);

    // integer and decimal parts
    long integerPart = absScaled / pow10;
    long decimalPart = absScaled % pow10;

    // convert to strings
    intStr = String(integerPart);
    decStr = String(decimalPart);

    // left padding
    while (decStr.length() < decimals) {
        decStr = "0" + decStr;
    }
}

void Display::layoutAndPrintFloat(float value, 
            int precision, int16_t decimalSeparatorX, int16_t baselineY) {

    String intStr, weightDecStr;

    splitFloat(value, intStr, weightDecStr, precision);
    
    if (value < 0) {
        intStr = "-" + intStr + ".";
    } else {
        intStr = intStr + ".";
    }
    int16_t x1, y1;
    uint16_t w, h;
    // y=16 ist just the middle of the display
    display->getTextBounds(intStr, 16, baselineY, &x1, &y1, &w, &h);
    display->setCursor(decimalSeparatorX - w, baselineY);

    display->print(intStr);
    display->println(weightDecStr);
}

