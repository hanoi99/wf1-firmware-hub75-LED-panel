#pragma once

#include "BaseScene.h"

// Night drive: a dark green sedan seen diagonally from the front (or from the side), the road with its dashed centre
// line streaming towards the viewer and street lamps passing by. Fully procedural.
class DriveScene : public BaseScene {
public:
    explicit DriveScene(DisplayDriver& display);
    const char* id() const override { return "drive"; }
    const char* title() const override { return "Nachtfahrt"; }
    uint16_t frameIntervalMs() const override { return 40; }
    void begin() override;

protected:
    void render(uint32_t now) override;

private:
    enum { O_VIEW, O_SPEED, O_LAMPS };
    struct Star { uint8_t x, y, phase; };
    void drawCar(int ox, int oy, int spin);
    void renderSide(uint32_t now);
    void drawSideCar(int ox, int oy, int spin);
    static constexpr int N_STARS = 22;
    Star stars_[N_STARS]{};
    int scroll_ = 0;
};
