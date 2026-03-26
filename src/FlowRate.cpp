#include "FlowRate.h"
#include "Calibration.h"
#include <Arduino.h>

FlowRate::FlowRate() : lastWeight(0), lastTime(0), flowRate(0), bufferIndex(0), bufferCount(0),
    timerAveragingActive(false), timerFlowRateSum(0), timerFlowRateSamples(0), 
    timerAverageFlowRate(0), hasValidTimerAverage(false), calculationPaused(false) {
    for (int i = 0; i < FLOWRATE_AVG_WINDOW; ++i) flowRateBuffer[i] = 0;
}

void FlowRate::update(float currentWeight) {
    // Skip flow rate calculation if paused (during tare operations)
    if (calculationPaused) {
        return;
    }
    
    unsigned long now = millis();
    
    if (lastTime > 0) {
        float deltaWeight = currentWeight - lastWeight;
        float deltaTime = (now - lastTime) / 1000.0f; // seconds
        
        // Only update if enough time has passed for meaningful calculation
        if (deltaTime >= MIN_DELTA_TIME) {
            // Simple tare transition detection
            bool tareTransition = false;
            
            // Check for tare transition: negative weight going to near zero
            if (lastWeight < -5.0f && abs(currentWeight) < 2.0f) {
                tareTransition = true;
            }
            
            // Check for large weight jump (likely tare operation)
            if (abs(deltaWeight) > 50.0f) {
                tareTransition = true;
            }
            
            if (!tareTransition) {
                // Detect different types of changes with higher thresholds
                bool majorWeightIncrease = deltaWeight > RAPID_CHANGE_THRESHOLD;
                bool weightRemoval = deltaWeight < -NEGATIVE_CHANGE_THRESHOLD;
                
                // Apply stronger deadband filter for load cell noise
                if (abs(deltaWeight) < WEIGHT_DEADBAND) {
                    deltaWeight = 0.0f;
                }
                
                if (deltaTime > 0) {
                    float instantRate = deltaWeight / deltaTime;
                    
                    // Only clear buffer for significant weight removal
                    if (weightRemoval && abs(deltaWeight) > 1.0f) {
                        // Major weight removed - reset buffer for fast zero response
                        for (int i = 0; i < FLOWRATE_AVG_WINDOW; i++) {
                            flowRateBuffer[i] = 0.0f;
                        }
                        bufferCount = 1;
                        bufferIndex = 0;
                        flowRateBuffer[0] = instantRate;
                    } else {
                        // Normal operation - store in circular buffer
                        flowRateBuffer[bufferIndex] = instantRate;
                        bufferIndex = (bufferIndex + 1) % FLOWRATE_AVG_WINDOW;
                        if (bufferCount < FLOWRATE_AVG_WINDOW) bufferCount++;
                    }
                    
                    // Calculate stable average with consistent smoothing
                    flowRate = calculateStableAverage(weightRemoval);
                    
                    // Track flow rate for timer-based averaging (only when positive flow)
                    if (timerAveragingActive && flowRate > 0.1f) { // Only count meaningful positive flow
                        timerFlowRateSum += flowRate;
                        timerFlowRateSamples++;
                    }
                    
                    // Apply zero threshold to eliminate tiny fluctuations
                    if (abs(flowRate) < ZERO_THRESHOLD) {
                        flowRate = 0.0f;
                    }
                }
            } else {
                // Tare transition detected - set flow rate to zero
                flowRate = 0.0f;
            }
            
            lastWeight = currentWeight;
            lastTime = now;
        }
    } else {
        // First update - just store the values
        lastWeight = currentWeight;
        lastTime = now;
    }
}

float FlowRate::calculateStableAverage(bool isWeightRemoval) {
    if (bufferCount == 0) return 0.0f;
    
    if (isWeightRemoval) {
        // For weight removal, use fewer samples for faster zero response
        int samplesToUse = min(5, bufferCount);
        float sum = 0.0f;
        for (int i = 0; i < samplesToUse; i++) {
            int index = (bufferIndex - 1 - i + FLOWRATE_AVG_WINDOW) % FLOWRATE_AVG_WINDOW;
            sum += flowRateBuffer[index];
        }
        return sum / samplesToUse;
    } else {
        // Normal operation - use gentle linear weighting for maximum stability
        float weightedSum = 0.0f;
        float totalWeight = 0.0f;
        
        // Use more samples with gentle linear weighting (not exponential)
        int samplesToUse = min(bufferCount, FLOWRATE_AVG_WINDOW);
        for (int i = 0; i < samplesToUse; i++) {
            int index = (bufferIndex - 1 - i + FLOWRATE_AVG_WINDOW) % FLOWRATE_AVG_WINDOW;
            // Gentle linear weighting: recent samples slightly more important
            float weight = 1.0f + (0.05f * (samplesToUse - i)); // Very gentle weighting
            weightedSum += flowRateBuffer[index] * weight;
            totalWeight += weight;
        }
        
        return weightedSum / totalWeight;
    }
}

float FlowRate::getFlowRate() const {
    return flowRate;
}

// Timer-based average flow rate methods
void FlowRate::startTimerAveraging() {
    timerAveragingActive = true;
    timerFlowRateSum = 0;
    timerFlowRateSamples = 0;
    hasValidTimerAverage = false;
    Serial.println("Started timer-based flow rate averaging");
}

void FlowRate::stopTimerAveraging() {
    if (timerAveragingActive && timerFlowRateSamples > 0) {
        timerAverageFlowRate = timerFlowRateSum / timerFlowRateSamples;
        hasValidTimerAverage = true;
        Serial.printf("Timer flow rate average: %.2f g/s (from %d samples)\n", 
                     timerAverageFlowRate, timerFlowRateSamples);
    } else {
        timerAverageFlowRate = 0;
        hasValidTimerAverage = false;
        Serial.println("No valid flow rate samples during timer period");
    }
    timerAveragingActive = false;
}

void FlowRate::resetTimerAveraging() {
    timerAveragingActive = false;
    timerFlowRateSum = 0;
    timerFlowRateSamples = 0;
    timerAverageFlowRate = 0;
    hasValidTimerAverage = false;
    Serial.println("Timer averaging reset");
}

float FlowRate::getTimerAverageFlowRate() const {
    return hasValidTimerAverage ? timerAverageFlowRate : 0.0f;
}

bool FlowRate::hasTimerAverage() const {
    return hasValidTimerAverage;
}

void FlowRate::pauseCalculation() {
    calculationPaused = true;
    Serial.println("Flow rate calculation paused");
}

void FlowRate::resumeCalculation() {
    calculationPaused = false;
    // Reset timing to avoid using old weight data
    lastTime = millis();
    Serial.println("Flow rate calculation resumed");
}

void FlowRate::clearFlowRateBuffer() {
    // Clear all flow rate history and reset to zero state
    for (int i = 0; i < FLOWRATE_AVG_WINDOW; i++) {
        flowRateBuffer[i] = 0.0f;
    }
    bufferIndex = 0;
    bufferCount = 0;
    flowRate = 0.0f;
    lastWeight = 0.0f;
    lastTime = 0;
    Serial.println("Flow rate buffer cleared for fresh start");
}

void FlowRate::addFlowSample(float flowRateValue) {
    flowSamples[flowSampleIndex] = flowRateValue;
    flowSampleTimestamps[flowSampleIndex] = millis();
    flowSampleIndex = (flowSampleIndex + 1) % FLOWRATE_SAMPLE_WINDOW;
    if (flowSampleCount < FLOWRATE_SAMPLE_WINDOW) {
        flowSampleCount++;
    }
}

void FlowRate::clearFlowSamples() {
    for (int i = 0; i < FLOWRATE_SAMPLE_WINDOW; i++) {
        flowSamples[i] = 0.0f;
        flowSampleTimestamps[i] = 0;
    }
    flowSampleIndex = 0;
    flowSampleCount = 0;
}

int FlowRate::getFlowSampleCount() const {
    return flowSampleCount;
}

void FlowRate::getFlowSamples(float samples[], int maxSamples) const {
    int count = min(flowSampleCount, maxSamples);
    for (int i = 0; i < count; i++) {
        int index = (flowSampleIndex - count + i + FLOWRATE_SAMPLE_WINDOW) % FLOWRATE_SAMPLE_WINDOW;
        samples[i] = flowSamples[index];
    }
}

void FlowRate::getFlowSampleTimestamps(unsigned long timestamps[], int maxSamples) const {
    int count = min(flowSampleCount, maxSamples);
    for (int i = 0; i < count; i++) {
        int index = (flowSampleIndex - count + i + FLOWRATE_SAMPLE_WINDOW) % FLOWRATE_SAMPLE_WINDOW;
        timestamps[i] = flowSampleTimestamps[index];
    }
}

