#pragma once

#include <Arduino.h>
#include "IssOrbit.h"

// Fetches the ISS position in a background task so slow HTTP requests never
// stall the display loop. Uses plain HTTP (no TLS), which keeps the firmware small.
class IssService {
public:
    void begin();
    // Scenes tell the service whether they need data; no traffic otherwise.
    void setWanted(bool wanted) { wanted_ = wanted; }
    // Snapshot of the orbit model (fixes + direction). Returns false before the first fix.
    bool snapshot(IssOrbit& out) const;
    uint32_t lastOkMs() const { return lastOkMs_; }
    uint8_t failures() const { return failures_; }

private:
    static void taskEntry(void* self);
    void run();
    bool fetchOnce();

    volatile bool wanted_ = false;
    volatile uint32_t lastOkMs_ = 0;
    volatile uint8_t failures_ = 0;
    IssOrbit orbit_;
    mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};
