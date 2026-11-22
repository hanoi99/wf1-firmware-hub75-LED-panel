#pragma once

#include "BaseScene.h"
#include "MapView.h"
#include "services/TimeService.h"

// World map with live day/night shading, terminator curve and sun position; optionally with the time.
class WorldClockScene : public BaseScene {
public:
    WorldClockScene(DisplayDriver& display, TimeService& time);
    const char* id() const override { return "world"; }
    const char* title() const override { return "Weltkarte"; }
    const char* category() const override { return "info"; }
    bool available() const override { return time_.isValid(); }
    uint16_t frameIntervalMs() const override { return 200; }
    void begin() override { lastShadeMs_ = 0; lastSy_ = 0; }

protected:
    void render(uint32_t now) override;

private:
    enum { O_VIEW, O_ZONE, O_SECS, O_NIGHT, O_HOME };
    TimeService& time_;
    MapView map_;
    uint32_t lastShadeMs_ = 0;
    uint8_t lastSy_ = 0;
};
