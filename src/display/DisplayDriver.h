#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "display/DisplayTuning.h"

class DisplayDriver {
public:
    static constexpr int W = 64;
    static constexpr int H = 64;

    bool begin(const DisplayTuning& tuning);
    MatrixPanel_I2S_DMA& matrix();
    void clear();                       // clears the *back* buffer
    void present();                     // shows the frame just drawn (flip when double-buffered)
    void setBrightness(uint8_t brightness);
    uint8_t brightness() const { return brightness_; }
    bool doubleBuffered() const { return doubleBuf_; }
    const DisplayTuning& tuning() const { return tuning_; }   // what was requested
    int refreshHz() const { return refreshHz_; }
    uint8_t colorBits() const { return bitsUsed_; }              // colour depth actually in use                // refresh rate calculated by the library

    inline void px(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (static_cast<unsigned>(x) < W && static_cast<unsigned>(y) < H)
            matrix_->drawPixelRGB888(x, y, r, g, b);
    }
    inline void px(int x, int y, const uint8_t* rgb) { px(x, y, rgb[0], rgb[1], rgb[2]); }

private:
    MatrixPanel_I2S_DMA* matrix_ = nullptr;
    uint8_t brightness_ = 96;
    bool doubleBuf_ = false;
    DisplayTuning tuning_;
    int refreshHz_ = 0;
    uint8_t bitsUsed_ = 0;
};
