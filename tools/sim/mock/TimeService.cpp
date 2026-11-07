// Simulierte Zeit: epoch0 + sim_ms
#include "services/TimeService.h"
#include <cstdlib>
time_t sim_epoch0 = 1793000000;   // wird von sim_main gesetzt
static time_t now() { return sim_epoch0 + sim_ms / 1000; }
void TimeService::begin() { setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); tzset(); }
void TimeService::update() {}
bool TimeService::isValid() const { return sim_epoch0 != 0; }
time_t TimeService::utcNow() const { return sim_epoch0 ? now() : 0; }
bool TimeService::getLocalTime(struct tm& r) const { if (!sim_epoch0) return false; time_t t = now(); localtime_r(&t, &r); return true; }
