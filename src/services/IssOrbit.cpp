#include "IssOrbit.h"
#include <math.h>

namespace {
constexpr float INC = 51.64f * (float)M_PI / 180.0f;
constexpr float PERIOD_S = 5560.0f;
constexpr float OMEGA_E = 7.2921159e-5f;  // Earth rotation rad/s
constexpr float RAD = (float)M_PI / 180.0f;

float wrapPi(float a) {
    while (a > (float)M_PI) a -= 2.0f * (float)M_PI;
    while (a <= -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}
}  // namespace

void IssOrbit::computeU() {
    float s = sinf(lat_ * RAD) / sinf(INC);
    if (s > 1.0f) s = 1.0f;
    if (s < -1.0f) s = -1.0f;
    float u = asinf(s);
    if (!ascending_) u = (float)M_PI - u;
    u0_ = u;
}

void IssOrbit::addFix(float latDeg, float lonDeg, uint32_t nowMs) {
    // Direction from the latitude change against a fix that is at least 30 s old.
    if (hasFix_ && nowMs - fixMs_ >= 30000UL) {
        prevLat_ = lat_; prevMs_ = fixMs_; hasPrev_ = true;
    }
    if (hasPrev_) {
        const float dLat = latDeg - prevLat_;
        if (fabsf(dLat) > 0.05f) { ascending_ = dLat > 0; dirKnown_ = true; }
    }
    lat_ = latDeg; lon_ = lonDeg; fixMs_ = nowMs; hasFix_ = true;
    computeU();
}

void IssOrbit::positionAt(int32_t dtSec, float& latDeg, float& lonDeg) const {
    if (!hasFix_) { latDeg = lonDeg = 0; return; }
    const float dt = (float)dtSec;
    const float n = 2.0f * (float)M_PI / PERIOD_S;
    const float u = u0_ + n * dt;
    const float ci = cosf(INC), si = sinf(INC);
    float s = si * sinf(u);
    latDeg = asinf(s) / RAD;
    const float lam = atan2f(ci * sinf(u), cosf(u));
    const float lam0 = atan2f(ci * sinf(u0_), cosf(u0_));
    float lon = lon_ + (wrapPi(lam - lam0) - OMEGA_E * dt) / RAD;
    while (lon > 180.0f) lon -= 360.0f;
    while (lon < -180.0f) lon += 360.0f;
    lonDeg = lon;
}
