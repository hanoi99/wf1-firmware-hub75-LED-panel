#pragma once

#include "BaseScene.h"
#include "network/NetworkService.h"

// Diagnostics on the panel itself (no serial monitor needed): version, reset reason,
// uptime, WiFi state and free heap. Shown for a while after every boot.
class SysInfoScene : public BaseScene {
public:
    SysInfoScene(DisplayDriver& display, NetworkService& network, const char* version,
                 const char* resetReason, bool resetBad, uint8_t unstableBoots)
        : BaseScene(display), network_(network), version_(version), reason_(resetReason),
          bad_(resetBad), unstable_(unstableBoots) {}
    void setBoot(const char* reason, bool bad, uint8_t unstable) { reason_ = reason; bad_ = bad; unstable_ = unstable; }
    const char* id() const override { return "sysinfo"; }
    const char* title() const override { return "System-Info"; }
    const char* category() const override { return "info"; }
    uint16_t frameIntervalMs() const override { return 250; }
    void begin() override {}

protected:
    void render(uint32_t now) override;

private:
    NetworkService& network_;
    const char* version_;
    const char* reason_;
    bool bad_;
    uint8_t unstable_;
};
