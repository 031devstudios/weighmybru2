// Ssd1306Driver.h
#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class Ssd1306Driver {
public:
    Ssd1306Driver(uint16_t width,
                  uint16_t height,
                  TwoWire* wire,
                  int8_t resetPin)
        : oled(width, height, wire, resetPin)
    {
    }

    bool begin(uint8_t vccSource, uint8_t i2cAddress) {
        return oled.begin(vccSource, i2cAddress);
    }

    // --- Methods mirroring Adafruit_SSD1306 API you use ---

    void clearDisplay() {
        oled.clearDisplay();
    }

    void display() {
        oled.display();
    }

    void cp437(bool x) {
        oled.cp437(x);
    }

    void setTextSize(uint8_t s) {
        oled.setTextSize(s);
    }

    void setTextColor(uint16_t c) {
        oled.setTextColor(c);
    }

    void setCursor(int16_t x, int16_t y) {
        oled.setCursor(x, y);
    }

    void drawRect(int16_t x, int16_t y,
                  int16_t w, int16_t h,
                  uint16_t color) {
        oled.drawRect(x, y, w, h, color);
    }

    void fillRect(int16_t x, int16_t y,
                  int16_t w, int16_t h,
                  uint16_t color) {
        oled.fillRect(x, y, w, h, color);
    }

    void getTextBounds(const String& str,
                       int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1,
                       uint16_t* w, uint16_t* h) {
        oled.getTextBounds(str, x, y, x1, y1, w, h);
    }

    void ssd1306_command(uint8_t cmd) {
        oled.ssd1306_command(cmd);
    }

    int16_t width() const {
        return oled.width();
    }

    int16_t height() const {
        return oled.height();
    }

    template<typename T>
    void print(const T& value) {
        oled.print(value);
    }

    void print(const char* s) {
        oled.print(s);
    }

private:
    Adafruit_SSD1306 oled;
};
