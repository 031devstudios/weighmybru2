#include "WiFiManager.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include "WebServer.h"  // For web server control
#include <FreeRTOS.h>

// ESP-IDF includes for advanced WiFi power management (SuperMini antenna fix)
#ifdef ESP_IDF_VERSION_MAJOR
    #include "esp_wifi.h"
    #include "esp_err.h"
#endif

Preferences wifiPrefs;

// Station credentials
char stored_ssid[33] = {0};
char stored_password[65] = {0};

// Cache for WiFi credentials to avoid repeated slow EEPROM reads
static String cachedSSID = "";
static String cachedPassword = "";
static bool credentialsCached = false;
static unsigned long lastCacheTime = 0;
const unsigned long CACHE_TIMEOUT = 300000; // 5 minutes cache timeout

// AP credentials
const char* ap_ssid = "WeighMyBru-AP";
const char* ap_password = "";

// WiFi Power Management State
static bool wifiEnabled = true; // WiFi enabled by default
static bool wifiEnabledCached = false;
static wifi_mode_t previousWiFiMode = WIFI_OFF; // Store previous mode when disabling WiFi

unsigned long startAttemptTime = 0;
const unsigned long timeout = 10000; // 10 seconds

void saveWiFiCredentials(const char* ssid, const char* password) {
    Serial.println("Saving WiFi credentials...");
    unsigned long startTime = millis();
    
    if (wifiPrefs.begin("wifi", false)) {
        wifiPrefs.putString("ssid", ssid);
        wifiPrefs.putString("password", password);
        wifiPrefs.end();
        
        // Update cache immediately
        cachedSSID = String(ssid);
        cachedPassword = String(password);
        credentialsCached = true;
        lastCacheTime = millis();
        
        Serial.printf("WiFi credentials saved in %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Failed to open WiFi preferences for writing");
    }
}

void clearWiFiCredentials() {
    Serial.println("Clearing WiFi credentials...");
    if (wifiPrefs.begin("wifi", false)) {
        wifiPrefs.clear();
        wifiPrefs.end();
        
        // Clear cache
        cachedSSID = "";
        cachedPassword = "";
        credentialsCached = true;
        lastCacheTime = millis();
        
        Serial.println("WiFi credentials cleared");
    } else {
        Serial.println("ERROR: Failed to open WiFi preferences for clearing");
    }
}

bool loadWiFiCredentialsFromEEPROM() {
    // Check if cache is still valid first (no serial output for speed)
    if (credentialsCached && (millis() - lastCacheTime < CACHE_TIMEOUT)) {
        return true;
    }
    
    unsigned long startTime = millis();
    
    // Add timeout protection
    const unsigned long EEPROM_TIMEOUT = 5000; // 5 second timeout
    bool success = false;
    
    unsigned long attemptStart = millis();
    if (wifiPrefs.begin("wifi", true)) {
        // Check if operation is taking too long
        if (millis() - attemptStart > EEPROM_TIMEOUT) {
            wifiPrefs.end();
            cachedSSID = "";
            cachedPassword = "";
        } else {
            cachedSSID = wifiPrefs.getString("ssid", "");
            cachedPassword = wifiPrefs.getString("password", "");
            success = true;
        }
        wifiPrefs.end();
        
        credentialsCached = true;
        lastCacheTime = millis();
        
        // Minimal serial output to reduce blocking
        Serial.printf("WiFi: %s in %lums\n", success ? "OK" : "TIMEOUT", millis() - startTime);
        return success;
    } else {
        // Use empty defaults if EEPROM fails
        cachedSSID = "";
        cachedPassword = "";
        credentialsCached = true;
        lastCacheTime = millis();
        Serial.println("WiFi: EEPROM FAIL");
        return false;
    }
}

void loadWiFiCredentials(char* ssid, char* password, size_t maxLen) {
    loadWiFiCredentialsFromEEPROM();
    strncpy(ssid, cachedSSID.c_str(), maxLen - 1);
    strncpy(password, cachedPassword.c_str(), maxLen - 1);
    ssid[maxLen - 1] = '\0';
    password[maxLen - 1] = '\0';
}

String getStoredSSID() {
    // Fast path - return immediately if already cached and recent
    if (credentialsCached && (millis() - lastCacheTime < CACHE_TIMEOUT)) {
        return cachedSSID;
    }
    
    // Only do EEPROM read if cache is invalid
    loadWiFiCredentialsFromEEPROM();
    return cachedSSID;
}

String getStoredPassword() {
    // Fast path - return immediately if already cached and recent
    if (credentialsCached && (millis() - lastCacheTime < CACHE_TIMEOUT)) {
        return cachedPassword;
    }
    
    // Only do EEPROM read if cache is invalid
    loadWiFiCredentialsFromEEPROM();
    return cachedPassword;
}

void setupWiFi() {
    // Check if WiFi should be enabled
    if (!loadWiFiEnabledState()) {
        Serial.println("WiFi is disabled - skipping WiFi setup for battery saving");
        WiFi.mode(WIFI_OFF);
        return;
    }
    
    char ssid[33] = {0};
    char password[65] = {0};
    loadWiFiCredentials(ssid, password, sizeof(ssid));
    
    // Ensure WiFi is completely reset first
    Serial.println("=== WIFI ANTENNA OPTIMIZATION ===");
    Serial.println("Resetting WiFi subsystem...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500); // Longer delay for complete reset
    
    // Apply SuperMini antenna fix for boards with poor antenna design
    applySuperMiniAntennaFix();
    
    // Check if we have stored credentials - prioritize STA connection
    if (strlen(ssid) > 0) {
        Serial.println("=== ATTEMPTING STA CONNECTION ===");
        Serial.println("Found stored credentials for: " + String(ssid));
        Serial.println("Trying STA mode first (power optimized)...");
        
        // Try STA mode first for lower power consumption
        WiFi.mode(WIFI_STA);
        delay(1000); // Ensure mode switch is stable
        
        // ANTENNA FIX: Reapply power settings after mode switch for SuperMini boards
        // Mode switch can reset power levels, so reapply the fix
        if (ENABLE_SUPERMINI_ANTENNA_FIX) {
            applySuperMiniAntennaFix();
        }
        
        startAttemptTime = millis();
        WiFi.begin(ssid, password);
        
        // Wait for connection with reasonable timeout
        int connectionAttempts = 0;
        const int maxAttempts = 24; // 12 seconds total - more time for reliable connection
        
        Serial.print("Connecting");
        while (WiFi.status() != WL_CONNECTED && connectionAttempts < maxAttempts) {
            delay(500);
            Serial.print(".");
            connectionAttempts++;
            
            // Check for immediate connection failures
            if (WiFi.status() == WL_NO_SSID_AVAIL) {
                Serial.println("\nNetwork '" + String(ssid) + "' not found");
                break;
            }
            if (WiFi.status() == WL_CONNECT_FAILED) {
                Serial.println("\nConnection failed - likely incorrect password");
                break;
            }
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nSTA CONNECTION SUCCESSFUL!");
            Serial.println("===========================");
            Serial.println("Connected to: " + String(ssid));
            Serial.println("IP Address: " + WiFi.localIP().toString());
            Serial.println("Gateway: " + WiFi.gatewayIP().toString());
            Serial.println("DNS: " + WiFi.dnsIP().toString());
            Serial.println("Signal: " + String(WiFi.RSSI()) + " dBm");
            Serial.println("AP mode disabled - optimized for low power");
            Serial.println("Will auto-fallback to AP if connection lost");
            Serial.println("===========================");
            
            // Setup mDNS for STA mode
            setupmDNS();
            
            return; // Exit early - we're connected via STA, no need for AP
        } else {
            Serial.println("\nSTA CONNECTION FAILED");
            Serial.println("Status code: " + String(WiFi.status()));
            Serial.println("Falling back to AP mode for configuration...");
        }
    } else {
        Serial.println("=== NO STORED CREDENTIALS ===");
        Serial.println("No WiFi credentials found - starting AP mode for initial setup");
    }
    
    // Fallback to AP mode if STA failed or no credentials exist
    Serial.println("Starting AP mode...");
    WiFi.mode(WIFI_AP);
    delay(1000); // Ensure mode switch is stable
    
    // Configure AP with optimized settings for maximum visibility
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    
    // Start AP with maximum power and visibility settings
    bool apStarted = false;
    Serial.println("Starting AP for credential configuration...");
    
    // Try channel 6 first (most common and widely supported)
    apStarted = WiFi.softAP(ap_ssid, ap_password, 6, false, 4); // Channel 6, broadcast SSID, max 4 clients
    
    if (apStarted) {
        Serial.println("AP started successfully on channel 6");
    } else {
        Serial.println("Channel 6 failed, trying channel 1...");
        apStarted = WiFi.softAP(ap_ssid, ap_password, 1, false, 4); // Channel 1, broadcast SSID
        
        if (apStarted) {
            Serial.println("AP started successfully on channel 1");
        } else {
            Serial.println("Channel 1 failed, trying default settings...");
            apStarted = WiFi.softAP(ap_ssid); // Simplest possible configuration
            if (apStarted) {
                Serial.println("AP started with default settings");
            }
        }
    }
    
    if (apStarted) {
        Serial.println("=== AP MODE ACTIVE ===");
        Serial.println("AP SSID: " + String(ap_ssid));
        Serial.println("AP IP: " + WiFi.softAPIP().toString());
        Serial.println("AP MAC: " + WiFi.softAPmacAddress());
        Serial.printf("AP Channel: %d\n", WiFi.channel());
        Serial.printf("WiFi TX Power: %d dBm\n", WiFi.getTxPower());
        Serial.println("Connect to 'WeighMyBru-AP' to configure WiFi");
        Serial.println("Access: http://192.168.4.1 or http://weighmybru.local");
        Serial.println("=====================");
        
        // Setup mDNS for AP mode
        setupmDNS();
    } else {
        Serial.println("ERROR: AP failed to start - hardware or RF issue suspected");
    }
}

void setupmDNS() {
    // Start mDNS service with hostname "weighmybru"
    if (MDNS.begin("weighmybru")) {
        Serial.println("mDNS responder started/updated");
        Serial.println("Access the scale at: http://weighmybru.local");
        
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("websocket", "tcp", 81);
        
        // Add some useful service properties
        MDNS.addServiceTxt("http", "tcp", "device", "WeighMyBru Coffee Scale");
        MDNS.addServiceTxt("http", "tcp", "version", "2.0");
        
    } else {
        Serial.println("Error starting mDNS responder");
    }
}

void printWiFiStatus(void * parameter) {
    TickType_t lastStatusUpdate = xTaskGetTickCount();
    for(;;){
    Serial.println("=== WiFi Status ===");
    Serial.println("WiFi Mode: " + String(WiFi.getMode()));
    Serial.println("AP Status: " + String(WiFi.softAPgetStationNum()) + " clients connected");
    Serial.println("AP IP: " + WiFi.softAPIP().toString());
    Serial.println("AP SSID: " + String(ap_ssid));
    Serial.println("STA Status: " + String(WiFi.status()));
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("STA IP: " + WiFi.localIP().toString());
        Serial.println("STA RSSI: " + String(WiFi.RSSI()) + " dBm");
    }
    Serial.println("WiFi Sleep: " + String(WiFi.getSleep() ? "ON" : "OFF"));
    Serial.println("==================");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
        // xTaskDelayUntil(&lastStatusUpdate, 30000 / portTICK_PERIOD_MS);
    }
}

void maintainWiFi(void * parameter) {
    // Skip maintenance if WiFi is disabled
    TickType_t lastMaintenance = xTaskGetTickCount();
    // const TickType_t maintenanceInterval = ; // Every 15 seconds for more responsive switching
    for(;;){
        if (isWiFiEnabled()) {     
        // if (millis() - lastMaintenance >= maintenanceInterval) {
            // lastMaintenance = millis();
        
        // Check current WiFi mode and connection health
        wifi_mode_t currentMode = WiFi.getMode();
        
        if (currentMode == WIFI_STA) {
            // We're in STA mode - check if connection is still healthy
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WARNING: STA connection lost! Attempting immediate reconnection...");
                
                // Try to reconnect to saved credentials
                char ssid[33] = {0};
                char password[65] = {0};
                loadWiFiCredentials(ssid, password, sizeof(ssid));
                
                if (strlen(ssid) > 0) {
                    Serial.println("Attempting to reconnect to: " + String(ssid));
                    WiFi.begin(ssid, password);
                    
                    // Wait briefly for reconnection - reduced timeout for faster fallback
                    int attempts = 0;
                    while (WiFi.status() != WL_CONNECTED && attempts < 6) { // 3 second timeout
                        delay(500);
                        Serial.print(".");
                        attempts++;
                    }
                    
                    if (WiFi.status() == WL_CONNECTED) {
                        Serial.println("\nSTA reconnection successful");
                        Serial.println("IP: " + WiFi.localIP().toString());
                    } else {
                        Serial.println("\nSTA reconnection failed - switching to AP mode immediately");
                        switchToAPMode();
                    }
                } else {
                    Serial.println("No stored credentials - switching to AP mode");
                    switchToAPMode();
                }
            } else {
                Serial.println("STA mode healthy - connection maintained");
                Serial.println("Connected to: " + WiFi.SSID() + " | IP: " + WiFi.localIP().toString() + " | RSSI: " + String(WiFi.RSSI()) + "dBm");
            }
        } else if (currentMode == WIFI_AP) {
            // We're in AP mode - just ensure it's still running properly
            if (WiFi.softAPgetStationNum() == 0) {
                Serial.println("AP mode active - 'WeighMyBru-AP' ready for configuration");
            } else {
                Serial.println("AP mode active - " + String(WiFi.softAPgetStationNum()) + " clients connected");
            }
        } else if (currentMode == WIFI_OFF) {
            Serial.println("CRITICAL: WiFi is OFF! This should not happen - restarting AP mode");
            switchToAPMode();
        }
        
        // Ensure WiFi sleep stays enabled for BLE coexistence
        if (!WiFi.getSleep()) {
            Serial.println("WARNING: WiFi sleep was disabled! Re-enabling for BLE coexistence...");
            WiFi.setSleep(true);
        }
        
        // Print status for debugging
        Serial.println("WiFi maintenance check completed");
    }
        xTaskDelayUntil(&lastMaintenance, 15000/portTICK_PERIOD_MS);
    }    
}

// Function to attempt STA connection with new credentials and switch from AP mode
bool attemptSTAConnection(const char* ssid, const char* password) {
    Serial.println("=== ATTEMPTING STA CONNECTION ===");
    Serial.println("SSID: " + String(ssid));
    Serial.println("Switching from AP mode to STA mode...");
    
    // Disconnect from AP mode but keep WiFi on
    WiFi.mode(WIFI_STA);
    delay(1000); // Allow mode switch to stabilize
    
    // ANTENNA FIX: Reapply power settings after mode switch for SuperMini boards
    if (ENABLE_SUPERMINI_ANTENNA_FIX) {
        Serial.println("Reapplying SuperMini antenna fix after mode switch...");
        applySuperMiniAntennaFix();
    }
    
    // Attempt connection with new credentials
    startAttemptTime = millis();
    WiFi.begin(ssid, password);
    
    // Wait for connection with reasonable timeout
    int connectionAttempts = 0;
    const int maxAttempts = 30; // 15 seconds total - more generous for initial connection
    
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED && connectionAttempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        connectionAttempts++;
        
        // Check for immediate connection failures
        if (WiFi.status() == WL_NO_SSID_AVAIL) {
            Serial.println("\nSSID not found");
            return false;
        }
        if (WiFi.status() == WL_CONNECT_FAILED) {
            Serial.println("\nConnection failed - likely wrong password");
            return false;
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nSTA CONNECTION SUCCESSFUL!");
        Serial.println("Connected to: " + String(ssid));
        Serial.println("IP Address: " + WiFi.localIP().toString());
        Serial.println("Gateway: " + WiFi.gatewayIP().toString());
        Serial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
        Serial.println("AP mode disabled - power consumption optimized");
        
        // Setup mDNS for the new STA connection
        setupmDNS();
        
        return true;
    } else {
        Serial.println("\nSTA connection failed or timed out");
        Serial.println("Status code: " + String(WiFi.status()));
        return false;
    }
}

// Function to switch back to AP mode if STA connection fails
void switchToAPMode() {
    Serial.println("=== SWITCHING TO AP MODE ===");
    Serial.println("Disconnecting from STA mode...");
    WiFi.disconnect(true);
    delay(500);
    
    Serial.println("Setting AP mode...");
    WiFi.mode(WIFI_AP);
    delay(1000); // Allow mode switch to stabilize
    
    // Restart AP with same settings as setupWiFi()
    Serial.println("Starting AP broadcast...");
    bool apStarted = WiFi.softAP(ap_ssid, ap_password, 6, false, 4);
    
    if (apStarted) {
        Serial.println("AP MODE RESTORED");
        Serial.println("==================");
        Serial.println("SSID: " + String(ap_ssid));
        Serial.println("IP: " + WiFi.softAPIP().toString());
        Serial.println("Config URL: http://192.168.4.1");
        Serial.println("mDNS: http://weighmybru.local");
        Serial.println("==================");
        
        // Setup mDNS for AP mode
        setupmDNS();
    } else {
        Serial.println("CRITICAL: Failed to restart AP mode!");
        Serial.println("Retrying with minimal settings...");
        // Try with minimal settings as fallback
        if (WiFi.softAP(ap_ssid)) {
            Serial.println("AP started with minimal settings");
            setupmDNS();
        } else {
            Serial.println("FATAL: Cannot start AP mode - WiFi hardware issue?");
        }
    }
}

// Apply SuperMini antenna fix for boards with poor antenna design
void applySuperMiniAntennaFix() {
    if (!ENABLE_SUPERMINI_ANTENNA_FIX) {
        Serial.println("SuperMini antenna fix disabled in configuration");
        return;
    }
    
    Serial.println("Applying SuperMini antenna fix...");
    
    // Arduino framework maximum power
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    Serial.println("Arduino framework power: 19.5dBm");
    
    // ESP-IDF level power boost (the key fix from forums)
    #ifdef ESP_IDF_VERSION_MAJOR
        esp_err_t result = esp_wifi_set_max_tx_power(40); // 10dBm (40 = 4 * 10dBm)
        if (result == ESP_OK) {
            Serial.println("ESP-IDF max TX power: 10dBm (touch-antenna fix applied)");
        } else {
            Serial.printf("ESP-IDF power setting failed: %s\n", esp_err_to_name(result));
        }
    #else
        Serial.println("ESP-IDF functions not available - using Arduino framework only");
    #endif
    
    Serial.println("SuperMini antenna optimization complete");
    Serial.println("   This fixes the common 'touch antenna to work' issue");
}

// Get current WiFi signal strength in dBm
int getWiFiSignalStrength() {
    if (WiFi.status() != WL_CONNECTED) {
        return -100; // Return very poor signal if not connected
    }
    return WiFi.RSSI();
}

// Get WiFi signal quality description
String getWiFiSignalQuality() {
    if (WiFi.status() != WL_CONNECTED) {
        return "Disconnected";
    }
    
    int rssi = WiFi.RSSI();
    
    if (rssi >= -30) {
        return "Excellent";
    } else if (rssi >= -50) {
        return "Very Good";
    } else if (rssi >= -60) {
        return "Good";
    } else if (rssi >= -70) {
        return "Fair";
    } else if (rssi >= -80) {
        return "Weak";
    } else {
        return "Very Weak";
    }
}

// Get detailed WiFi connection information
String getWiFiConnectionInfo() {
    String info = "{";
    
    if (WiFi.status() == WL_CONNECTED) {
        info += "\"connected\":true,";
        info += "\"mode\":\"STA\",";
        info += "\"ssid\":\"" + WiFi.SSID() + "\",";
        info += "\"signal_strength\":" + String(WiFi.RSSI()) + ",";
        info += "\"signal_quality\":\"" + getWiFiSignalQuality() + "\",";
        info += "\"channel\":" + String(WiFi.channel()) + ",";
        info += "\"tx_power\":" + String(WiFi.getTxPower()) + ",";
        info += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        info += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
        info += "\"dns\":\"" + WiFi.dnsIP().toString() + "\",";
        info += "\"mac\":\"" + WiFi.macAddress() + "\"";
    } else {
        info += "\"connected\":false,";
        info += "\"mode\":\"AP\",";
        info += "\"ssid\":\"" + String(ap_ssid) + "\",";
        info += "\"signal_strength\":null,";
        info += "\"signal_quality\":\"N/A - AP Mode\",";
        info += "\"channel\":" + String(WiFi.channel()) + ",";
        info += "\"tx_power\":" + String(WiFi.getTxPower()) + ",";
        info += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
        info += "\"gateway\":\"N/A\",";
        info += "\"dns\":\"N/A\",";
        info += "\"mac\":\"" + WiFi.macAddress() + "\",";
        info += "\"connected_clients\":" + String(WiFi.softAPgetStationNum());
    }
    
    info += "}";
    return info;
}

// WiFi Power Management Functions

bool loadWiFiEnabledState() {
    if (wifiEnabledCached) {
        return wifiEnabled;
    }
    
    if (wifiPrefs.begin("wifi", true)) {
        wifiEnabled = wifiPrefs.getBool("enabled", true); // Default to enabled
        wifiPrefs.end();
        wifiEnabledCached = true;
        Serial.printf("WiFi enabled state loaded: %s\n", wifiEnabled ? "ON" : "OFF");
    } else {
        Serial.println("ERROR: Failed to load WiFi enabled state");
        wifiEnabled = true; // Default to enabled on error
    }
    
    return wifiEnabled;
}

void saveWiFiEnabledState(bool enabled) {
    if (wifiPrefs.begin("wifi", false)) {
        wifiPrefs.putBool("enabled", enabled);
        wifiPrefs.end();
        wifiEnabled = enabled;
        wifiEnabledCached = true;
        Serial.printf("WiFi enabled state saved: %s\n", enabled ? "ON" : "OFF");
    } else {
        Serial.println("ERROR: Failed to save WiFi enabled state");
    }
}

bool isWiFiEnabled() {
    loadWiFiEnabledState();
    return wifiEnabled;
}

void enableWiFi() {
    Serial.println("Enabling WiFi...");
    
    // Save the enabled state
    saveWiFiEnabledState(true);
    
    // If WiFi was previously off, restore it
    if (WiFi.getMode() == WIFI_OFF) {
        // Try to restore to STA mode first if we have credentials
        if (loadWiFiCredentialsFromEEPROM() && !cachedSSID.isEmpty()) {
            Serial.println("Attempting to reconnect to saved network...");
            if (attemptSTAConnection(cachedSSID.c_str(), cachedPassword.c_str())) {
                Serial.println("WiFi reconnected to STA mode");
                startWebServer(); // Start web server when WiFi is enabled
                return;
            }
        }
        
        // Fall back to AP mode if STA connection fails
        Serial.println("Starting WiFi in AP mode...");
        switchToAPMode();
        startWebServer(); // Start web server when WiFi is enabled
    }
    
    Serial.println("WiFi enabled");
}

void disableWiFi() {
    Serial.println("Disabling WiFi to save battery...");
    
    // Stop web server first to prevent TCP/IP stack issues
    stopWebServer();
    
    // Save current mode before disabling
    previousWiFiMode = WiFi.getMode();
    
    // Save the disabled state
    saveWiFiEnabledState(false);
    
    // Gracefully close active connections before disabling WiFi
    Serial.println("Closing active connections...");
    
    // Give time for current HTTP responses to complete
    delay(100);
    
    // Properly disconnect based on current mode
    if (previousWiFiMode == WIFI_STA || previousWiFiMode == WIFI_AP_STA) {
        Serial.println("Disconnecting from STA...");
        WiFi.disconnect(true);
    }
    
    if (previousWiFiMode == WIFI_AP || previousWiFiMode == WIFI_AP_STA) {
        Serial.println("Stopping AP mode...");
        WiFi.softAPdisconnect(true);
    }
    
    // Additional delay to ensure cleanup
    delay(200);
    
    // Now safely turn off WiFi
    WiFi.mode(WIFI_OFF);
    
    Serial.println("WiFi disabled - battery saving mode active");
}

void toggleWiFi() {
    if (isWiFiEnabled() && WiFi.getMode() != WIFI_OFF) {
        disableWiFi();
    } else {
        enableWiFi();
    }
}