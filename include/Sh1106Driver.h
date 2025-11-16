// Sh1106Driver.h
#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>

class Sh1106Driver {
public:
    Sh1106Driver(uint16_t width,
                 uint16_t height,
                 TwoWire* wire,
                 int8_t resetPin)
        : oled(width, height, wire, resetPin)
    {
    }

    // API kompatybilne z Ssd1306Driver

    bool begin(uint8_t /*vccSource*/, uint8_t i2cAddress) {
        bool ok = oled.begin(i2cAddress);
        if (!ok) return false;    
        oled.setRotation(2);
        oled.clearDisplay();
        return true;
    }


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

    void setFont(const GFXfont *f) {
        oled.setFont(f);
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

    // SH1106 też ma setContrast, ale jeśli nie ma – można to zaimplementować inaczej albo zostawić pustą
    void ssd1306_command(uint8_t cmd) {
        // SH1106 nie ma tej samej funkcji prywatnej, więc jeśli używasz tylko do kontrastu,
        // lepiej dorobić osobną metodę setContrast i wołać oled.setContrast().
        // Na razie zostawmy jako no-op, a kontrastem zajmiemy się później.
        (void)cmd;
    }

    int16_t width() const {
        return oled.width();
    }

    int16_t height() const {
        return oled.height();
    }

    // print/println – forward do Adafruit_GFX
    template<typename T>
    void print(const T& value) {
        oled.print(value);
    }

    void println() {
        oled.println();
    }

    template<typename T>
    void println(const T& value) {
        oled.println(value);
    }

private:
    // Dla SH1106 128x64 – jak masz inny, tu dopasujesz klasę / parametry
    Adafruit_SH1106G oled;
};
