#pragma once

#include "display/DisplayDriver.h"
#include "display/FastMath.h"

// Tiny helpers for procedural scenes.
namespace poly {

// Fills a convex polygon (scanline), shifted by (ox, oy).
inline void fill(DisplayDriver& d, const int (*p)[2], int n, fm::RGB c, int ox = 0, int oy = 0) {
    int ymin = 9999, ymax = -9999;
    for (int i = 0; i < n; ++i) { ymin = min(ymin, p[i][1]); ymax = max(ymax, p[i][1]); }
    for (int y = ymin; y <= ymax; ++y) {
        int xl = 9999, xr = -9999;
        for (int i = 0; i < n; ++i) {
            const int x0 = p[i][0], y0 = p[i][1], x1 = p[(i + 1) % n][0], y1 = p[(i + 1) % n][1];
            if (y0 == y1) {
                if (y == y0) { xl = min(xl, min(x0, x1)); xr = max(xr, max(x0, x1)); }
                continue;
            }
            if (y < min(y0, y1) || y > max(y0, y1)) continue;
            const int x = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            xl = min(xl, x); xr = max(xr, x);
        }
        for (int x = xl; x <= xr; ++x) d.px(x + ox, y + oy, c.r, c.g, c.b);
    }
}

inline void rect(DisplayDriver& d, int x, int y, int w, int h, fm::RGB c) {
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) d.px(x + i, y + j, c.r, c.g, c.b);
}

inline void disc(DisplayDriver& d, int cx, int cy, int r, fm::RGB c) {
    for (int y = -r; y <= r; ++y)
        for (int x = -r; x <= r; ++x)
            if (x * x + y * y <= r * r) d.px(cx + x, cy + y, c.r, c.g, c.b);
}

inline uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

}  // namespace poly
