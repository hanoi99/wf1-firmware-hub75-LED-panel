#pragma once

#include "BaseScene.h"
#include "services/TimeService.h"

class ClockScene : public BaseScene {
public:
    ClockScene(DisplayDriver& display, TimeService& time);
    const char* id() const override { return "clock"; }
    const char* title() const override { return "Uhr"; }
    const char* category() const override { return "info"; }
    bool available() const override { return time_.isValid(); }
    uint16_t frameIntervalMs() const override { return 100; }
    void begin() override {}

protected:
    void render(uint32_t now) override;

private:
    enum { O_STYLE, O_H24, O_SECS, O_COLOR };
    void digital(const struct tm& t, uint32_t now);
    void analog(const struct tm& t);
    TimeService& time_;
};
