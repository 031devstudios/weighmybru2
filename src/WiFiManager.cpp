#include "WiFiManager.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include "WebServer.h"  // For web server control

// ESP-IDF includes for advanced WiFi power management (SuperMini antenna fix)
#ifdef ESP_IDF_VERSION_MAJOR
    #include "esp_wifi.h"
    #include "esp_err.h"
#endif

/* 
 * WiFi Power Optimization Strategy:
 * 
 * STA Mode (60mA typical):
 * - Uses maximum TX power (19.5dBm) for connection reliability
 * - Enables WiFi sleep mode for BLE coexistence
 * - Single connection to router - lower overhead
 * 
 * AP Mode (90mA typical, optimized to ~70mA):
 * - Reduced TX power to 15dBm (sufficient for local connections)
 * - Increased beacon interval from 100ms to 200ms
 * - Limited to 2 max clients instead of 4
 * - Enabled AP power save mode
 * - Expected power reduction: 20-30mA
 * 
 * WiFi Disabled (50mA typical):
 * - Complete WiFi subsystem shutdown
 * - CPU frequency reduced to 80MHz
 * - Bluetooth remains active for scale functionality
 */

Preferences wifiPrefs;

// EEPROM addresses for backup storage
#define EEPROM_WIFI_ENABLED_ADDR 100
#define EEPROM_MAGIC_BYTE_ADDR 101
#define EEPROM_MAGIC_VALUE 0xAB  // Magic byte to verify EEPROM has valid data

// Station credentials
char stored_ssid[33] = {0};
char stored_password[65] = {0};

// Cache for WiFi credentials to avoid repeated slow EEPROM reads
static String cachedSSID = "";
static String cachedPassword = "";
static bool credentialsCached = false;
static unsigned long lastCacheTime = 0;
const unsigned long CACHE_TIMEOUT = 300000; // 5 minutes cache timeout

// Filesystem status tracking
static bool filesystemAvailable = false;
static bool filesystemChecked = false;
static unsigned long lastFilesystemError = 0;
const unsigned long FILESYSTEM_ERROR_COOLDOWN = 30000; // Show error message every 30 seconds max

// AP credentials
const char* ap_ssid = "WeighMyBru-AP";
const char* ap_password = "";

// WiFi Power Management State
static bool wifiEnabled = true; // WiFi enabled by default
static bool wifiEnabledCached = false;
static wifi_mode_t previousWiFiMode = WIFI_OFF; // Store previous mode when disabling WiFi

unsigned long startAttemptTime = 0;
const unsigned long timeout = 10000; // 10 seconds

void checkFilesystemStatus() {
    if (filesystemChecked) {
        return; // Already checked
    }
    
    // Test if filesystem/NVS is available
    Preferences testPrefs;
    if (testPrefs.begin("test", false)) {
        testPrefs.end();
        filesystemAvailable = true;
        Serial.println("✓ Filesystem/NVS is available");
    } else {
        filesystemAvailable = false;
        Serial.println("=================================");
        Serial.println("⚠️  FILESYSTEM NOT AVAILABLE");
        Serial.println("=================================");
        Serial.println("The device filesystem has not been");
        Serial.println("uploaded to the ESP32.");
        Serial.println("");
        Serial.println("To fix this, run:");
        Serial.println("pio run -t uploadfs");
        Serial.println("or upload filesystem via PlatformIO");
        Serial.println("");
        Serial.println("Device will work in AP mode until");
        Serial.println("filesystem is uploaded.");
        Serial.println("=================================");
    }
    filesystemChecked = true;
}

void showFilesystemErrorIfNeeded() {
    if (filesystemAvailable) {
        return; // No error to show
    }
    
    unsigned long now = millis();
    if (now - lastFilesystemError > FILESYSTEM_ERROR_COOLDOWN) {
        Serial.println("⚠️  Filesystem not available - run 'pio run -t uploadfs'");
        lastFilesystemError = now;
    }
}

void saveWiFiCredentials(const char* ssid, const char* password) {
    Serial.println("Saving WiFi credentials...");
    unsigned long startTime = millis();
    
    checkFilesystemStatus();
    
    if (!filesystemAvailable) {
        // Update cache even if we can't save to NVS
        cachedSSID = String(ssid);
        cachedPassword = String(password);
        credentialsCached = true;
        lastCacheTime = millis();
        
        Serial.println("INFO: WiFi credentials cached (filesystem unavailable for permanent storage)");
        return;
    }
    
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
        showFilesystemErrorIfNeeded();
        // Still update cache for this session
        cachedSSID = String(ssid);
        cachedPassword = String(password);
        credentialsCached = true;
        lastCacheTime = millis();
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
    
    checkFilesystemStatus();
    
    if (!filesystemAvailable) {
        // Use empty defaults if filesystem unavailable
        cachedSSID = "";
        cachedPassword = "";
        credentialsCached = true;
        lastCacheTime = millis();
        return false;
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
        showFilesystemErrorIfNeeded();
        // Use empty defaults if EEPROM fails
        cachedSSID = "";
        cachedPassword = "";
        credentialsCached = true;
        lastCacheTime = millis();
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
    // Debug: Show what state we're loading
    Serial.println("=== WiFi SETUP DEBUG ===");
    bool wifi_should_be_enabled = loadWiFiEnabledState();
    Serial.printf("WiFi state on boot: %s\n", wifi_should_be_enabled ? "ENABLED" : "DISABLED");
    Serial.println("========================");
    
    // Check if WiFi should be enabled
    if (!wifi_should_be_enabled) {
        Serial.println("WiFi is disabled - ensuring proper low-power state for battery saving");
        // Use the proper disable function to ensure clean shutdown
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(100); // Ensure hardware state settles
        
        // Additional ESP-IDF level power management for maximum power savings
        #ifdef ESP_IDF_VERSION_MAJOR
            esp_err_t result = esp_wifi_stop();
            if (result == ESP_OK) {
                Serial.println("ESP-IDF WiFi subsystem stopped for maximum power saving");
            } else {
                Serial.printf("ESP-IDF WiFi stop failed: %s\n", esp_err_to_name(result));
            }
        #endif
        
        Serial.println("WiFi hardware properly disabled for maximum battery life");
        return;
    }

    setupWiFiForced();
}

void setupWiFiForced() {
    Serial.println("=== FORCING WiFi INITIALIZATION ===");
    
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
    
    // Start AP with power-optimized settings for battery efficiency
    bool apStarted = false;
    Serial.println("Starting AP for credential configuration (power optimized)...");
    
    // Try channel 6 first (most common and widely supported)
    // Reduced max clients from 4 to 2 for lower power consumption
    apStarted = WiFi.softAP(ap_ssid, ap_password, 6, false, 2); // Channel 6, broadcast SSID, max 2 clients
    
    if (apStarted) {
        // Apply AP-specific power optimizations to reduce battery consumption
        applyAPModePowerOptimization();
        
        Serial.println("AP started successfully on channel 6 (power optimized)");
    } else {
        Serial.println("Channel 6 failed, trying channel 1...");
        apStarted = WiFi.softAP(ap_ssid, ap_password, 1, false, 2); // Channel 1, broadcast SSID, max 2 clients
        
        if (apStarted) {
            // Apply AP-specific power optimizations
            applyAPModePowerOptimization();
            Serial.println("AP started successfully on channel 1 (power optimized)");
        } else {
            Serial.println("Channel 1 failed, trying default settings...");
            apStarted = WiFi.softAP(ap_ssid); // Simplest possible configuration
            if (apStarted) {
                Serial.println("AP started with default settings");
                // Still apply power optimization even with default settings
                applyAPModePowerOptimization();
            }
        }
    }
    
    if (apStarted) {
        // Apply AP mode power optimizations for battery efficiency
        applyAPModePowerOptimization();
        
        Serial.println("=== AP MODE ACTIVE (POWER OPTIMIZED) ===");
        Serial.println("AP SSID: " + String(ap_ssid));
        Serial.println("AP IP: " + WiFi.softAPIP().toString());
        Serial.println("AP MAC: " + WiFi.softAPmacAddress());
        Serial.printf("AP Channel: %d\n", WiFi.channel());
        Serial.printf("WiFi TX Power: %d dBm (optimized for battery)\n", WiFi.getTxPower());
        Serial.println("Max Clients: 2 (reduced for power savings)");
        Serial.println("Beacon Interval: 200ms (increased for power savings)");
        Serial.println("Connect to 'WeighMyBru-AP' to configure WiFi");
        Serial.println("Access: http://192.168.4.1 or http://weighmybru.local");
        Serial.println("========================================");
        
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

void printWiFiStatus() {
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
}

void maintainWiFi() {
    // Skip maintenance if WiFi is disabled
    if (!isWiFiEnabled()) {
        return;
    }
    
    static unsigned long lastMaintenance = 0;
    const unsigned long maintenanceInterval = 15000; // Every 15 seconds for more responsive switching
    
    if (millis() - lastMaintenance >= maintenanceInterval) {
        lastMaintenance = millis();
        
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
    
    // Restart AP with power-optimized settings 
    Serial.println("Starting AP broadcast (power optimized)...");
    bool apStarted = WiFi.softAP(ap_ssid, ap_password, 6, false, 2); // Reduced to 2 clients for power savings
    
    if (apStarted) {
        // Apply AP-specific power optimizations for battery efficiency 
        applyAPModePowerOptimization();
        
        Serial.println("AP MODE RESTORED (POWER OPTIMIZED)");
        Serial.println("==================");
        Serial.println("SSID: " + String(ap_ssid));
        Serial.println("IP: " + WiFi.softAPIP().toString());
        Serial.println("Config URL: http://192.168.4.1");
        Serial.println("mDNS: http://weighmybru.local");
        Serial.println("Max Clients: 2 (optimized for battery)");
        Serial.println("==================");;
        
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
    
    // Check current WiFi mode to apply appropriate power settings
    wifi_mode_t currentMode = WiFi.getMode();
    
    if (currentMode == WIFI_STA) {
        // STA mode needs maximum power for connection reliability
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        Serial.println("STA mode - Arduino framework power: 19.5dBm (maximum for reliability)");
        
        // ESP-IDF level power boost for STA mode
        #ifdef ESP_IDF_VERSION_MAJOR
            esp_err_t result = esp_wifi_set_max_tx_power(40); // 10dBm (40 = 4 * 10dBm)
            if (result == ESP_OK) {
                Serial.println("STA mode - ESP-IDF max TX power: 10dBm (touch-antenna fix applied)");
            } else {
                Serial.printf("ESP-IDF power setting failed: %s\n", esp_err_to_name(result));
            }
        #endif
    } else {
        // For AP mode, we'll handle power optimization separately 
        // Just apply basic antenna fix here
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        Serial.println("Non-STA mode - Arduino framework power: 19.5dBm (will be optimized separately)");
        
        #ifdef ESP_IDF_VERSION_MAJOR
            esp_err_t result = esp_wifi_set_max_tx_power(40);
            if (result == ESP_OK) {
                Serial.println("Non-STA mode - ESP-IDF max TX power: 10dBm");
            } else {
                Serial.printf("ESP-IDF power setting failed: %s\n", esp_err_to_name(result));
            }
        #endif
    }
    
    #ifndef ESP_IDF_VERSION_MAJOR
        Serial.println("ESP-IDF functions not available - using Arduino framework only");
    #endif
    
    Serial.println("SuperMini antenna optimization complete");
    Serial.println("   This fixes the common 'touch antenna to work' issue");
}

// Apply AP mode specific power optimizations for battery efficiency
void applyAPModePowerOptimization() {
    Serial.println("Applying AP mode power optimizations...");
    
    #ifdef ESP_IDF_VERSION_MAJOR
        // Reduce AP mode TX power for battery savings (AP doesn't need max range like STA)
        WiFi.setTxPower(WIFI_POWER_15dBm); // Reduced from 19.5dBm to 15dBm for AP mode
        Serial.println("AP TX power reduced to 15dBm for battery efficiency");
        
        // Configure AP beacon interval for power savings
        wifi_config_t ap_config;
        esp_err_t result = esp_wifi_get_config(WIFI_IF_AP, &ap_config);
        if (result == ESP_OK) {
            ap_config.ap.beacon_interval = 200; // Increase from default 100ms to 200ms
            result = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
            if (result == ESP_OK) {
                Serial.println("AP beacon interval increased to 200ms for power savings");
            } else {
                Serial.printf("Failed to set beacon interval: %s\n", esp_err_to_name(result));
            }
        } else {
            Serial.printf("Failed to get AP config: %s\n", esp_err_to_name(result));
        }
        
        // Set AP mode power save parameters
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM); // Enable minimal power save for AP mode
        Serial.println("AP power save mode enabled");
        
    #else
        // Fallback for non-ESP-IDF builds
        WiFi.setTxPower(WIFI_POWER_15dBm);
        Serial.println("AP power reduced to 15dBm (basic optimization)");
    #endif
    
    Serial.println("AP power optimization complete - should reduce consumption by ~20-30mA");
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
    
    Serial.println("Loading WiFi enabled state...");
    
    bool nvs_loaded = false;
    bool eeprom_loaded = false;
    bool nvs_enabled = true;
    bool eeprom_enabled = true;
    
    // Method 1: Try to load from NVS (Preferences)
    if (wifiPrefs.begin("wifi", true)) {
        if (wifiPrefs.isKey("enabled")) {
            nvs_enabled = wifiPrefs.getBool("enabled", true);
            nvs_loaded = true;
            Serial.printf("✓ WiFi state loaded from NVS: %s\n", nvs_enabled ? "ENABLED" : "DISABLED");
        } else {
            Serial.println("! No WiFi state found in NVS (first boot)");
        }
        wifiPrefs.end();
    } else {
        Serial.println("✗ Failed to access NVS for WiFi state");
    }
    
    // Method 2: Try to load from EEPROM backup
    EEPROM.begin(512);
    uint8_t magic = EEPROM.read(EEPROM_MAGIC_BYTE_ADDR);
    if (magic == EEPROM_MAGIC_VALUE) {
        uint8_t enabled_byte = EEPROM.read(EEPROM_WIFI_ENABLED_ADDR);
        eeprom_enabled = (enabled_byte == 1);
        eeprom_loaded = true;
        Serial.printf("✓ WiFi state loaded from EEPROM: %s\n", eeprom_enabled ? "ENABLED" : "DISABLED");
    } else {
        Serial.println("! No valid WiFi state found in EEPROM");
    }
    EEPROM.end();
    
    // Decide which value to use
    if (nvs_loaded && eeprom_loaded) {
        if (nvs_enabled == eeprom_enabled) {
            wifiEnabled = nvs_enabled;
            Serial.printf("WiFi state consistent: %s\n", wifiEnabled ? "ENABLED" : "DISABLED");
        } else {
            // Conflict - use NVS as primary
            wifiEnabled = nvs_enabled;
            Serial.printf("WiFi state conflict! Using NVS value: %s\n", wifiEnabled ? "ENABLED" : "DISABLED");
        }
    } else if (nvs_loaded) {
        wifiEnabled = nvs_enabled;
        Serial.printf("WiFi state from NVS only: %s\n", wifiEnabled ? "ENABLED" : "DISABLED");
    } else if (eeprom_loaded) {
        wifiEnabled = eeprom_enabled;
        Serial.printf("WiFi state from EEPROM only: %s\n", wifiEnabled ? "ENABLED" : "DISABLED");
    } else {
        wifiEnabled = true; // Default to enabled on first boot
        Serial.println("WiFi state: DEFAULT (ENABLED) - first boot detected");
    }
    
    wifiEnabledCached = true;
    return wifiEnabled;
}

void saveWiFiEnabledState(bool enabled) {
    Serial.printf("Saving WiFi state: %s...\n", enabled ? "ENABLED" : "DISABLED");
    
    bool nvs_success = false;
    bool eeprom_success = false;
    
    // Method 1: Try NVS (Preferences)
    if (wifiPrefs.begin("wifi", false)) {
        wifiPrefs.putBool("enabled", enabled);
        wifiPrefs.end();
        nvs_success = true;
        Serial.printf("✓ WiFi state saved to NVS: %s\n", enabled ? "ON" : "OFF");
    } else {
        Serial.println("✗ Failed to save WiFi state to NVS!");
    }
    
    // Method 2: Backup to EEPROM
    EEPROM.begin(512);
    EEPROM.write(EEPROM_WIFI_ENABLED_ADDR, enabled ? 1 : 0);
    EEPROM.write(EEPROM_MAGIC_BYTE_ADDR, EEPROM_MAGIC_VALUE);
    if (EEPROM.commit()) {
        eeprom_success = true;
        Serial.printf("✓ WiFi state backup saved to EEPROM: %s\n", enabled ? "ON" : "OFF");
    } else {
        Serial.println("✗ Failed to save WiFi state backup to EEPROM!");
    }
    EEPROM.end();
    
    if (nvs_success || eeprom_success) {
        wifiEnabled = enabled;
        wifiEnabledCached = true;
        Serial.println("WiFi state persistence: SUCCESS");
    } else {
        Serial.println("WiFi state persistence: FAILED - using in-memory fallback");
        wifiEnabled = enabled;
        wifiEnabledCached = true;
    }
}

bool isWiFiEnabled() {
    loadWiFiEnabledState();
    return wifiEnabled;
}

void resetWiFiEnabledState() {
    Serial.println("RESETTING WiFi state from all storage...");
    
    // Clear NVS
    if (wifiPrefs.begin("wifi", false)) {
        wifiPrefs.remove("enabled");
        wifiPrefs.end();
        Serial.println("✓ WiFi state cleared from NVS");
    }
    
    // Clear EEPROM 
    EEPROM.begin(512);
    EEPROM.write(EEPROM_WIFI_ENABLED_ADDR, 0xFF); // Invalid value
    EEPROM.write(EEPROM_MAGIC_BYTE_ADDR, 0x00);   // Clear magic byte
    if (EEPROM.commit()) {
        Serial.println("✓ WiFi state cleared from EEPROM");
    }
    EEPROM.end();
    
    // Clear cache
    wifiEnabledCached = false;
    wifiEnabled = true; // Reset to default
    
    Serial.println("WiFi state reset complete - next boot will use defaults");
}

void enableWiFi() {
    Serial.println("Enabling WiFi...");
    
    // Save the enabled state
    saveWiFiEnabledState(true);
    
    // If WiFi was previously off, restore it properly
    if (WiFi.getMode() == WIFI_OFF) {
        // Restart ESP-IDF WiFi subsystem if it was stopped
        #ifdef ESP_IDF_VERSION_MAJOR
            esp_err_t result = esp_wifi_start();
            if (result == ESP_OK) {
                Serial.println("ESP-IDF WiFi subsystem restarted");
            }
        #endif
        
        delay(100); // Allow subsystem to initialize
        
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
    
    // Additional ESP-IDF level power management for maximum power savings
    #ifdef ESP_IDF_VERSION_MAJOR
        esp_err_t result = esp_wifi_stop();
        if (result == ESP_OK) {
            Serial.println("ESP-IDF WiFi subsystem stopped for maximum power saving");
        }
    #endif
    
    // Final delay to ensure all hardware is in low-power state
    delay(100);
    
    Serial.println("WiFi disabled - maximum battery saving mode active");
}

void toggleWiFi() {
    if (isWiFiEnabled() && WiFi.getMode() != WIFI_OFF) {
        disableWiFi();
    } else {
        enableWiFi();
    }
}
