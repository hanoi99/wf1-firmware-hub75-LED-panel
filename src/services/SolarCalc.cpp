#include "SolarCalc.h"
#include <math.h>

SunPos sunPosition(time_t utc) {
    const double rad = M_PI / 180.0;
    const double n = (double)utc / 86400.0 - 10957.5;  // days since J2000.0
    double L = fmod(280.460 + 0.9856474 * n, 360.0);
    double g = fmod(357.528 + 0.9856003 * n, 360.0) * rad;
    const double lambda = (L + 1.915 * sin(g) + 0.020 * sin(2 * g)) * rad;
    const double eps = (23.439 - 0.0000004 * n) * rad;
    const double decl = asin(sin(eps) * sin(lambda));
    const double ra = atan2(cos(eps) * sin(lambda), cos(lambda));
    double gmst = fmod(280.46061837 + 360.98564736629 * n, 360.0);
    double lon = ra / rad - gmst;
    lon = fmod(lon + 540.0, 360.0) - 180.0;
    if (lon < -180.0) lon += 360.0;
    return {decl, lon};
}
