#include "FireScene.h"

FireScene::FireScene(DisplayDriver& display) : BaseScene(display) {
    addRange("flame", "Flammenhöhe", 1, 10, 6);
    addSelect("pal", "Farbe", "Feuer|Blau|Grün|Violett", 0);
    addToggle("logs", "Holzscheite", true);
    addToggle("sparks", "Funken", true);
    buildPalette();
}

void FireScene::begin() {
    memset(heat_, 0, sizeof(heat_));
    memset(sparks_, 0, sizeof(sparks_));
    buildPalette();
}

void FireScene::drawLogs() {
    auto log = [&](int x0, int y0, int x1, int y1) {
        const int n = 40;
        for (int i = 0; i <= n; ++i) {
            const int x = x0 + (x1 - x0) * i / n, y = y0 + (y1 - y0) * i / n;
            for (int t = 0; t < 3; ++t) {
                const int shade = 70 + t * 22 + fm::rnd(8);
                d_.px(x, y + t, shade, shade * 55 / 100, shade * 18 / 100);
            }
        }
        // glowing end caps
        d_.px(x0, y0 + 1, 255, 120, 20);
        d_.px(x1, y1 + 1, 255, 120, 20);
    };
    log(12, 58, 46, 62);
    log(18, 62, 52, 58);
}

void FireScene::render(uint32_t now) {
    const int flame = val(O_FLAME);
    const int coolMax = 13 - flame;                         // 12 (short) .. 3 (tall)

    // heat sources: strongest in the middle, only above the logs
    for (int x = 0; x < 64; ++x) {
        const int dist = abs(x - 32);
        if (dist > 17) { heat_[63][x] = heat_[62][x] = 0; continue; }
        const int w = 255 - dist * 11;
        heat_[63][x] = (uint8_t)((w * (175 + fm::rnd(81))) >> 8);
        heat_[62][x] = (uint8_t)((w * (150 + fm::rnd(70))) >> 8);
    }
    for (int y = 0; y < 62; ++y) {
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
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            const fm::RGB c = palette_[heat_[y][x]];
            d_.px(x, y, c.r, c.g, c.b);
        }

    if (val(O_SPARKS)) {
        if (fm::rnd(100) < 18) {
            for (auto& s : sparks_) if (s.life == 0) {
                s.x = (int16_t)((26 + fm::rnd(12)) * 256); s.y = (int16_t)(50 * 256);
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
    if (val(O_LOGS)) drawLogs();
}
