#pragma once

#include "BaseScene.h"

// Endless Mandelbrot zoom (fixed-point, no FPU needed) with palette cycling.
class MandelbrotScene : public BaseScene {
public:
    explicit MandelbrotScene(DisplayDriver& display);
    const char* id() const override { return "mandel"; }
    const char* title() const override { return "Mandelbrot"; }
    uint16_t frameIntervalMs() const override { return 80; }
    void begin() override;

protected:
    void render(uint32_t now) override;
    void onOption(uint8_t i) override { if (i == O_PAL) buildPalette(); }

private:
    enum { O_PAL, O_SPEED, O_CYCLE };
    void buildPalette();
    void nextTarget();
    double scale_ = 1.0;     // 1 = full view, small = deep zoom
    int zoomDir_ = -1;       // -1 in, +1 out
    uint8_t target_ = 0;
    uint16_t holdFrames_ = 0;
    fm::RGB palette_[256];
};
