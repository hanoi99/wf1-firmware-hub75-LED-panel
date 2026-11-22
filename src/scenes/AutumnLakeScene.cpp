#include "AutumnLakeScene.h"

namespace {
enum : uint8_t { SKY0 = 0 /*..15*/, HILL_FAR = 16, HILL_NEAR = 17, GROUND = 18, TRUNK = 19, TRUNK_LIT = 20,
                 LEAF0 = 21 /*..28*/, SUN = 29, GLOW0 = 32 /*..39*/ };

struct Mood { fm::RGB top, mid, hor, hillFar, hillNear, ground, sun; int sunX, sunY, glow; };
const Mood MOODS[3] = {
    {{35, 30, 95}, {200, 80, 110}, {255, 170, 90}, {105, 60, 100}, {62, 40, 72}, {44, 32, 28}, {255, 235, 170}, 47, 37, 10},   // sunset
    {{60, 90, 160}, {230, 150, 160}, {255, 215, 150}, {120, 115, 160}, {75, 85, 115}, {60, 52, 46}, {255, 245, 200}, 49, 38, 9}, // dawn
    {{50, 120, 225}, {110, 180, 240}, {185, 225, 250}, {95, 140, 165}, {62, 112, 88}, {62, 84, 36}, {255, 255, 235}, 53, 9, 4},   // day
};
const fm::RGB LEAF[8] = {{130, 30, 15}, {170, 42, 20}, {205, 72, 20}, {230, 112, 25},
                         {240, 152, 35}, {245, 192, 52}, {200, 90, 20}, {150, 52, 15}};

uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}
}

AutumnLakeScene::AutumnLakeScene(DisplayDriver& display)
    : BaseScene(display), img_(scratch::block().bytes), pal_(scratch::block().palette) {
    addSelect("mood", "Stimmung", "Abendrot|Morgenlicht|Sonniger Tag", 0);
    addRange("leaves", "Fallendes Laub", 0, 12, 6);
    addRange("ripple", "Wellen", 1, 10, 4);
}

void AutumnLakeScene::begin() {
    scratch::claim(this);
    memset(leaves_, 0, sizeof(leaves_));
    build();
}

void AutumnLakeScene::buildPalette() {
    const Mood& m = MOODS[val(O_MOOD)];
    for (int i = 0; i < 16; ++i) {          // sky: top -> mid (first 60 %) -> horizon
        const int t = i * 255 / 15;
        pal_[SKY0 + i] = t < 150 ? fm::mix(m.top, m.mid, (uint8_t)(t * 255 / 150)) : fm::mix(m.mid, m.hor, (uint8_t)((t - 150) * 255 / 105));
    }
    pal_[HILL_FAR] = m.hillFar; pal_[HILL_NEAR] = m.hillNear; pal_[GROUND] = m.ground;
    pal_[TRUNK] = {58, 32, 18}; pal_[TRUNK_LIT] = {96, 58, 30};
    const int dim = val(O_MOOD) == 2 ? 256 : (val(O_MOOD) == 1 ? 200 : 170);   // leaves are darker in weak light
    for (int i = 0; i < 8; ++i) pal_[LEAF0 + i] = fm::scale(LEAF[i], (uint8_t)min(255, dim));
    pal_[SUN] = m.sun;
    for (int k = 0; k < 8; ++k) pal_[GLOW0 + k] = fm::mix(m.hor, m.sun, (uint8_t)(k * (m.glow > 5 ? 36 : 18)));
}

void AutumnLakeScene::build() {
    buildPalette();
    const Mood& m = MOODS[val(O_MOOD)];
    // sky + sun glow
    for (int y = 0; y < HORIZON; ++y)
        for (int x = 0; x < 64; ++x) {
            uint8_t idx = (uint8_t)(SKY0 + y * 15 / (HORIZON - 1));
            const int dx = x - m.sunX, dy = y - m.sunY;
            const int d2 = dx * dx + dy * dy;
            if (d2 <= 5) idx = SUN;
            else if (d2 < m.glow * m.glow) idx = (uint8_t)(GLOW0 + 7 - min(7, (int)(fm::isqrt((uint32_t)d2) * 8 / m.glow)));
            img_[y * 64 + x] = idx;
        }
    // far and near hills
    for (int x = 0; x < 64; ++x) {
        const int hf = 33 + ((int)fm::sin8((uint8_t)(x * 5 + 20)) - 128) / 36;
        const int hn = 39 + ((int)fm::sin8((uint8_t)(x * 7 + 90)) - 128) / 40;
        for (int y = hf; y < HORIZON; ++y) img_[y * 64 + x] = HILL_FAR;
        for (int y = hn; y < HORIZON; ++y) img_[y * 64 + x] = HILL_NEAR;
    }
    // headland with the tree
    for (int y = 39; y < HORIZON; ++y)
        for (int x = 0; x < 42; ++x) {
            const int dx = x - 17, dy = y - 46;
            if (dx * dx * 49 + dy * dy * 484 <= 484 * 49) img_[y * 64 + x] = GROUND;   // ellipse rx 22, ry 7
        }
    // trunk with a slight curve, and branches
    for (int y = 45; y >= 27; --y) {
        const int cx = 17 + ((int)fm::sin8((uint8_t)(y * 9)) - 128) / 85;
        const int w = y > 38 ? 3 : 2;
        for (int t = 0; t < w; ++t) img_[y * 64 + cx + t] = t == 0 ? TRUNK_LIT : TRUNK;
    }
    auto branch = [&](int x0, int y0, int x1, int y1) {
        const int n = max(abs(x1 - x0), abs(y1 - y0));
        for (int i = 0; i <= n; ++i) img_[(y0 + (y1 - y0) * i / n) * 64 + x0 + (x1 - x0) * i / n] = TRUNK;
    };
    branch(17, 33, 10, 25); branch(18, 32, 26, 24); branch(18, 30, 18, 18);
    // canopy: clumped leaf colours
    struct C { int x, y, r; };
    static const C canopy[] = {{18, 19, 8}, {10, 23, 6}, {26, 23, 6}, {14, 14, 6}, {23, 14, 6}, {18, 27, 5}, {7, 28, 3}, {29, 29, 3}};
    for (int y = 4; y < 34; ++y)
        for (int x = 0; x < 40; ++x) {
            bool in = false;
            for (const C& c : canopy) { const int dx = x - c.x, dy = y - c.y; if (dx * dx + dy * dy <= c.r * c.r) { in = true; break; } }
            if (!in) continue;
            const uint32_t cell = hash2(x / 3, y / 3), px = hash2(x, y);
            int k = (int)(cell % 6) + ((y < 18) ? 1 : 0);            // upper leaves are lighter
            if ((px & 7) == 0) k += (px & 8) ? 1 : -1;               // a little speckle
            k = constrain(k, 0, 7);
            img_[y * 64 + x] = (uint8_t)(LEAF0 + k);
        }
}

void AutumnLakeScene::render(uint32_t now) {
    // still image
    for (int y = 0; y < HORIZON; ++y)
        for (int x = 0; x < 64; ++x) {
            const fm::RGB c = pal_[img_[y * 64 + x]];
            d_.px(x, y, c.r, c.g, c.b);
        }
    // water: mirror image with ripples, darker and bluer with depth
    const int amp = val(O_RIPPLE);
    for (int y = HORIZON; y < 64; ++y) {
        const int srcY = 2 * HORIZON - 1 - y;
        const int depth = (y - HORIZON) * 255 / 17;
        const int shift = ((int)fm::sin8((uint8_t)(y * 17 + now / 9)) - 128) * amp / 140;
        for (int x = 0; x < 64; ++x) {
            const int sx = constrain(x + shift + (((int)fm::sin8((uint8_t)(x * 9 + y * 5 + now / 14)) - 128) * amp / 400), 0, 63);
            fm::RGB c = pal_[img_[srcY * 64 + sx]];
            c = fm::scale(c, 185);
            c = fm::mix(c, fm::RGB{14, 32, 62}, (uint8_t)(36 + depth / 4));
            const int glint = (int)fm::sin8((uint8_t)(x * 11 + y * 29 + now / 11)) + (int)fm::sin8((uint8_t)(x * 5 - y * 13 + now / 17));
            if (glint > 492 - amp * 2) c = fm::mix(c, fm::RGB{255, 225, 170}, 55);
            d_.px(x, y, c.r, c.g, c.b);
        }
    }
    // falling leaves
    const int want = val(O_LEAVES);
    int alive = 0;
    for (auto& l : leaves_) if (l.life) ++alive;
    if (alive < want && fm::rnd(100) < 7) {
        for (auto& l : leaves_) if (l.life == 0) {
            l.x = (int16_t)((8 + fm::rnd(22)) * 256); l.y = (int16_t)((14 + fm::rnd(16)) * 256);
            l.vx = (int16_t)((int)fm::rnd(40) - 12); l.vy = (int16_t)(30 + fm::rnd(30));
            l.col = (uint8_t)(LEAF0 + fm::rnd(8)); l.life = 255;
            break;
        }
    }
    for (auto& l : leaves_) {
        if (!l.life) continue;
        l.x += l.vx + ((int)fm::sin8((uint8_t)(now / 18 + l.col * 20)) - 128) / 3;
        l.y += l.vy;
        const int px = l.x >> 8, py = l.y >> 8;
        if (py >= HORIZON + 2 + (int)(l.col & 7) || px < 0 || px > 63) { l.life = 0; continue; }
        const fm::RGB c = py >= HORIZON ? fm::scale(pal_[l.col], 150) : pal_[l.col];
        d_.px(px, py, c.r, c.g, c.b);
        if (py < HORIZON && (l.col & 1)) d_.px(px + 1, py, c.r, c.g, c.b);
    }
}
