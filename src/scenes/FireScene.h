#pragma once

#include "BaseScene.h"

// Campfire: flames concentrated above two crossed logs, with rising sparks.
class FireScene : public BaseScene {
public:
    explicit FireScene(DisplayDriver& display);
    const char* id() const override { return "fire"; }
    const char* title() const override { return "Lagerfeuer"; }
    uint16_t frameIntervalMs() const override { return 50; }
    void begin() override;

protected:
    void render(uint32_t now) override;
    void onOption(uint8_t i) override { if (i == O_PAL) buildPalette(); }

private:
    enum { O_FLAME, O_PAL, O_LOGS, O_SPARKS };
    struct Spark { int16_t x, y, vx, vy; uint8_t life; };
    void buildPalette() { pal::heat(palette_, (int)val(O_PAL)); }
    void drawLogs();
    uint8_t heat_[64][64];
    fm::RGB palette_[256];
    Spark sparks_[12];
};
