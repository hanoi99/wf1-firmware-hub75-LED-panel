#include "services/IssService.h"
// Simulated ISS source: sim_iss_feed() pushes position fixes into the same orbit model.
extern unsigned long sim_ms;
static IssOrbit g_orbit;
void IssService::begin() {}
void sim_iss_feed(float lat, float lon) { g_orbit.addFix(lat, lon, (uint32_t)sim_ms); }
bool IssService::snapshot(IssOrbit& out) const { out = g_orbit; return out.hasFix(); }
