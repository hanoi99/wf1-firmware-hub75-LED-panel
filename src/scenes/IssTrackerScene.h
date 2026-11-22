#pragma once

#include "BaseScene.h"
#include "MapView.h"
#include "services/IssService.h"
#include "services/TimeService.h"

// Live ISS position on the world map with predicted ground track. Position data
// comes from IssService (background HTTP task); the orbit model keeps it moving.
class IssTrackerScene : public BaseScene {
public:
    IssTrackerScene(DisplayDriver& display, IssService& iss, TimeService& time);
    const char* id() const override { return "iss"; }
    const char* title() const override { return "ISS-Tracker"; }
    const char* category() const override { return "info"; }
    bool available() const override { return iss_.lastOkMs() != 0 || iss_.failures() < 3; }
    uint16_t frameIntervalMs() const override { return 100; }
    void begin() override;
    void end() override;
    String info() const override;

protected:
    void render(uint32_t now) override;

private:
    enum { O_TRACK, O_NIGHT, O_INFO };
    static constexpr int PAST_PTS = 30, FUTURE_PTS = 60, TRACK_STEP_S = 90;
    void rebuildTrack(const IssOrbit& orbit, int32_t ageSec);

    IssService& iss_;
    TimeService& time_;
    MapView map_;
    uint32_t lastShadeMs_ = 0, trackFixMs_ = 0, trackBuiltMs_ = 0;
    uint8_t tx_[PAST_PTS + FUTURE_PTS], ty_[PAST_PTS + FUTURE_PTS];
    bool trackValid_ = false;
    float lat_ = 0, lon_ = 0;
    bool haveFix_ = false;
};
