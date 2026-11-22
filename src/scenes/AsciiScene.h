#pragma once

#include "BaseScene.h"

// ASCII-art effects: plasma / fire / tunnel rendered as characters on a 16x10 grid.
class AsciiScene : public BaseScene {
public:
    explicit AsciiScene(DisplayDriver& display);
    const char* id() const override { return "ascii"; }
    const char* title() const override { return "ASCII-Art"; }
    uint16_t frameIntervalMs() const override { return 60; }
    void begin() override;

protected:
    void render(uint32_t now) override;

private:
    enum { O_MODE, O_COLOR, O_SPEED };
    static constexpr int COLS = 16, ROWS = 10, FIRE_ROWS = 14;
    void fieldPlasma(uint32_t t);
    void fieldTunnel(uint32_t t);
    void fieldFire();
    uint8_t value_[ROWS][COLS];
    uint8_t heat_[FIRE_ROWS][COLS];
    uint8_t dist_[ROWS][COLS], ang_[ROWS][COLS];
};
