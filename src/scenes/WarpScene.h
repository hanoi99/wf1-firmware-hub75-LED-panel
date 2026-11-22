#pragma once

#include "BaseScene.h"

// Hyperspace fly-through: stars streak outward from the centre.
class WarpScene : public BaseScene {
public:
    explicit WarpScene(DisplayDriver& display);
    const char* id() const override { return "warp"; }
    const char* title() const override { return "Hyperraum"; }
    uint16_t frameIntervalMs() const override { return 33; }
    void begin() override;

protected:
    void render(uint32_t now) override;

private:
    enum { O_SPEED, O_TRAIL, O_COLOR };
    static constexpr int N = 70;
    struct Star { int16_t x, y; int32_t z; uint8_t hue; };
    void respawn(Star& s);
    static bool project(const Star& s, int32_t z, int& px, int& py);
    Star stars_[N];
};
