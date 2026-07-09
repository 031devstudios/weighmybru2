#ifndef SCALE_H
#define SCALE_H

#include <HX711.h>
#include <Preferences.h>

class Scale {
public:
    Scale(uint8_t dataPin, uint8_t clockPin, float calibrationFactor);
    bool begin();  // Returns true if successful, false if HX711 fails
    void tare(uint8_t times = 20);
    void set_scale(float factor);
    float getWeight();
    float getCurrentWeight();
    long getRawValue();
    void saveCalibration(); // Save calibration factor to NVS
    void loadCalibration(); // Load calibration factor from NVS
    float getCalibrationFactor() const { return calibrationFactor; } // Getter for API
    bool isHX711Connected() const { return isConnected; } // Check if HX711 is responding
    
    // Filtering configuration - adjustable for different load cells
    void setBrewingThreshold(float threshold);
    void setStabilityTimeout(unsigned long timeout);
    void setMedianSamples(int samples);
    void setAverageSamples(int samples);
    
    float getBrewingThreshold() const { return brewingThreshold; }
    unsigned long getStabilityTimeout() const { return stabilityTimeout; }
    int getMedianSamples() const { return medianSamples; }
    int getAverageSamples() const { return averageSamples; }
    String getFilterState() const; // Get current filter state as string for debugging
    
    void saveFilterSettings();
    void loadFilterSettings();
    
    // FlowRate integration for tare operations
    void setFlowRatePtr(class FlowRate* flowRatePtr);
    void sleep();
private:
    HX711 hx711;
    Preferences preferences;
    uint8_t dataPin;
    uint8_t clockPin;
    float calibrationFactor = 0.0f;
    float currentWeight;
    bool isConnected = false;  // Track HX711 connection status
    class FlowRate* flowRatePtr = nullptr; // For pausing flow rate during tare
    
    // Smart filtering variables - reduced buffer for faster response
    static const int MAX_SAMPLES = 10;  // Reduced from 50 to 10 for faster response
    float readings[MAX_SAMPLES];
    int readingIndex = 0;
    bool samplesInitialized = false;
    float previousFilteredWeight = 0;
    
    // Brewing state tracking for smart filtering
    enum FilterState {
        STABLE,     // Using average filter - stable weight
        BREWING,    // Using median filter - active brewing
        TRANSITIONING // Waiting for stability after brewing activity
    };
    FilterState currentFilterState = STABLE;
    unsigned long lastBrewingActivity = 0;  // Track when brewing was last detected
    float lastStableWeight = 0.0f;          // Last weight when in stable state
    
    // Configurable filtering parameters
    float brewingThreshold = 0.15f;  // Keep for API compatibility
    unsigned long stabilityTimeout = 2000;  // Keep for API compatibility
    int medianSamples = 3;  // Keep for API compatibility
    int averageSamples = 2;  // Samples for average filter - reduced for faster response
    
    // Filter methods
    float medianFilter(int samples);
    float averageFilter(int samples);
    void initializeSamples(float initialValue);
};

#endif
