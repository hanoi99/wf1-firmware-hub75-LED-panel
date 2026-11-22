#pragma once

#include "BaseScene.h"

// "Matrix" code rain on a 16x10 character grid (4x6 px cells, 3x5 font).
class DigitalRainScene : public BaseScene {
public:
    explicit DigitalRainScene(DisplayDriver& display);
    const char* id() const override { return "rain"; }
    const char* title() const override { return "Matrix-Regen"; }
    uint16_t frameIntervalMs() const override { return 50; }
    void begin() override;

protected:
    void render(uint32_t now) override;

private:
    enum { O_COLOR, O_SPEED, O_DENS };
    static constexpr int COLS = 16, ROWS = 10;
    struct Column { int8_t head; uint8_t len, wait; uint16_t acc; bool active; };
    void respawn(Column& c, bool initial);
    Column col_[COLS];
    char glyph_[ROWS][COLS];
};
