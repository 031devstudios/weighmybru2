#pragma once

#include <Arduino.h>
#include "config.h"
#include "Fonts/FreeMonoBold18pt7b.h"
#include "Fonts/FreeMonoBold12pt7b.h"
#include "Fonts/FreeMonoBold9pt7b.h"

struct DisplayState {
    float weight = 0.0f;
    float flowRate = 0.0f;
    float timerSeconds = 0.0f;

    bool btConnected = false;
    int  batteryPercent = -1;
    bool batteryCritical = false;

    bool wifiEnabled = false;
    bool wifiConnected = false;

    bool scaleConnected = false;
};

template<typename Driver>
class IDisplayLayout {
public:
    virtual ~IDisplayLayout() = default;
    virtual void render(Driver& driver, const DisplayState& state) = 0;
};

template<typename Driver>
class ClassicLayout64 : public IDisplayLayout<Driver> {
public:
    void render(Driver& display, const DisplayState& st) override {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1); 

        // ================== WEIGHT ==================
        float displayWeight = st.weight;
        if (displayWeight >= -0.1f && displayWeight <= 0.1f) {
            displayWeight = 0.0f;
        }

        bool weightNegative = (displayWeight < 0.0f);
        float absWeight = fabs(displayWeight);
        int weightInteger = static_cast<int>(absWeight);
        int weightDecimal = static_cast<int>((absWeight - weightInteger) * 10.0f + 0.5f);


        if (weightDecimal >= 10) {
            weightInteger += 1;
            weightDecimal = 0;
        }

        char weightBuf[16];
        if (weightNegative) {
            snprintf(weightBuf, sizeof(weightBuf), "-%d.%01d", weightInteger, weightDecimal);
        } else {
            snprintf(weightBuf, sizeof(weightBuf), "%d.%01d", weightInteger, weightDecimal);
        }

        display.setFont(&FreeMonoBold18pt7b);
        display.setCursor(0, 22);   // jak w templatce
        display.print(weightBuf);

        // ================== TIMER (M:SS.d) ==================
        float currentTime = st.timerSeconds;
        bool timerNegative = (currentTime < 0.0f);
        float absTimer = fabs(currentTime);

        int totalSeconds = static_cast<int>(absTimer);
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        int tenths = static_cast<int>((absTimer - totalSeconds) * 10.0f + 0.5f);

        if (tenths >= 10) {
            tenths = 0;
            seconds += 1;
            if (seconds >= 60) {
                seconds = 0;
                minutes += 1;
            }
        }

        char timerBuf[16];
        if (timerNegative) {
            snprintf(timerBuf, sizeof(timerBuf), "-%d:%02d.%d", minutes, seconds, tenths);
        } else {
            snprintf(timerBuf, sizeof(timerBuf), "%d:%02d.%d", minutes, seconds, tenths);
        }

        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(0, 39);  
        display.print(timerBuf);

        // ================== FLOW (RRR.R) ==================
        float currentFlowRate = st.flowRate;

        float displayFlowRate = currentFlowRate;
        if (displayFlowRate >= -0.1f && displayFlowRate <= 0.1f) {
            displayFlowRate = 0.0f;
        }

        bool flowNegative = (displayFlowRate < 0.0f);
        float absFlow = fabs(displayFlowRate);
        int flowInteger = static_cast<int>(absFlow);
        int flowDecimal = static_cast<int>((absFlow - flowInteger) * 10.0f + 0.5f);

        if (flowDecimal >= 10) {
            flowInteger += 1;
            flowDecimal = 0;
        }

        char flowBuf[16];
        if (flowNegative) {
            snprintf(flowBuf, sizeof(flowBuf), "-%d.%d", flowInteger, flowDecimal);
        } else {
            snprintf(flowBuf, sizeof(flowBuf), "%d.%d", flowInteger, flowDecimal);
        }

        float barFlow = currentFlowRate;
        if (barFlow < 0.0f) {
            barFlow = 0.0f;
        }

        const float FLOW_MAX = 15.0f;
        int barWidth = 0;

        if (barFlow <= 0.0f) {
            barWidth = 0;
        } else if (barFlow >= FLOW_MAX) {
            barWidth = 128;
        } else {
            barWidth = static_cast<int>((barFlow / FLOW_MAX) * 128.0f + 0.5f);
        }

        if (barWidth > 0) {
            display.fillRect(0, 60, barWidth, 4, 1);
        }

        // FLOW TEXT + BT + WIFI
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(0, 57);
        display.print(flowBuf);

        // display.setFont(&FreeMonoBold9pt7b);
        // display.setCursor(104, 40);
        // display.println("BT");

        // display.setCursor(104, 54);
        // display.print("WI");

        display.display();
    }
};

template<typename Driver>
class ClassicLayout32 : public IDisplayLayout<Driver> {
public:
    void render(Driver& display, const DisplayState& st){
        int16_t x1, y1;
    uint16_t w, h;

    display.clearDisplay();

    // --- Weight with deadband and custom formatting ---

    float displayWeight = st.weight;
    if (displayWeight >= -0.1f && displayWeight <= 0.1f) {
        displayWeight = 0.0f;
    }

    bool isNegative = displayWeight < 0;
    float absWeight = fabs(displayWeight);
    int integerPart = (int)absWeight;
    int decimalPart = (int)((absWeight - integerPart) * 10.0f + 0.5f);

    if (decimalPart >= 10) {
        integerPart += 1;
        decimalPart = 0;
    }

    display.setTextSize(3);
    int weightY = 5;
    display.setCursor(0, weightY);

    int currentX = 0;
    if (isNegative) {
        display.print("-");
        display.getTextBounds("-", 0, 0, &x1, &y1, &w, &h);
        currentX += w;
    }

    String intStr = String(integerPart);
    display.setCursor(currentX, weightY);
    display.print(intStr);

    display.getTextBounds(intStr, 0, 0, &x1, &y1, &w, &h);
    currentX += w;

    display.setTextSize(1);
    display.setCursor(currentX, weightY + 11);
    display.print(".");
    display.getTextBounds(".", 0, 0, &x1, &y1, &w, &h);
    currentX += w;

    display.setTextSize(2);
    display.setCursor(currentX, weightY + 3);
    display.print(String(decimalPart));

    // --- Timer (T) ---

    float currentTime = st.timerSeconds;
    bool timerNegative = currentTime < 0;
    float absTimer = fabs(currentTime);
    int timerInteger = (int)absTimer;
    int timerDecimal = (int)((absTimer - timerInteger) * 10.0f + 0.5f);

    if (timerDecimal >= 10) {
        timerInteger += 1;
        timerDecimal = 0;
    }

    display.setTextSize(2);
    String timerIntStr = String(timerInteger);
    if (timerNegative) {
        timerIntStr = "-" + timerIntStr;
    }

    uint16_t timerIntWidth, timerDecWidth, timerH, timerLabelWidth;
    display.getTextBounds(timerIntStr, 0, 0, &x1, &y1, &timerIntWidth, &timerH);

    display.setTextSize(1);
    display.getTextBounds("T", 0, 0, &x1, &y1, &timerLabelWidth, &timerH);
    display.getTextBounds(".", 0, 0, &x1, &y1, &w, &timerH);
    uint16_t timerDotWidth = w;
    display.getTextBounds(String(timerDecimal), 0, 0, &x1, &y1, &timerDecWidth, &timerH);

    int timerLabelX = DISPLAY_WIDTH - timerLabelWidth;
    int timerStartX = timerLabelX - timerIntWidth - timerDotWidth - timerDecWidth;

    display.setTextSize(2);
    display.setCursor(timerStartX, 0);
    display.print(timerIntStr);

    display.setTextSize(1);
    display.setCursor(timerStartX + timerIntWidth, 7);
    display.print(".");

    display.setCursor(timerStartX + timerIntWidth + timerDotWidth, 7);
    display.print(String(timerDecimal));

    display.setCursor(timerLabelX, 0);
    display.print("T");

    // --- Flow rate (F) ---

    float currentFlowRate = st.flowRate;

    float displayFlowRate = currentFlowRate;
    if (displayFlowRate >= -0.1f && displayFlowRate <= 0.1f) {
        displayFlowRate = 0.0f;
    }

    bool flowNegative = displayFlowRate < 0;
    float absFlow = fabs(displayFlowRate);
    int flowInteger = (int)absFlow;
    int flowDecimal = (int)((absFlow - flowInteger) * 10.0f + 0.5f);

    if (flowDecimal >= 10) {
        flowInteger += 1;
        flowDecimal = 0;
    }

    display.setTextSize(2);
    String flowIntStr = String(flowInteger);
    if (flowNegative) {
        flowIntStr = "-" + flowIntStr;
    }

    uint16_t flowIntWidth, flowDecWidth, flowH, flowLabelWidth;
    display.getTextBounds(flowIntStr, 0, 0, &x1, &y1, &flowIntWidth, &flowH);

    display.setTextSize(1);
    display.getTextBounds("F", 0, 0, &x1, &y1, &flowLabelWidth, &flowH);
    display.getTextBounds(".", 0, 0, &x1, &y1, &w, &flowH);
    uint16_t flowDotWidth = w;
    display.getTextBounds(String(flowDecimal), 0, 0, &x1, &y1, &flowDecWidth, &flowH);

    int flowLabelX = DISPLAY_WIDTH - flowLabelWidth;
    int flowStartX = flowLabelX - flowIntWidth - flowDotWidth - flowDecWidth;

    display.setTextSize(2);
    display.setCursor(flowStartX, 16);
    display.print(flowIntStr);

    display.setTextSize(1);
    display.setCursor(flowStartX + flowIntWidth, 23);
    display.print(".");

    display.setCursor(flowStartX + flowIntWidth + flowDotWidth, 23);
    display.print(String(flowDecimal));

    display.setCursor(flowLabelX, 16);
    display.print("F");

    display.display();
    }

};