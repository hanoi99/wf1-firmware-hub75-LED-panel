#pragma once

#include "BaseScene.h"

// Night at sea: a small island with a palm tree, the moon in the corner with its glittering
// reflection, twinkling stars and slowly drifting clouds. Fully procedural, tiny RAM footprint.
class MoonIslandScene : public BaseScene {
public:
    explicit MoonIslandScene(DisplayDriver& display);
    const char* id() const override { return "island"; }
    const char* title() const override { return "Mondinsel"; }
    uint16_t frameIntervalMs() const override { return 60; }
    void begin() override;

protected:
    void render(uint32_t now) override;

private:
    enum { O_WAVES, O_STARS, O_CLOUDS };
    static constexpr int HORIZON = 36;
    static constexpr int N_STARS = 36;
    struct Star { uint8_t x, y, phase, base; };
    bool island(int x, int y) const;       // island + palm silhouette at (x, y), y < HORIZON
    Star stars_[N_STARS]{};
    int cloudX_[2]{};
};
