#pragma once

#include "BaseScene.h"

// Slow psychedelic plasma / nebula clouds from sine lookup tables.
class PlasmaScene : public BaseScene {
public:
    explicit PlasmaScene(DisplayDriver& display);
    const char* id() const override { return "plasma"; }
    const char* title() const override { return "Plasma"; }
    uint16_t frameIntervalMs() const override { return 40; }
    void begin() override;

protected:
    void render(uint32_t now) override;
    void onOption(uint8_t i) override { if (i == O_PAL) pal::smooth(palette_, (int)val(O_PAL)); }

private:
    enum { O_PAL, O_SPEED, O_SCALE };
    fm::RGB palette_[256];
    uint8_t dist_[64 * 64];
};
