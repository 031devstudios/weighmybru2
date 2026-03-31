#pragma once

#define FLOWRATE_AVG_WINDOW 20  // Increased for better smoothing
#define FLOWRATE_SAMPLE_WINDOW 10 // Samples for Auto Brew detection

class FlowRate {
public:
    FlowRate();
    void update(float currentWeight);
    float getFlowRate() const; // grams per second
    
    // Timer-based average flow rate tracking
    void startTimerAveraging();
    void stopTimerAveraging();
    void resetTimerAveraging();
    float getTimerAverageFlowRate() const;
    bool hasTimerAverage() const;
    
    // Tare operation support
    void pauseCalculation();  // Pause flow rate during tare operations
    void resumeCalculation(); // Resume flow rate after tare completes
    void clearFlowRateBuffer(); // Clear all flow rate history for fresh start
    
    // Auto Brew Timer support - sample buffer for slope calculation
    void addFlowSample(float flowRate); // Add sample to circular buffer
    void clearFlowSamples(); // Clear sample buffer (for tare)
    int getFlowSampleCount() const; // Get number of samples in buffer
    void getFlowSamples(float samples[], int maxSamples) const; // Get all samples
    void getFlowSampleTimestamps(unsigned long timestamps[], int maxSamples) const; // Get timestamps

private:
    float lastWeight;
    unsigned long lastTime;
    float flowRate;
    float flowRateBuffer[FLOWRATE_AVG_WINDOW];
    int bufferIndex;
    int bufferCount;
    
    // Timer-based average tracking
    bool timerAveragingActive;
    float timerFlowRateSum;
    int timerFlowRateSamples;
    float timerAverageFlowRate;
    bool hasValidTimerAverage;
    bool calculationPaused; // Flag to pause flow rate during tare operations
    
    // Auto Brew Timer sample buffer
    float flowSamples[FLOWRATE_SAMPLE_WINDOW];
    unsigned long flowSampleTimestamps[FLOWRATE_SAMPLE_WINDOW];
    int flowSampleIndex;
    int flowSampleCount;
    
    // Flow rate filtering parameters
    static constexpr float WEIGHT_DEADBAND = 0.08f;     // Increased deadband for load cell noise
    static constexpr float MIN_DELTA_TIME = 0.15f;      // Increased to 150ms for much more stable readings
    static constexpr float ZERO_THRESHOLD = 0.08f;      // Increased zero threshold
    static constexpr float RAPID_CHANGE_THRESHOLD = 1.5f; // Higher threshold for major changes
    static constexpr float NEGATIVE_CHANGE_THRESHOLD = 0.5f; // Higher threshold for weight removal
    
    // Helper methods
    float calculateStableAverage(bool isWeightRemoval);
};
