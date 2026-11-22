#pragma once
#include <time.h>

struct SunPos {
    double declRad;     // solar declination
    double subLonDeg;   // longitude where the sun is at the zenith (-180..180)
};

// Low-precision solar position (error well below 0.1 degrees), plenty for a
// 64-pixel map. Uses double: only called about once per minute.
SunPos sunPosition(time_t utc);
