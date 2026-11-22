#include "MoonIslandScene.h"

namespace {
constexpr int MOON_X = 50, MOON_Y = 11, MOON_R = 5;
constexpr int ISL_X = 17, ISL_W = 14;          // island centre and half width
constexpr int PALM_X = 19, PALM_TOP = 17;       // crown of the palm

bool palmFrond(int x, int y) {
    // five fronds drawn as short drooping arcs from the crown
    static const int8_t DX[5] = {-8, -5, 0, 5, 8}, DY[5] = {4, -2, -4, -2, 4};
    for (int f = 0; f < 5; ++f) {
        for (int i = 0; i <= 8; ++i) {
            const int fx = PALM_X + DX[f] * i / 8;
            const int fy = PALM_TOP + DY[f] * i / 8 + (i * i) / 14;   // droop
            if (x == fx && y == fy) return true;
            if (f != 2 && x == fx && y == fy + 1 && i > 2) return true;
        }
    }
    return false;
}
}

MoonIslandScene::MoonIslandScene(DisplayDriver& display) : BaseScene(display) {
    addRange("waves", "Wellen", 1, 10, 5);
    addToggle("stars", "Sterne", true);
    addToggle("clouds", "Wolken", true);
}

void MoonIslandScene::begin() {
    for (auto& s : stars_) {
        s.x = (uint8_t)fm::rnd(64); s.y = (uint8_t)fm::rnd(30);
        s.phase = (uint8_t)fm::rnd(256); s.base = (uint8_t)(60 + fm::rnd(120));
        const int dx = s.x - MOON_X, dy = s.y - MOON_Y;
        if (dx * dx + dy * dy < 100) s.y = (uint8_t)(s.y + 14);      // keep the moon's surroundings clear
    }
    cloudX_[0] = 8; cloudX_[1] = 44;
}

bool MoonIslandScene::island(int x, int y) const {
    const int dx = x - ISL_X;
    if (abs(dx) <= ISL_W) {
        const int top = HORIZON - 1 - (8 * (ISL_W * ISL_W - dx * dx)) / (ISL_W * ISL_W);
        if (y >= top && y < HORIZON) return true;
    }
    // palm trunk: slightly bent line from the island to the crown
    if (y >= PALM_TOP && y < HORIZON - 4) {
        const int tx = 16 + (HORIZON - 4 - y) * 3 / (HORIZON - 4 - PALM_TOP) + ((HORIZON - 4 - y) > 8 ? 0 : 0);
        if (x == tx) return true;
    }
    return y < HORIZON && palmFrond(x, y);
}

void MoonIslandScene::render(uint32_t now) {
    const int waves = val(O_WAVES);
    // night sky: darker on top, a touch of blue haze at the horizon
    for (int y = 0; y < HORIZON; ++y) {
        const fm::RGB c = fm::mix(fm::RGB{4, 6, 24}, fm::RGB{22, 34, 78}, (uint8_t)(y * 255 / HORIZON));
        for (int x = 0; x < 64; ++x) d_.px(x, y, c.r, c.g, c.b);
    }
    // moon glow, disc and a few craters
    for (int y = MOON_Y - 11; y <= MOON_Y + 11; ++y)
        for (int x = MOON_X - 11; x <= MOON_X + 11; ++x) {
            if (x < 0 || x > 63 || y < 0) continue;
            const int d2 = (x - MOON_X) * (x - MOON_X) + (y - MOON_Y) * (y - MOON_Y);
            if (d2 > 121) continue;
            if (d2 <= MOON_R * MOON_R) {
                const bool crater = (x == MOON_X - 2 && y == MOON_Y - 1) || (x == MOON_X + 1 && y == MOON_Y + 2) || (x == MOON_X + 2 && y == MOON_Y - 2);
                d_.px(x, y, crater ? 200 : 240, crater ? 200 : 240, crater ? 185 : 215);
            } else {
                const int g = (121 - d2) / 5;            // soft halo
                d_.px(x, y, 22 + g, 34 + g, 78 + g * 2);
            }
        }
    if (val(O_STARS)) {
        for (const Star& s : stars_) {
            const int v = s.base * (150 + fm::sin8((uint8_t)(s.phase + now / 20)) / 2) / 255;
            d_.px(s.x, s.y, (uint8_t)min(255, v), (uint8_t)min(255, v), (uint8_t)min(255, v + 20));
        }
    }
    if (val(O_CLOUDS)) {
        static const int8_t CY[2] = {20, 9};
        for (int c = 0; c < 2; ++c) {
            if (now % 300 < 60) {}   // (drift handled below)
            const int cx = ((cloudX_[c] * 10 + (int)(now / (c ? 900 : 1500))) % 110) - 20;
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -9; dx <= 9; ++dx) {
                    if (dx * dx * 4 + dy * dy * 36 > 4 * 81) continue;
                    const int x = cx + dx, y = CY[c] + dy;
                    if (x < 0 || x > 63) continue;
                    const int moonLit = max(0, 60 - (abs(x - MOON_X) + abs(y - MOON_Y)) * 2);
                    d_.px(x, y, 34 + moonLit / 2, 42 + moonLit / 2, 78 + moonLit / 2);
                }
        }
    }
    // island silhouette
    for (int y = 4; y < HORIZON; ++y)
        for (int x = 0; x < 40; ++x)
            if (island(x, y)) d_.px(x, y, 6, 14, 12);
    for (int x = ISL_X - ISL_W; x <= ISL_X + ISL_W; ++x) d_.px(x, HORIZON - 1, 54, 48, 38);   // sand line at the water
    // sea
    for (int y = HORIZON; y < 64; ++y) {
        const int depth = (y - HORIZON) * 255 / (64 - HORIZON);
        const fm::RGB base = fm::mix(fm::RGB{22, 44, 96}, fm::RGB{5, 12, 36}, (uint8_t)depth);
        const int refY = 2 * HORIZON - 1 - y;
        const int shift = ((int)fm::sin8((uint8_t)(y * 19 + now / (14 - waves))) - 128) * waves / 90;
        for (int x = 0; x < 64; ++x) {
            fm::RGB c = base;
            const int wv = (int)fm::sin8((uint8_t)(x * 9 + y * 17 + now * waves / 40)) + (int)fm::sin8((uint8_t)(x * 5 - y * 11 + now * waves / 55));
            if (wv > 400) c = fm::mix(c, fm::RGB{90, 125, 190}, 90);                  // moonlit crests
            // moon glitter column: wider with distance from the horizon
            const int half = 2 + (y - HORIZON) / 5;
            if (abs(x - MOON_X) <= half && wv > 350 - (half - abs(x - MOON_X)) * 18) c = fm::mix(c, fm::RGB{235, 235, 200}, (uint8_t)(90 + (half - abs(x - MOON_X)) * 12));
            // island mirror image
            if (refY >= 4 && y < HORIZON + 14 && island(x + shift, refY)) c = fm::mix(c, fm::RGB{4, 10, 10}, (uint8_t)(200 - (y - HORIZON) * 9));
            d_.px(x, y, c.r, c.g, c.b);
        }
    }
}
