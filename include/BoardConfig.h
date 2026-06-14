#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Board identification and pin configuration

#ifdef BOARD_SUPERMINI
  #define BOARD_NAME "ESP32-S3-DevKitC-1 (SuperMini)"
  #define BOARD_TYPE_SUPERMINI
  
#elif defined(BOARD_XIAO)
  #define BOARD_NAME "XIAO ESP32S3" 
  #define BOARD_TYPE_XIAO
  
#else
  #define BOARD_NAME "ESP32-S3 (Unknown)"
  #define BOARD_TYPE_SUPERMINI  // Default fallback
  
#endif

// Pin definitions (currently identical for both boards)
#define HX711_DATA_PIN        5   // GPIO5 - HX711 Data pin
#define HX711_CLOCK_PIN       6   // GPIO6 - HX711 Clock pin  
#define TOUCH_TARE_PIN        4   // GPIO4 - Touch sensor for tare (T0)
#define TOUCH_SLEEP_PIN       3   // GPIO3 - Touch sensor for sleep functionality
#define BATTERY_PIN           7   // GPIO7 - Battery voltage monitoring (ADC1_CH6)
#define I2C_SDA_PIN           8   // GPIO8 - I2C Data pin for display
#define I2C_SCL_PIN           9   // GPIO9 - I2C Clock pin for display
#define OLED_POWER_PIN        2   // GPIO2 - Power control for OLED display
#define TOUCH_TARE_POWER_PIN  10  // GPIO10 - Power control for tare touch sensor

// Board-specific configurations
#ifdef BOARD_TYPE_SUPERMINI
  #define FLASH_SIZE_MB       4
  #define BOARD_DESCRIPTION   "ESP32-S3 SuperMini with 4MB Flash"
  
#elif defined(BOARD_TYPE_XIAO)
  #define FLASH_SIZE_MB       8
  #define BOARD_DESCRIPTION   "XIAO ESP32S3 with 8MB Flash"
  
#endif

// Common ESP32-S3 features available on both boards
#define HAS_WIFI            true
#define HAS_BLUETOOTH       true
#define HAS_PSRAM           true
#define HAS_TOUCH_SENSOR    true
#define ADC_RESOLUTION      12    // 12-bit ADC
#define PWM_RESOLUTION      8     // 8-bit PWM

#endif // BOARD_CONFIG_H