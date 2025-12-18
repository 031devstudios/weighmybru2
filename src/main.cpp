#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <esp_sleep.h>
#ifdef ESP_IDF_VERSION_MAJOR
    #include "esp_wifi.h"
    #include "esp_err.h"
#endif
#include "WebServer.h"
#include "Scale.h"
#include "WiFiManager.h"
#include "FlowRate.h"
#include "Calibration.h"
#include "BluetoothScale.h"
#include "TouchSensor.h"
#include "Display.h"
#include "PowerManager.h"
#include "BatteryMonitor.h"
#include "BoardConfig.h"
#include "Version.h"

// Board-specific pin configuration
uint8_t dataPin = HX711_DATA_PIN;     // HX711 Data pin
uint8_t clockPin = HX711_CLOCK_PIN;   // HX711 Clock pin  
uint8_t touchPin = TOUCH_TARE_PIN;    // Touch sensor for tare
uint8_t sleepTouchPin = TOUCH_SLEEP_PIN;  // Touch sensor for sleep functionality
uint8_t batteryPin = BATTERY_PIN;     // Battery voltage monitoring
uint8_t sdaPin = I2C_SDA_PIN;         // I2C Data pin for display
uint8_t sclPin = I2C_SCL_PIN;         // I2C Clock pin for display
float calibrationFactor = 4195.712891;
Scale scale(dataPin, clockPin, calibrationFactor);
FlowRate flowRate;
BluetoothScale bluetoothScale;
TouchSensor touchSensor(touchPin, &scale);
Display oledDisplay(sdaPin, sclPin, &scale, &flowRate);
PowerManager powerManager(sleepTouchPin, &oledDisplay);
BatteryMonitor batteryMonitor(batteryPin);

void setup() {
  Serial.begin(115200);
  
  // Set CPU frequency explicitly for power optimization
  setCpuFrequencyMhz(80);
  Serial.printf("CPU frequency set to: %dMHz for power optimization\n", getCpuFrequencyMhz());
  
  // Version and board identification
  Serial.println("=================================");
  Serial.printf("WeighMyBru² v%s\n", WEIGHMYBRU_VERSION_STRING);
  Serial.printf("Board: %s\n", WEIGHMYBRU_BOARD_NAME);
  Serial.printf("Build: %s %s\n", WEIGHMYBRU_BUILD_DATE, WEIGHMYBRU_BUILD_TIME);
  Serial.printf("Full Version: %s\n", WEIGHMYBRU_FULL_VERSION);
  Serial.printf("Flash Size: %dMB\n", FLASH_SIZE_MB);
  Serial.printf("CPU Frequency: %dMHz (Power Optimized)\n", getCpuFrequencyMhz());
  Serial.println("=================================");
  
  // Link scale and flow rate for tare operation coordination
  scale.setFlowRatePtr(&flowRate);
  
  // Check for factory reset request (hold touch pin during boot)
  pinMode(touchPin, INPUT_PULLDOWN);
  if (digitalRead(touchPin) == HIGH) {
    Serial.println("FACTORY RESET: Touch pin held during boot - clearing WiFi credentials");
    clearWiFiCredentials();
    delay(1000);
  }
  
  // CRITICAL: Initialize BLE FIRST before WiFi to prevent radio conflicts
  Serial.println("Initializing BLE FIRST for GaggiMate compatibility...");
  Serial.printf("Free heap before BLE init: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM before BLE init: %u bytes\n", ESP.getFreePsram());
  
  try {
    bluetoothScale.begin();  // Initialize BLE without scale reference
    Serial.println("BLE initialized successfully - GaggiMate should be able to connect");
    Serial.printf("Free heap after BLE init: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM after BLE init: %u bytes\n", ESP.getFreePsram());
  } catch (...) {
    Serial.println("BLE initialization failed - continuing without Bluetooth");
    Serial.printf("Free heap after BLE fail: %u bytes\n", ESP.getFreeHeap());
  }
  
  // Initialize display with error handling - don't block if display fails
  Serial.println("Initializing display...");
  bool displayAvailable = oledDisplay.begin();
  
  if (!displayAvailable) {
    Serial.println("WARNING: Display initialization failed!");
    Serial.println("System will continue in headless mode without display.");
    Serial.println("All functionality remains available via web interface.");
  } else {
    Serial.println("Display initialized - ready for visual feedback");
    // Set reduced brightness for power optimization
    oledDisplay.setBrightness(128);  // 50% brightness vs 255 max
    Serial.println("Display brightness set to 50% for power optimization");
  }
  
  // Check wake-up reason and show appropriate message
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal (touch sensor)");
      // Show the same starting message as normal boot for consistency
      delay(1500);
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
    default:
      Serial.println("Wakeup was not caused by deep sleep: " + String(wakeup_reason));
      // For normal startup, the begin() method already shows a startup message
      delay(1000);
      break;
  }
  //Wait for BLE to finish intitalizing before starting WiFi
  delay(1500); 
  
  // Initialize WiFi power management BEFORE any WiFi operations
  Serial.println("Initializing WiFi power management...");
  
  // CRITICAL: Force WiFi completely off first to ensure clean state
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1000); // Allow hardware to fully reset
  
  // Debug: Uncomment the next line to force reset WiFi state for testing
  // resetWiFiEnabledState(); // DISABLED - state has been cleared
  
  // ALWAYS enable WiFi power management for optimal battery life
  WiFi.setSleep(true);
  Serial.println("WiFi power management enabled for battery optimization");
  
  // CRITICAL: Always setup WiFi first (like tare button scenario)
  // This ensures all WiFi subsystems are properly initialized
  // Then disable it cleanly if needed (replicating tare button sequence)
  Serial.println("FORCING WiFi initialization to replicate tare button scenario...");
  setupWiFiForced(); // Use forced setup to bypass state checks

  // Wait for WiFi to fully stabilize after BLE is already running
  delay(1500);
  Serial.printf("Version: %s\n", ESP.getSdkVersion());
  // Initialize scale with error handling - don't block web server if HX711 fails
  Serial.println("Initializing scale...");
  if (!scale.begin()) {
    Serial.println("WARNING: Scale (HX711) initialization failed!");
    Serial.println("Web server will continue to run, but scale readings will not be available.");
    Serial.println("Check HX711 wiring and connections.");
  } else {
    Serial.println("Scale initialized successfully");
    // Now that scale is ready, set the reference in BluetoothScale
    bluetoothScale.setScale(&scale);
  }
  
  // BLE was initialized earlier - no need to initialize again
  // bluetoothScale.begin(&scale);
  
  // Set bluetooth reference in display for status indicator (if display available)
  if (oledDisplay.isConnected()) {
    oledDisplay.setBluetoothScale(&bluetoothScale);
  }
  
  // Set display reference in bluetooth for timer control
  bluetoothScale.setDisplay(&oledDisplay);
  
  // Set power manager reference in display for timer state synchronization (if display available)
  if (oledDisplay.isConnected()) {
    oledDisplay.setPowerManager(&powerManager);
  }
  
  // Set battery monitor reference in display for battery status (if display available)
  if (oledDisplay.isConnected()) {
    oledDisplay.setBatteryMonitor(&batteryMonitor);
  }

  // Initialize touch sensor
  touchSensor.begin();

  // Initialize power manager
  powerManager.begin();

  // Initialize battery monitor
  batteryMonitor.begin();

  // Show IP addresses and welcome message if display is available
  delay(100); // Small delay to ensure WiFi is fully initialized
  if (oledDisplay.isConnected()) {
    oledDisplay.showIPAddresses();
  }

  // Link display to touch sensor for tare feedback (if display available)
  if (oledDisplay.isConnected()) {
    touchSensor.setDisplay(&oledDisplay);
  }
  
  // Link flow rate to touch sensor for averaging reset on tare
  touchSensor.setFlowRate(&flowRate);

  setupWebServer(scale, flowRate, bluetoothScale, oledDisplay, batteryMonitor);
  
  // CRITICAL: After full initialization, check if WiFi should be disabled
  // This exactly replicates the tare button scenario: WiFi started, then disabled
  Serial.println("=== POST-INITIALIZATION WiFi STATE CHECK ===");
  if (!loadWiFiEnabledState()) {
    Serial.println("WiFi should be disabled - applying clean shutdown like tare button");
    Serial.println("(WiFi was initialized first, now disabling cleanly)");
    
    // Small delay to ensure all systems are stable (like tare button timing)
    delay(100);
    
    // Now call disableWiFi() exactly like tare button does
    disableWiFi();
    
    Serial.println("WiFi cleanly disabled - 0.05A power consumption expected");
  } else {
    Serial.println("WiFi should remain enabled - no action needed");
  }
}

void loop() {
  static unsigned long lastWeightUpdate = 0;
  static unsigned long lastWiFiCheck = 0;
  static unsigned long lastDisplayUpdate = 0;
  
  // Update weight at reduced frequency for power optimization
  if (millis() - lastWeightUpdate >= 50) { // Reduced from 20ms to 50ms (20Hz from 50Hz)
    float weight = scale.getWeight();
    flowRate.update(weight);
    lastWeightUpdate = millis();
  }
  
  static unsigned long lastBLEUpdate = 0;
  
  // Check WiFi status every 30 seconds for debugging
  if (millis() - lastWiFiCheck >= 30000) {
    printWiFiStatus();
    lastWiFiCheck = millis();
  }
  
  // Maintain WiFi AP stability
  maintainWiFi();
  
  // Update Bluetooth less frequently to reduce BLE interference and power usage
  if (millis() - lastBLEUpdate >= 100) { // Reduced from 50ms to 100ms (10Hz from 20Hz)
    bluetoothScale.update();
    lastBLEUpdate = millis();
  }
  
  // Update touch sensor
  touchSensor.update();
  
  // Update power manager
  powerManager.update();
  
  // Update battery monitor
  batteryMonitor.update();
  
  // Update display less frequently for power saving
  if (millis() - lastDisplayUpdate >= 100) { // Reduced display refresh rate to 10Hz
    oledDisplay.update();
    lastDisplayUpdate = millis();
  }
  
  // Increased delay for better power efficiency while maintaining responsiveness
  delay(10); // Optimized delay: 10ms for good responsiveness with power savings
}
