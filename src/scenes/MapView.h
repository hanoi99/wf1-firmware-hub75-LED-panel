#pragma once
// Shared world-map renderer (64x26 equirectangular) with day/night shading and a
// terminator curve. Used by the world clock and the ISS tracker.
#include <math.h>
#include <time.h>
#include "data/WorldMap.h"
#include "display/DisplayDriver.h"
#include "display/FastMath.h"
#include "services/SolarCalc.h"

class MapView {
public:
    static constexpr int W = WorldMap::W;
    static constexpr int H = WorldMap::H;

    // Recomputes the shading for the given UTC time (utc == 0: no shading).
    void setTime(time_t utc) {
        hasSun_ = utc != 0;
        if (!hasSun_) return;
        const SunPos sun = sunPosition(utc);
        subLon_ = (float)sun.subLonDeg;
        decl_ = (float)sun.declRad;
        const float sd = sinf(decl_), cd = cosf(decl_);
        float cosH[W];
        for (int x = 0; x < W; ++x) cosH[x] = cosf((lonOf(x) - subLon_) * 0.0174533f);
        for (int y = 0; y < H; ++y) {
            const float lat = latOf(y) * 0.0174533f;
            const float sp = sinf(lat) * sd, cp = cosf(lat) * cd;
            for (int x = 0; x < W; ++x) {
                const float s = sp + cp * cosH[x];
                sinEl_[y][x] = (int8_t)constrain((int)lroundf(s * 127.0f), -127, 127);
            }
        }
    }

    void draw(DisplayDriver& d, bool night, bool terminator, bool sun) const {
        static const fm::RGB DAY_SEA{18, 60, 150}, DAY_LAND{70, 170, 70};
        static const fm::RGB NIGHT_SEA{3, 6, 24}, NIGHT_LAND{16, 34, 20};
        const bool shade = night && hasSun_;
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const bool land = WorldMap::isLand(x, y);
                fm::RGB c = land ? DAY_LAND : DAY_SEA;
                if (shade) {
                    const int s = sinEl_[y][x];  // twilight ramp from sin(el) -0.11 .. +0.05
                    const uint8_t f = (uint8_t)constrain(((s + 14) * 255) / 20, 0, 255);
                    c = fm::mix(land ? NIGHT_LAND : NIGHT_SEA, c, f);
                }
                if (terminator && hasSun_ && onTerminator(x, y)) c = {255, 180, 50};
                d.px(x, y, c.r, c.g, c.b);
            }
        }
        if (sun && hasSun_) {
            int sx, sy;
            if (project(decl_ / 0.0174533f, subLon_, sx, sy)) {
                d.px(sx, sy, 255, 240, 80);
                d.px(sx - 1, sy, 255, 200, 40); d.px(sx + 1, sy, 255, 200, 40);
                d.px(sx, sy - 1, 255, 200, 40); d.px(sx, sy + 1, 255, 200, 40);
            }
        }
    }

    static bool project(float lat, float lon, int& x, int& y) {
        x = (int)floorf((lon + 180.0f) * W / 360.0f);
        y = (int)floorf((WorldMap::LAT_TOP - lat) * H / (WorldMap::LAT_TOP - WorldMap::LAT_BOT));
        if (x < 0) x += W;
        if (x >= W) x -= W;
        return y >= 0 && y < H;
    }
    static bool landAt(float lat, float lon) {
        int x, y;
        return project(lat, lon, x, y) && WorldMap::isLand(x, y);
    }
    bool hasSun() const { return hasSun_; }

private:
    static float lonOf(int x) { return -180.0f + (x + 0.5f) * 360.0f / W; }
    static float latOf(int y) { return WorldMap::LAT_TOP - (y + 0.5f) * (WorldMap::LAT_TOP - WorldMap::LAT_BOT) / H; }
    bool onTerminator(int x, int y) const {
        const bool a = sinEl_[y][x] < 0;
        if (x + 1 < W && a != (sinEl_[y][x + 1] < 0)) return true;
        if (y + 1 < H && a != (sinEl_[y + 1][x] < 0)) return true;
        return false;
    }
    int8_t sinEl_[H][W]{};
    float subLon_ = 0, decl_ = 0;
    bool hasSun_ = false;
};
