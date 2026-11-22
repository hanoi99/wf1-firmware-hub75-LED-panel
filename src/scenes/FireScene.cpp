#include "FireScene.h"

FireScene::FireScene(DisplayDriver& display)
    : BaseScene(display),
      heat_(*reinterpret_cast<uint8_t (*)[64][64]>(scratch::block().bytes)),
      palette_(scratch::block().palette) {
    addRange("flame", "Flammenhöhe", 1, 10, 6);
    addSelect("pal", "Farbe", "Feuer|Blau|Grün|Violett", 0);
    addSelect("logs", "Holz", "Kein Holz|Zwei Scheite|Holzstapel", 2);
    addToggle("sparks", "Funken", true);
    buildPalette();
}

void FireScene::begin() {
    scratch::claim(this);
    memset(heat_, 0, sizeof(heat_));
    memset(sparks_, 0, sizeof(sparks_));
    buildPalette();
}

namespace {
// Thick (3 px) log from (x0,y0) to (x1,y1); bark shading across the thickness.
void thickLog(DisplayDriver& d, int x0, int y0, int x1, int y1, bool capGlow) {
    const bool steep = abs(y1 - y0) > abs(x1 - x0);
    const int n = max(abs(x1 - x0), abs(y1 - y0)) * 2 + 1;
    for (int i = 0; i <= n; ++i) {
        const int x = x0 + (x1 - x0) * i / n, y = y0 + (y1 - y0) * i / n;
        for (int t = 0; t < 3; ++t) {
            const int shade = 84 + (t == 1 ? 52 : t == 0 ? 0 : 22) + fm::rnd(8);
            d.px(steep ? x + t : x, steep ? y : y + t, shade, shade * 52 / 100, shade * 16 / 100);
        }
    }
    if (capGlow) {   // glowing, charred ends
        d.px(x0, y0 + 1, 255, 110, 20); d.px(x1, y1 + 1, 255, 110, 20);
    }
}
}

// Where the flames start (row of the heat source), depending on the wood.
int FireScene::sourceRow() const {
    const int w = val(O_LOGS);
    return w == 2 ? 55 : w == 1 ? 58 : 61;
}

void FireScene::drawLogs() {
    const int w = val(O_LOGS);
    if (w == 1) {                      // two crossed logs
        thickLog(d_, 12, 59, 46, 63, true);
        thickLog(d_, 18, 63, 52, 59, true);
    } else if (w == 2) {               // log pile: two base logs, a tipi of two leaning logs, one on top
        thickLog(d_, 8, 61, 44, 62, true);
        thickLog(d_, 20, 62, 56, 61, true);
        thickLog(d_, 21, 63, 30, 55, false);
        thickLog(d_, 43, 63, 34, 55, false);
        thickLog(d_, 14, 58, 50, 59, true);
        // embers glowing between the logs
        for (int i = 0; i < 9; ++i) {
            const int x = 24 + fm::rnd(17), y = 59 + (int)fm::rnd(4);
            d_.px(x, y, 255, (uint8_t)(70 + fm::rnd(90)), 10);
        }
    }
}

void FireScene::render(uint32_t now) {
    const int flame = val(O_FLAME);
    const int coolMax = 13 - flame;                         // 12 (short) .. 3 (tall)

    // heat sources sit on top of the wood; nothing burns below it
    const int src = sourceRow();
    const int reach = val(O_LOGS) == 2 ? 13 : 17;
    for (int x = 0; x < 64; ++x) {
        const int dist = abs(x - 32);
        if (dist > reach) { heat_[src][x] = heat_[src - 1][x] = 0; continue; }
        const int w = 255 - dist * (reach == 13 ? 14 : 11);
        heat_[src][x] = (uint8_t)((w * (175 + fm::rnd(81))) >> 8);
        heat_[src - 1][x] = (uint8_t)((w * (150 + fm::rnd(70))) >> 8);
    }
    for (int y = 0; y < src - 1; ++y) {
        const int sway = ((int)fm::sin8((uint8_t)(now / 30 + y * 6)) - 128) / 56;   // -2..+2 px
        for (int x = 0; x < 64; ++x) {
            int xs = x + sway;
            const int l = max(0, xs - 1), c = constrain(xs, 0, 63), r = min(63, xs + 1);
            const int sum = heat_[y + 1][l] + heat_[y + 1][c] + heat_[y + 1][r] + heat_[min(63, y + 2)][c];
            const int dx = abs(x - 32);
            const int cool = (int)fm::rnd(coolMax + 1) + (dx > 14 ? (dx - 14) / 2 : 0);
            const int v = sum / 4 - cool;
            heat_[y][x] = (uint8_t)(v > 0 ? v : 0);
        }
    }
    for (int y = 0; y <= src; ++y)
        for (int x = 0; x < 64; ++x) {
            const fm::RGB c = palette_[heat_[y][x]];
            d_.px(x, y, c.r, c.g, c.b);
        }

    if (val(O_SPARKS)) {
        if (fm::rnd(100) < 18) {
            for (auto& s : sparks_) if (s.life == 0) {
                s.x = (int16_t)((26 + fm::rnd(12)) * 256); s.y = (int16_t)((src - 4) * 256);
                s.vx = (int16_t)((int)fm::rnd(120) - 60); s.vy = (int16_t)(-(110 + (int)fm::rnd(130)));
                s.life = (uint8_t)(20 + fm::rnd(30));
                break;
            }
        }
        for (auto& s : sparks_) {
            if (s.life == 0) continue;
            --s.life;
            s.x += s.vx + ((int)fm::rnd(61) - 30);
            s.y += s.vy;
            const int px = s.x >> 8, py = s.y >> 8;
            if (py < 0) { s.life = 0; continue; }
            const uint8_t f = (uint8_t)min(255, s.life * 9);
            d_.px(px, py, 255, (uint8_t)(60 + f / 2), (uint8_t)(f / 8));
        }
    }
    for (int y = src + 1; y < 64; ++y)           // below the wood: clear leftovers of the previous scene
        for (int x = 0; x < 64; ++x) d_.px(x, y, 0, 0, 0);
    drawLogs();
}
