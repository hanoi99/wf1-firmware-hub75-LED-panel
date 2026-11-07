#pragma once

#include "BaseScene.h"

// Night sky: twinkling coloured stars, faint Milky Way band, shooting stars.
class StarfieldScene : public BaseScene {
public:
    explicit StarfieldScene(DisplayDriver& display);
    const char* id() const override { return "stars"; }
    const char* title() const override { return "Sternenhimmel"; }
    uint16_t frameIntervalMs() const override { return 40; }
    void begin() override;

protected:
    void render(uint32_t now) override;

private:
    enum { O_DENS, O_TWINKLE, O_MW, O_SHOOT };
    static constexpr int MAX_STARS = 130;
    struct Star { uint8_t x, y, base, phase, rate, tint; };
    struct Shooter { int16_t x, y, vx, vy; uint8_t life; };
    void generate();
    Star stars_[MAX_STARS];
    uint8_t milky_[64 * 64];
    Shooter shooter_{};
};
