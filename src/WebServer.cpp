#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "WebServer.h"
#include "Scale.h"
#include "WiFiManager.h"
#include <Preferences.h>
#include "FlowRate.h"
#include "Calibration.h"
#include "BluetoothScale.h"

Preferences preferences;

// Cache for display settings to avoid repeated slow EEPROM reads
static int cachedDecimals = -1; // -1 indicates not cached yet
static unsigned long lastDecimalCacheTime = 0;
const unsigned long DECIMAL_CACHE_TIMEOUT = 300000; // 5 minutes cache timeout

int getCachedDecimals() {
    // Fast path - return immediately if already cached and recent
    if (cachedDecimals != -1 && (millis() - lastDecimalCacheTime < DECIMAL_CACHE_TIMEOUT)) {
        return cachedDecimals;
    }
    
    unsigned long startTime = millis();
    
    if (preferences.begin("display", false)) {
        cachedDecimals = preferences.getInt("decimals", 1);
        preferences.end();
        lastDecimalCacheTime = millis();
        Serial.printf("Display: OK in %lums\n", millis() - startTime);
    } else {
        cachedDecimals = 1; // Use default
        lastDecimalCacheTime = millis();
        Serial.println("Display: FAIL");
    }
    
    return cachedDecimals;
}

void setCachedDecimals(int decimals) {
    Serial.println("Saving decimal setting...");
    unsigned long startTime = millis();
    
    if (preferences.begin("display", false)) {
        preferences.putInt("decimals", decimals);
        preferences.end();
        cachedDecimals = decimals; // Update cache
        lastDecimalCacheTime = millis();
        Serial.printf("Decimal setting saved in %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Failed to save decimal setting to EEPROM");
    }
}

void diagnoseEEPROMPerformance() {
    Serial.println("=== EEPROM Performance Diagnostics ===");
    
    // Test WiFi preferences
    unsigned long startTime = millis();
    Preferences testPrefs;
    if (testPrefs.begin("test", false)) {
        testPrefs.putInt("testkey", 42);
        int val = testPrefs.getInt("testkey", 0);
        testPrefs.end();
        Serial.printf("EEPROM test write/read took: %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Cannot open test preferences namespace");
    }
    
    // Test existing namespaces
    startTime = millis();
    if (testPrefs.begin("wifi", true)) {
        String ssid = testPrefs.getString("ssid", "");
        testPrefs.end();
        Serial.printf("WiFi namespace read took: %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Cannot open wifi preferences namespace");
    }
    
    startTime = millis();
    if (testPrefs.begin("display", true)) {
        int decimals = testPrefs.getInt("decimals", 1);
        testPrefs.end();
        Serial.printf("Display namespace read took: %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Cannot open display preferences namespace");
    }
    
    Serial.println("=== End Diagnostics ===");
}

AsyncWebServer server(80);

/*
 * API Endpoints for External Brewing Systems (e.g., GaggiMate):
 * 
 * Ultra-fast weight reading (minimal latency):
 * GET /api/brew/weight
 * Response: "45.2" (weight in grams, 1 decimal)
 * 
 * Fast brewing status:
 * GET /api/brew/status  
 * Response: {"w":45.2,"f":2.1} (weight and flowrate)
 * 
 * Standard dashboard:
 * GET /api/dashboard
 * Response: {"weight":45.23,"flowrate":2.15}
 */

void setupWebServer(Scale &scale, FlowRate &flowRate, BluetoothScale &bluetoothScale, OledDisplay &display, BatteryMonitor &battery) {
  if (!LittleFS.begin()) {
    Serial.println();
    Serial.println("=====================================");
    Serial.println("FILESYSTEM NOT FOUND!");
    Serial.println("=====================================");
    Serial.println("The LittleFS filesystem failed to mount.");
    Serial.println("This means the web interface files are missing.");
    Serial.println();
    Serial.println("To fix this, please run:");
    Serial.println("  pio run -t uploadfs");
    Serial.println();
    Serial.println("Or in PlatformIO IDE:");
    Serial.println("  Project Tasks → Platform → Upload Filesystem Image");
    Serial.println();
    Serial.println("The scale will continue to work, but the web interface will be unavailable.");
    Serial.println("=====================================");
    Serial.println();
    return;
  }

  // Run EEPROM diagnostics
  diagnoseEEPROMPerformance();

  // Pre-cache settings to avoid delays on first page load
  Serial.println("Pre-caching settings for faster page loads...");
  getCachedDecimals();        // This will cache the decimal setting
  getStoredSSID();            // This will cache WiFi credentials

  // Register API route first
  server.on("/api/dashboard", HTTP_GET, [&scale, &flowRate, &display, &battery, &bluetoothScale](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"weight\":" + String(scale.getCurrentWeight(), 2) + ",";
    json += "\"flowrate\":" + String(flowRate.getFlowRate(), 1) + ",";
    json += "\"scale_connected\":" + String(scale.isHX711Connected() ? "true" : "false") + ",";
    json += "\"filter_state\":\"" + scale.getFilterState() + "\",";
    
    // Always show unified mode
    json += "\"mode\":\"UNIFIED\",";
    
    // Add timer information
    unsigned long elapsedTime = display.getElapsedTime();
    if (elapsedTime > 0 || display.isTimerRunning()) {
      unsigned long minutes = elapsedTime / 60000;
      unsigned long seconds = (elapsedTime % 60000) / 1000;
      unsigned long milliseconds = elapsedTime % 1000;
      json += "\"timer_running\":" + String(display.isTimerRunning() ? "true" : "false") + ",";
      json += "\"timer_elapsed\":" + String(elapsedTime) + ",";
      json += "\"timer_display\":\"" + String(minutes) + ":" + 
              (seconds < 10 ? "0" : "") + String(seconds) + "." + 
              (milliseconds < 100 ? (milliseconds < 10 ? "00" : "0") : "") + String(milliseconds) + "\",";
      
      // Add timer average flow rate
      if (flowRate.hasTimerAverage()) {
        json += "\"timer_avg_flowrate\":" + String(flowRate.getTimerAverageFlowRate(), 2);
      } else {
        json += "\"timer_avg_flowrate\":null";
      }
    } else {
      json += "\"timer_running\":false,";
      json += "\"timer_elapsed\":0,";
      json += "\"timer_display\":\"0:00.000\",";
      json += "\"timer_avg_flowrate\":null";
    }
    
    // Add battery information
    json += ",\"battery_voltage\":" + String(battery.getBatteryVoltage(), 2);
    json += ",\"battery_percentage\":" + String(battery.getBatteryPercentage());
    json += ",\"battery_status\":\"" + battery.getBatteryStatus() + "\"";
    json += ",\"battery_segments\":" + String(battery.getBatterySegments());
    json += ",\"battery_low\":" + String(battery.isLowBattery() ? "true" : "false");
    json += ",\"battery_critical\":" + String(battery.isCriticalBattery() ? "true" : "false");
    
    // Add signal strength information
    json += ",\"wifi_signal_strength\":" + String(getWiFiSignalStrength());
    json += ",\"wifi_signal_quality\":\"" + getWiFiSignalQuality() + "\"";
    json += ",\"bluetooth_connected\":" + String(bluetoothScale.isConnected() ? "true" : "false");
    json += ",\"bluetooth_signal_strength\":" + String(bluetoothScale.getBluetoothSignalStrength());
    
    json += "}";
    request->send(200, "application/json", json);
  });

  // Timer control endpoints
  server.on("/api/timer/start", HTTP_POST, [&display](AsyncWebServerRequest *request) {
    display.startTimer();
    request->send(200, "text/plain", "Timer started");
  });

  server.on("/api/timer/stop", HTTP_POST, [&display](AsyncWebServerRequest *request) {
    display.stopTimer();
    request->send(200, "text/plain", "Timer stopped");
  });

  server.on("/api/timer/reset", HTTP_POST, [&display](AsyncWebServerRequest *request) {
    display.resetTimer();
    request->send(200, "text/plain", "Timer reset");
  });

  server.on("/api/weight", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(scale.getCurrentWeight()));
  });

  // Lightweight weight-only endpoint for brewing applications
  server.on("/api/weight-fast", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    // Minimal processing for fastest response
    request->send(200, "text/plain", String(scale.getCurrentWeight(), 2));
  });

  // Brewing mode endpoints for external devices like GaggiMate
  server.on("/api/brew/weight", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    // Ultra-fast response for brewing systems
    float weight = scale.getCurrentWeight();
    request->send(200, "text/plain", String(weight, 1)); // 1 decimal for speed
  });
  
  server.on("/api/brew/status", HTTP_GET, [&scale, &flowRate](AsyncWebServerRequest *request) {
    // Minimal JSON for brewing systems
    String json = "{\"w\":" + String(scale.getCurrentWeight(), 1) + 
                  ",\"f\":" + String(flowRate.getFlowRate(), 1) + "}";
    request->send(200, "application/json", json);
  });

  // Battery calibration endpoints (must be before general /api/battery route)
  server.on("/api/battery/calibrate", HTTP_POST, [&battery](AsyncWebServerRequest *request) {
    if (request->hasParam("actualVoltage", true)) {
      String value = request->getParam("actualVoltage", true)->value();
      float actualVoltage = value.toFloat();
      if (actualVoltage > 0.0f && actualVoltage <= 5.0f) {
        battery.calibrateVoltage(actualVoltage);
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"Battery calibrated to " + String(actualVoltage, 3) + "V\",";
        json += "\"new_voltage\":" + String(battery.getBatteryVoltage(), 3) + ",";
        json += "\"new_percentage\":" + String(battery.getBatteryPercentage()) + ",";
        json += "\"calibration_offset\":" + String(battery.getCalibrationOffset(), 3);
        json += "}";
        request->send(200, "application/json", json);
      } else {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid voltage. Must be between 0.1V and 5.0V\"}");
      }
    } else {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'actualVoltage' parameter\"}");
    }
  });

  // GET version for easy browser access
  server.on("/api/battery/calibrate", HTTP_GET, [&battery](AsyncWebServerRequest *request) {
    if (request->hasParam("voltage")) {
      String value = request->getParam("voltage")->value();
      float actualVoltage = value.toFloat();
      if (actualVoltage > 0.0f && actualVoltage <= 5.0f) {
        float beforeVoltage = battery.getBatteryVoltage();
        int beforePercentage = battery.getBatteryPercentage();
        
        battery.calibrateVoltage(actualVoltage);
        
        float afterVoltage = battery.getBatteryVoltage();
        int afterPercentage = battery.getBatteryPercentage();
        
        String json = "{";
        json += "\"status\":\"success\",";
        json += "\"message\":\"Battery calibrated successfully\",";
        json += "\"before_voltage\":" + String(beforeVoltage, 3) + ",";
        json += "\"before_percentage\":" + String(beforePercentage) + ",";
        json += "\"after_voltage\":" + String(afterVoltage, 3) + ",";
        json += "\"after_percentage\":" + String(afterPercentage) + ",";
        json += "\"target_voltage\":" + String(actualVoltage, 3) + ",";
        json += "\"calibration_offset\":" + String(battery.getCalibrationOffset(), 3);
        json += "}";
        request->send(200, "application/json", json);
        Serial.printf("Battery calibrated via GET: %.3fV (was %.3fV, now %.3fV)\n", actualVoltage, beforeVoltage, afterVoltage);
      } else {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid voltage. Must be between 0.1V and 5.0V\"}");
      }
    } else {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'voltage' parameter. Use ?voltage=4.30\"}");
    }
  });

  // Battery monitoring endpoint (general status)
  server.on("/api/battery", HTTP_GET, [&battery](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"voltage\":" + String(battery.getBatteryVoltage(), 3);
    json += ",\"percentage\":" + String(battery.getBatteryPercentage());
    json += ",\"status\":\"" + battery.getBatteryStatus() + "\"";
    json += ",\"segments\":" + String(battery.getBatterySegments());
    json += ",\"low_battery\":" + String(battery.isLowBattery() ? "true" : "false");
    json += ",\"critical_battery\":" + String(battery.isCriticalBattery() ? "true" : "false");
    json += ",\"charging\":" + String(battery.isCharging() ? "true" : "false");
    json += ",\"calibration_offset\":" + String(battery.getCalibrationOffset(), 3);
    json += "}";
    request->send(200, "application/json", json);
  });

  // Battery debug endpoint for troubleshooting
  server.on("/api/battery/debug", HTTP_GET, [&battery](AsyncWebServerRequest *request) {
    // We need to expose the raw ADC reading for debugging
    // Let's create a temporary battery instance to get raw data
    int rawADC = analogRead(7); // GPIO7 battery pin
    float rawVoltage = ((float)rawADC / 4095.0f) * 3.3f;
    float dividedVoltage = rawVoltage * 2.0f; // Apply voltage divider ratio
    
    String json = "{";
    json += "\"raw_adc\":" + String(rawADC) + ",";
    json += "\"raw_voltage\":" + String(rawVoltage, 3) + ",";
    json += "\"divided_voltage\":" + String(dividedVoltage, 3) + ",";
    json += "\"calibrated_voltage\":" + String(battery.getBatteryVoltage(), 3) + ",";
    json += "\"calibration_offset\":" + String(battery.getCalibrationOffset(), 3) + ",";
    json += "\"percentage\":" + String(battery.getBatteryPercentage());
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/tare", HTTP_POST, [&scale, &display, &flowRate](AsyncWebServerRequest *request){
    scale.tare(20);
    
    // Reset timer when taring (prepare for fresh brew)
    display.resetTimer();
    
    // Reset flow rate averaging for fresh brew measurement
    flowRate.resetTimerAveraging();
    
    request->send(200, "text/plain", "Scale tared! Timer and flow rate reset for fresh brew.");
  });

  server.on("/api/set-calibrationfactor", HTTP_POST, [&scale](AsyncWebServerRequest *request){
  if (request->hasParam("calibrationfactor", true)) {
    String value = request->getParam("calibrationfactor", true)->value();
    float calibrationFactor = value.toFloat();
    Serial.printf("Updated calibration factor weight: %.2f\n", calibrationFactor);
    request->send(200, "text/plain", "Calibration factor updated to " + value);
    scale.set_scale(calibrationFactor); // Assuming you have a method to set the calibration factor in Scale class
  } else {
    request->send(400, "text/plain", "Missing 'calibrationfactor' parameter");
  }
});

  server.on("/api/calibrate", HTTP_POST, [&scale](AsyncWebServerRequest *request){
    if (request->hasParam("knownWeight", true)) {
      String value = request->getParam("knownWeight", true)->value();
      float knownWeight = value.toFloat();
      // Read raw value from the scale (uncalibrated)
      long raw = scale.getRawValue();
      if (knownWeight > 0 && raw != 0) {
        float newCalibrationFactor = (float)raw / knownWeight;
        scale.set_scale(newCalibrationFactor);
        Serial.printf("Calibration complete. New factor: %.6f\n", newCalibrationFactor);
        request->send(200, "text/plain", "Scale calibrated! New factor: " + String(newCalibrationFactor, 6));
      } else {
        request->send(400, "text/plain", "Invalid known weight or scale reading");
      }
    } else {
      request->send(400, "text/plain", "Missing 'knownWeight' parameter");
    }
  });

  server.on("/api/calibrationfactor", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(scale.getCalibrationFactor(), 6));
  });

  // Scale connection status endpoint
  server.on("/api/scale/status", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"connected\":" + String(scale.isHX711Connected() ? "true" : "false") + ",";
    json += "\"weight\":" + String(scale.getCurrentWeight(), 2) + ",";
    json += "\"raw_value\":" + String(scale.getRawValue()) + ",";
    json += "\"calibration_factor\":" + String(scale.getCalibrationFactor(), 6);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/wifi-creds", HTTP_GET, [](AsyncWebServerRequest *request) {
    String ssid = getStoredSSID();
    String password = getStoredPassword();
    String json = "{\"ssid\":\"" + ssid + "\",\"password\":\"" + password + "\"}";
    request->send(200, "application/json", json);
  });

  server.on("/api/wifi-creds", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();
      
      Serial.println("New WiFi credentials received via web interface");
      
      // Save credentials first
      saveWiFiCredentials(ssid.c_str(), password.c_str());
      
      // Attempt immediate STA connection to avoid needing a reboot
      bool connected = attemptSTAConnection(ssid.c_str(), password.c_str());
      
      if (connected) {
        request->send(200, "application/json", 
          "{\"status\":\"success\",\"message\":\"Connected successfully! AP mode disabled for power savings.\",\"ip\":\"" + WiFi.localIP().toString() + "\"}");
      } else {
        // Connection failed - switch back to AP mode
        switchToAPMode();
        request->send(200, "application/json", 
          "{\"status\":\"failed\",\"message\":\"Connection failed. Check credentials and try again. AP mode restored.\"}");
      }
    } else {
      request->send(400, "text/plain", "Missing SSID or password");
    }
  });

  server.on("/api/wifi-creds", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    clearWiFiCredentials();
    request->send(200, "text/plain", "WiFi credentials cleared. Reboot to apply changes.");
  });

  // WiFi Power Management endpoints
  server.on("/api/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"enabled\":" + String(isWiFiEnabled() ? "true" : "false") + ",";
    json += "\"connected\":" + String((WiFi.status() == WL_CONNECTED) ? "true" : "false");
    if (WiFi.status() == WL_CONNECTED) {
      json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
    }
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/wifi-toggle", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool currentlyEnabled = isWiFiEnabled() && WiFi.getMode() != WIFI_OFF;
    
    if (currentlyEnabled) {
      // Send response before disabling WiFi
      request->send(200, "text/plain", "WiFi disabled for battery saving. Device will be inaccessible until WiFi is re-enabled.");
      // Add delay to allow response to be sent, then disable WiFi
      delay(100);
      disableWiFi();
    } else {
      enableWiFi();
      request->send(200, "text/plain", "WiFi enabled");
    }
  });

  server.on("/api/wifi-enable", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("enabled", true)) {
      bool enabled = request->getParam("enabled", true)->value() == "true";
      if (enabled) {
        enableWiFi();
        request->send(200, "text/plain", "WiFi enabled");
      } else {
        // Send response before disabling WiFi
        request->send(200, "text/plain", "WiFi disabled for battery saving. Device will be inaccessible until WiFi is re-enabled.");
        // Add delay to allow response to be sent, then disable WiFi
        delay(100);
        disableWiFi();
      }
    } else {
      request->send(400, "text/plain", "Missing enabled parameter");
    }
  });

  // Signal strength endpoint for WiFi and Bluetooth monitoring
  server.on("/api/signal-strength", HTTP_GET, [&bluetoothScale](AsyncWebServerRequest *request) {
    String json = "{";
    
    // WiFi signal strength
    json += "\"wifi\":" + getWiFiConnectionInfo() + ",";
    
    // Bluetooth signal strength
    json += "\"bluetooth\":" + bluetoothScale.getBluetoothConnectionInfo();
    
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/decimal-setting", HTTP_GET, [](AsyncWebServerRequest *request) {
    int decimals = getCachedDecimals();
    String json = "{\"decimals\":" + String(decimals) + "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/decimal-setting", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("decimals", true)) {
      int decimals = request->getParam("decimals", true)->value().toInt();
      if (decimals < 0) decimals = 0;
      if (decimals > 2) decimals = 2;
      setCachedDecimals(decimals);
      request->send(200, "text/plain", "Decimal setting saved.");
    } else {
      request->send(400, "text/plain", "Missing decimals parameter");
    }
  });

  server.on("/api/flowrate", HTTP_GET, [&flowRate](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(flowRate.getFlowRate(), 1));
  });

  // Bluetooth status API
  server.on("/api/bluetooth/status", HTTP_GET, [&bluetoothScale](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"connected\":" + String(bluetoothScale.isConnected() ? "true" : "false");
    json += "}";
    request->send(200, "application/json", json);
  });

  // Filter settings API endpoints
  server.on("/api/filter-settings", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"brewingThreshold\":" + String(scale.getBrewingThreshold(), 2) + ",";
    json += "\"stabilityTimeout\":" + String(scale.getStabilityTimeout()) + ",";
    json += "\"medianSamples\":" + String(scale.getMedianSamples()) + ",";
    json += "\"averageSamples\":" + String(scale.getAverageSamples());
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/filter-settings", HTTP_POST, [&scale](AsyncWebServerRequest *request) {
    String response = "{\"status\":\"success\",\"message\":\"";
    bool updated = false;
    
    if (request->hasParam("brewingThreshold", true)) {
      float threshold = request->getParam("brewingThreshold", true)->value().toFloat();
      scale.setBrewingThreshold(threshold);
      response += "Brewing threshold updated. ";
      updated = true;
    }
    if (request->hasParam("stabilityTimeout", true)) {
      unsigned long timeout = request->getParam("stabilityTimeout", true)->value().toInt();
      scale.setStabilityTimeout(timeout);
      response += "Stability timeout updated. ";
      updated = true;
    }
    if (request->hasParam("medianSamples", true)) {
      int samples = request->getParam("medianSamples", true)->value().toInt();
      scale.setMedianSamples(samples);
      response += "Median samples updated. ";
      updated = true;
    }
    if (request->hasParam("averageSamples", true)) {
      int samples = request->getParam("averageSamples", true)->value().toInt();
      scale.setAverageSamples(samples);
      response += "Average samples updated. ";
      updated = true;
    }
    
    if (updated) {
      response += "\"}";
      request->send(200, "application/json", response);
    } else {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No valid parameters provided\"}");
    }
  });

  // Filter debug endpoint - shows current filter state
  server.on("/api/filter-debug", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"filterState\":\"" + scale.getFilterState() + "\",";
    json += "\"brewingThreshold\":" + String(scale.getBrewingThreshold(), 2) + ",";
    json += "\"stabilityTimeout\":" + String(scale.getStabilityTimeout()) + ",";
    json += "\"medianSamples\":" + String(scale.getMedianSamples()) + ",";
    json += "\"averageSamples\":" + String(scale.getAverageSamples()) + ",";
    json += "\"currentWeight\":" + String(scale.getCurrentWeight(), 1);
    json += "}";
    request->send(200, "application/json", json);
  });

  // Combined settings endpoint for faster loading
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Get WiFi credentials (from cache)
    String ssid = getStoredSSID();
    String password = getStoredPassword();
    
    // Get decimal setting (from cache)
    int decimals = getCachedDecimals();
    
    // Combine into single JSON response
    String json = "{";
    json += "\"ssid\":\"" + ssid + "\",";
    json += "\"password\":\"" + password + "\",";
    json += "\"decimals\":" + String(decimals);
    json += "}";
    
    request->send(200, "application/json", json);
  });

  // Emergency NVS reset endpoint (use with caution)
  server.on("/api/reset-nvs", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("confirm", true) && request->getParam("confirm", true)->value() == "yes") {
      Serial.println("Resetting NVS storage...");
      
      // Clear all preferences
      Preferences clearPrefs;
      clearPrefs.begin("wifi", false);
      clearPrefs.clear();
      clearPrefs.end();
      
      clearPrefs.begin("display", false);
      clearPrefs.clear();
      clearPrefs.end();
      
      clearPrefs.begin("scale", false);
      clearPrefs.clear();
      clearPrefs.end();
      
      request->send(200, "text/plain", "NVS storage reset. Device will restart in 3 seconds.");
      
      // Restart the ESP32 after a short delay
      delay(3000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "Missing confirmation parameter. Use 'confirm=yes' to reset NVS.");
    }
  });

  // Serve static files for non-API paths
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // 404 Not Found handler for unmatched routes
  server.onNotFound([](AsyncWebServerRequest *request) {
    String path = request->url();
    // If the request is for an API endpoint that doesn't exist, return 404
    if (path.startsWith("/api/")) {
      request->send(404, "text/plain", "API endpoint not found");
      return;
    }
    // For all other unmatched paths, serve index.html (SPA fallback)
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Add explicit MIME type handlers for font files
  server.on("/css/all.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/css/all.min.css", "text/css");
  });
  
  server.on("/js/alpine.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/js/alpine.min.js", "application/javascript");
  });
  
  server.on("/webfonts/fa-solid-900.woff2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/webfonts/fa-solid-900.woff2", "font/woff2");
  });
  
  server.on("/webfonts/fa-regular-400.woff2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/webfonts/fa-regular-400.woff2", "font/woff2");
  });

  // Only start the web server if WiFi is enabled
  if (isWiFiEnabled()) {
    server.begin();
    Serial.println("Web server started - accessible via WiFi");
  } else {
    Serial.println("Web server NOT started - WiFi is disabled for battery saving");
  }
}

void startWebServer() {
  if (isWiFiEnabled()) {
    server.begin();
    Serial.println("Web server started");
  }
}

void stopWebServer() {
  server.end();
  Serial.println("Web server stopped");
}
