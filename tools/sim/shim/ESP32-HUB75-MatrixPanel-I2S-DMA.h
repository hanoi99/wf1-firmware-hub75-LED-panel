#pragma once
// Mock der HUB75-Library: gleiche API-Teilmenge, Doppelpuffer wie das Original
// (nach flip enthält der neue Rückpuffer den veralteten Frame von vor 2 Frames).
#include "Arduino.h"
#include "Adafruit_GFX.h"
struct HUB75_I2S_CFG {
    enum shift_driver { SHIFTREG = 0, FM6124, FM6126A, ICN2038S, MBI5124, DP3246 };
    enum clk_speed { HZ_8M = 8000000, HZ_16M = 16000000, HZ_20M = 20000000 };
    struct i2s_pins { int8_t r1, g1, b1, r2, g2, b2, a, b, c, d, e, lat, oe, clk; } gpio;
    uint16_t mx_width, mx_height, chain_length;
    shift_driver driver = SHIFTREG;
    bool double_buff = false;
    clk_speed i2sspeed = HZ_8M;
    uint8_t latch_blanking = 0;
    bool clkphase = true;
    void setPixelColorDepthBits(uint8_t b) {}
    uint16_t min_refresh_rate = 60;
    HUB75_I2S_CFG(uint16_t w, uint16_t h, uint16_t c, i2s_pins p) : gpio(p), mx_width(w), mx_height(h), chain_length(c) {}
};
class MatrixPanel_I2S_DMA : public Adafruit_GFX {
public:
    explicit MatrixPanel_I2S_DMA(const HUB75_I2S_CFG& c) : Adafruit_GFX(64, 64), cfg_(c) {}
    bool begin() { return true; }
    void drawPixel(int16_t x, int16_t y, uint16_t c) override {
        drawPixelRGB888(x, y, (c >> 8) & 0xF8, (c >> 3) & 0xFC, (c << 3) & 0xF8);
    }
    void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= 64 || y >= 64) return;
        uint8_t* p = &buf_[back_][(y * 64 + x) * 3]; p[0] = r; p[1] = g; p[2] = b;
    }
    void fillScreenRGB888(uint8_t r, uint8_t g, uint8_t b) { for (int i = 0; i < 4096; ++i) { buf_[back_][i*3] = r; buf_[back_][i*3+1] = g; buf_[back_][i*3+2] = b; } }
    void fillScreen(uint16_t c) override { fillScreenRGB888((c >> 8) & 0xF8, (c >> 3) & 0xFC, (c << 3) & 0xF8); }
    void clearScreen() { memset(buf_[back_], 0, sizeof(buf_[back_])); }
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
    void setBrightness8(uint8_t) {}
    void flipDMABuffer() { if (!cfg_.double_buff) return; back_ ^= 1; ++flips; }
    const uint8_t* front() const { return buf_[cfg_.double_buff ? back_ ^ 1 : back_]; }
    int flips = 0;
    int calculated_refresh_rate = 100;
private:
    HUB75_I2S_CFG cfg_;
    uint8_t buf_[2][64 * 64 * 3]{};
    int back_ = 0;
};
