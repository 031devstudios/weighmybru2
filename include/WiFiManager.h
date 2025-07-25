#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>

void setupWiFi();
void saveWiFiCredentials(const char* ssid, const char* password);
void loadWiFiCredentials(char* ssid, char* password, size_t maxLen);
bool loadWiFiCredentialsFromEEPROM(); // Load and cache WiFi credentials from EEPROM
String getStoredSSID();
String getStoredPassword();
void setupmDNS(); // Setup mDNS for weighmybru.local hostname

#endif
