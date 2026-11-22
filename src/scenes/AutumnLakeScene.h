#pragma once

#include "BaseScene.h"
#include "Scratch.h"

// Autumn tree on a headland at a lake: sunset/dawn/day sky, hills, rippling mirror image in the water
// and slowly falling leaves. The still image is kept as palette indices in the shared scratch block.
class AutumnLakeScene : public BaseScene {
public:
    explicit AutumnLakeScene(DisplayDriver& display);
    const char* id() const override { return "autumn"; }
    const char* title() const override { return "Herbstbaum am See"; }
    uint16_t frameIntervalMs() const override { return 60; }
    void begin() override;

protected:
    void render(uint32_t now) override;
    void onOption(uint8_t i) override { if (i == O_MOOD && scratch::owns(this)) build(); }

private:
    enum { O_MOOD, O_LEAVES, O_RIPPLE };
    static constexpr int HORIZON = 46;     // rows 0..45 scene, 46..63 water
    static constexpr int MAX_LEAVES = 12;
    struct Leaf { int16_t x, y, vx, vy; uint8_t col, life; };
    void build();
    void buildPalette();
    uint8_t* img_;                         // 64 x HORIZON palette indices (shared scratch)
    fm::RGB (&pal_)[256];
    Leaf leaves_[MAX_LEAVES]{};
};
