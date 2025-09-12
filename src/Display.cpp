#include "Display.h"
#include "Scale.h"
#include "FlowRate.h"
#include "BluetoothScale.h"
#include "PowerManager.h"
#include "BatteryMonitor.h"
#include <WiFi.h>

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
    
    String line1 = "WeighMyBru";
    String line2 = "Starting";
    
    int16_t x1, y1;
    uint16_t w1, h1, w2, h2;
    
    // Get text bounds for both lines
    display->getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    display->getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
    
    // Calculate centered positions
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    int centerX2 = (SCREEN_WIDTH - w2) / 2;
    
    // Position lines to fit in 32 pixels
    int line1Y = 0;  // Start at top
    int line2Y = 16; // Second line at pixel 16
    
    // Display first line
    display->setCursor(centerX1, line1Y);
    display->print(line1);
    
    // Display second line
    display->setCursor(centerX2, line2Y);
    display->print(line2);
    
    display->display();
    
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
    
    // Show countdown in same large format as WeighMyBru Ready
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(OLED_WHITE);
    
    String line1 = "Sleep in";
    String line2 = String(seconds) + "...";
    
    int16_t x1, y1;
    uint16_t w1, h1, w2, h2;
    
    // Get text bounds for both lines
    display->getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    display->getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
    
    // Calculate centered positions
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    int centerX2 = (SCREEN_WIDTH - w2) / 2;
    
    // Position lines to fit in 32 pixels
    int line1Y = 0;  // Start at top
    int line2Y = 16; // Second line at pixel 16
    
    // Display first line
    display->setCursor(centerX1, line1Y);
    display->print(line1);
    
    // Display second line
    display->setCursor(centerX2, line2Y);
    display->print(line2);
    
    display->display();
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
    
    // Show sleep message with large top line and small bottom line
    display->clearDisplay();
    display->setTextColor(OLED_WHITE);
    
    // First line: "Sleep in 3" in large text (size 2)
    display->setTextSize(2);
    String line1 = "Sleeping..";
    
    int16_t x1, y1;
    uint16_t w1, h1;
    display->getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    
    display->setCursor(centerX1, 0);
    display->print(line1);
    
    // Second line: "Touch to cancel" in small text (size 1)
    display->setTextSize(1);
    String line2 = "Touch to cancel";
    
    uint16_t w2, h2;
    display->getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
    int centerX2 = (SCREEN_WIDTH - w2) / 2;
    
    // Position small text at bottom (24 pixels from top gives us 8 pixels for the text)
    display->setCursor(centerX2, 24);
    display->print(line2);
    
    display->display();
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
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(OLED_WHITE);
    
    // Calculate text positioning for centering
    String line1 = "Touch To";
    String line2 = "Wake Up";
    
    int16_t x1, y1;
    uint16_t w1, h1, w2, h2;
    
    // Get text bounds for both lines
    display->getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    display->getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
    
    // Calculate centered positions - tighter spacing for size 2 text
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    int centerX2 = (SCREEN_WIDTH - w2) / 2;
    
    // Position lines closer together to fit in 32 pixels
    int line1Y = 0;  // Start at top
    int line2Y = 16; // Second line at pixel 16
    
    // Display first line
    display->setCursor(centerX1, line1Y);
    display->print(line1);
    
    // Display second line
    display->setCursor(centerX2, line2Y);
    display->print(line2);
    
    display->display();
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
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(OLED_WHITE);
    
    // Calculate text positioning for centering
    String line1 = "Sleep";
    String line2 = "Cancelled";
    
    int16_t x1, y1;
    uint16_t w1, h1, w2, h2;
    
    // Get text bounds for both lines
    display->getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    display->getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
    
    // Calculate centered positions - tighter spacing for size 2 text
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    int centerX2 = (SCREEN_WIDTH - w2) / 2;
    
    // Position lines closer together to fit in 32 pixels
    int line1Y = 0;  // Start at top
    int line2Y = 16; // Second line at pixel 16
    
    // Display first line
    display->setCursor(centerX1, line1Y);
    display->print(line1);
    
    // Display second line
    display->setCursor(centerX2, line2Y);
    display->print(line2);
    
    display->display();
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
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(OLED_WHITE);
    
    // Since "Taring..." is a single word, we'll center it on one line
    // For consistency with WeighMyBru style, we can split it as "Taring" and "..."
    String line1 = "Taring";
    String line2 = "...";
    
    int16_t x1, y1;
    uint16_t w1, h1, w2, h2;
    
    // Get text bounds for both lines
    display->getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    display->getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
    
    // Calculate centered positions - tighter spacing for size 2 text
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    int centerX2 = (SCREEN_WIDTH - w2) / 2;
    
    // Position lines closer together to fit in 32 pixels
    int line1Y = 0;  // Start at top
    int line2Y = 16; // Second line at pixel 16
    
    // Display first line
    display->setCursor(centerX1, line1Y);
    display->print(line1);
    
    // Display second line
    display->setCursor(centerX2, line2Y);
    display->print(line2);
    
    display->display();
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
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(OLED_WHITE);
    
    // Split "Tared!" into two lines for better visual impact
    String line1 = "Scale";
    String line2 = "Tared!";
    
    int16_t x1, y1;
    uint16_t w1, h1, w2, h2;
    
    // Get text bounds for both lines
    display->getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    display->getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
    
    // Calculate centered positions - tighter spacing for size 2 text
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    int centerX2 = (SCREEN_WIDTH - w2) / 2;
    
    // Position lines closer together to fit in 32 pixels
    int line1Y = 0;  // Start at top
    int line2Y = 16; // Second line at pixel 16
    
    // Display first line
    display->setCursor(centerX1, line1Y);
    display->print(line1);
    
    // Display second line
    display->setCursor(centerX2, line2Y);
    display->print(line2);
    
    display->display();
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
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(OLED_WHITE);
    
    // Calculate text positioning for centering
    String line1 = "WeighMyBru";
    String line2 = "Ready";
    
    int16_t x1, y1;
    uint16_t w1, h1, w2, h2;
    
    // Get text bounds for both lines
    display->getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    display->getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
    
    // Calculate centered positions - tighter spacing for size 2 text
    int centerX1 = (SCREEN_WIDTH - w1) / 2;
    int centerX2 = (SCREEN_WIDTH - w2) / 2;
    
    // Position lines closer together to fit in 32 pixels
    int line1Y = 0;  // Start at top
    int line2Y = 16; // Second line at pixel 16
    
    // Display first line
    display->setCursor(centerX1, line1Y);
    display->print(line1);
    
    // Display second line
    display->setCursor(centerX2, line2Y);
    display->print(line2);
    
    display->display();
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
    // Return early if display is not connected
    if (!displayConnected) {
        return;
    }
    
    // If we're showing a message, don't override it with weight display
    if (showingMessage) {
        return;
    }
    
    // Declare variables used throughout function
    int16_t x1, y1;
    uint16_t w, h;
    
    display->clearDisplay();
    
    // Apply deadband to prevent flickering between 0.0g and -0.0g
    float displayWeight = weight;
    if (weight >= -0.1 && weight <= 0.1) {
        displayWeight = 0.0;
    }
    
    // Split weight into integer and decimal parts for custom rendering
    bool isNegative = displayWeight < 0;
    float absWeight = abs(displayWeight);
    int integerPart = (int)absWeight;
    int decimalPart = (int)((absWeight - integerPart) * 10 + 0.5); // Round to 1 decimal
    
    // Handle carry-over when decimal part rounds to 10 (e.g., 4.95 -> 5.0)
    if (decimalPart >= 10) {
        integerPart += 1;
        decimalPart = 0;
    }
    
    // Draw weight with custom decimal point - positioned at left middle
    display->setTextSize(3);
    int weightY = 5; // Middle of 32-pixel screen (size 3 text is ~21px tall, so (32-21)/2 ≈ 5)
    display->setCursor(0, weightY);
    
    // Draw negative sign if needed
    int currentX = 0;
    if (isNegative) {
        display->print("-");
        // Calculate width of "-" in size 3
        display->getTextBounds("-", 0, 0, &x1, &y1, &w, &h);
        currentX += w;
    }
    
    // Draw integer part in size 3
    String intStr = String(integerPart);
    display->setCursor(currentX, weightY);
    display->print(intStr);
    
    // Calculate position after integer part
    display->getTextBounds(intStr, 0, 0, &x1, &y1, &w, &h);
    currentX += w;
    
    // Draw smaller decimal point (size 1) positioned to align with baseline
    display->setTextSize(1);
    display->setCursor(currentX, weightY + 11); // Offset from weight baseline for alignment
    display->print(".");
    display->getTextBounds(".", 0, 0, &x1, &y1, &w, &h);
    currentX += w;
    
    // Draw decimal digit in size 2 for better readability
    display->setTextSize(2);
    display->setCursor(currentX, weightY + 3); // Positioned relative to weight baseline
    display->print(String(decimalPart));
    
    // Right side: Timer and flow rate stacked (size 2)
    display->setTextSize(2);
    
    // Get timer value and format without "s"
    float currentTime = getTimerSeconds();
    
    // Get flow rate and format without "g/s"
    float currentFlowRate = 0.0;
    if (flowRatePtr != nullptr) {
        currentFlowRate = flowRatePtr->getFlowRate();
    }
    
    // Apply deadband to flow rate
    float displayFlowRate = currentFlowRate;
    if (currentFlowRate >= -0.1 && currentFlowRate <= 0.1) {
        displayFlowRate = 0.0;
    }
    
    // === CUSTOM TIMER RENDERING (like weight) ===
    bool timerNegative = currentTime < 0;
    float absTimer = abs(currentTime);
    int timerInteger = (int)absTimer;
    int timerDecimal = (int)((absTimer - timerInteger) * 10 + 0.5);
    
    // Handle timer carry-over
    if (timerDecimal >= 10) {
        timerInteger += 1;
        timerDecimal = 0;
    }
    
    // Calculate timer position with "T" label at far right
    display->setTextSize(2);
    String timerIntStr = String(timerInteger);
    if (timerNegative) timerIntStr = "-" + timerIntStr;
    
    uint16_t timerIntWidth, timerDecWidth, timerH, timerLabelWidth;
    display->getTextBounds(timerIntStr, 0, 0, &x1, &y1, &timerIntWidth, &timerH);
    display->setTextSize(1);
    display->getTextBounds("T", 0, 0, &x1, &y1, &timerLabelWidth, &timerH);
    display->getTextBounds(".", 0, 0, &x1, &y1, &w, &timerH);
    uint16_t timerDotWidth = w;
    display->getTextBounds(String(timerDecimal), 0, 0, &x1, &y1, &timerDecWidth, &timerH);
    
    // Position "T" at far right, numbers to the left
    int timerLabelX = SCREEN_WIDTH - timerLabelWidth;
    int timerStartX = timerLabelX - timerIntWidth - timerDotWidth - timerDecWidth;
    
    // Draw timer integer part (size 2)
    display->setTextSize(2);
    display->setCursor(timerStartX, 0);
    display->print(timerIntStr);
    
    // Draw timer decimal point (size 1)
    display->setTextSize(1);
    display->setCursor(timerStartX + timerIntWidth, 7); // Aligned with size 2 baseline
    display->print(".");
    
    // Draw timer decimal digit (size 1)
    display->setCursor(timerStartX + timerIntWidth + timerDotWidth, 7);
    display->print(String(timerDecimal));
    
    // Draw "T" label at far right (size 1)
    display->setTextSize(1);
    display->setCursor(timerLabelX, 0); // Far right position
    display->print("T");
    
    // === CUSTOM FLOW RATE RENDERING (like weight) ===
    bool flowNegative = displayFlowRate < 0;
    float absFlow = abs(displayFlowRate);
    int flowInteger = (int)absFlow;
    int flowDecimal = (int)((absFlow - flowInteger) * 10 + 0.5);
    
    // Handle flow rate carry-over
    if (flowDecimal >= 10) {
        flowInteger += 1;
        flowDecimal = 0;
    }
    
    // Calculate flow rate position with "F" label at far right
    display->setTextSize(2);
    String flowIntStr = String(flowInteger);
    if (flowNegative) flowIntStr = "-" + flowIntStr;
    
    uint16_t flowIntWidth, flowDecWidth, flowH, flowLabelWidth;
    display->getTextBounds(flowIntStr, 0, 0, &x1, &y1, &flowIntWidth, &flowH);
    display->setTextSize(1);
    display->getTextBounds("F", 0, 0, &x1, &y1, &flowLabelWidth, &flowH);
    display->getTextBounds(".", 0, 0, &x1, &y1, &w, &flowH);
    uint16_t flowDotWidth = w;
    display->getTextBounds(String(flowDecimal), 0, 0, &x1, &y1, &flowDecWidth, &flowH);
    
    // Position "F" at far right, numbers to the left
    int flowLabelX = SCREEN_WIDTH - flowLabelWidth;
    int flowStartX = flowLabelX - flowIntWidth - flowDotWidth - flowDecWidth;
    
    // Draw flow rate integer part (size 2)
    display->setTextSize(2);
    display->setCursor(flowStartX, 16); // Below timer
    display->print(flowIntStr);
    
    // Draw flow rate decimal point (size 1)
    display->setTextSize(1);
    display->setCursor(flowStartX + flowIntWidth, 23); // Aligned with size 2 baseline
    display->print(".");
    
    // Draw flow rate decimal digit (size 1)
    display->setCursor(flowStartX + flowIntWidth + flowDotWidth, 23);
    display->print(String(flowDecimal));
    
    // Draw "F" label at far right (size 1)
    display->setTextSize(1);
    display->setCursor(flowLabelX, 16); // Far right position, below timer
    display->print("F");
    
    display->display();
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
