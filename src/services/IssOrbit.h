#pragma once
#include <stdint.h>

// Circular-orbit model of the ISS (inclination 51.64 deg, period ~92.7 min).
// It is seeded with real position fixes from the API and is used to draw the
// ground track and to keep the marker moving smoothly between/after fixes.
class IssOrbit {
public:
    // Feed a new fix. nowMs: local millis() timestamp of the fix.
    void addFix(float latDeg, float lonDeg, uint32_t nowMs);
    bool hasFix() const { return hasFix_; }
    bool ready() const { return dirKnown_; }  // direction (ascending/descending) known
    // Position dtSec seconds after the latest fix (negative = before).
    void positionAt(int32_t dtSec, float& latDeg, float& lonDeg) const;
    uint32_t fixMs() const { return fixMs_; }
    bool ascending() const { return ascending_; }

private:
    bool hasFix_ = false, dirKnown_ = false, ascending_ = true;
    float lat_ = 0, lon_ = 0;
    uint32_t fixMs_ = 0;
    // older reference fix for direction detection
    bool hasPrev_ = false;
    float prevLat_ = 0;
    uint32_t prevMs_ = 0;
    float u0_ = 0;  // argument of latitude at the latest fix (rad)
    void computeU();
};
