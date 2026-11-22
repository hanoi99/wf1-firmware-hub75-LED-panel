#include "WarpScene.h"

WarpScene::WarpScene(DisplayDriver& display) : BaseScene(display) {
    addRange("speed", "Geschwindigkeit", 1, 10, 5);
    addToggle("trail", "Schweif", true);
    addSelect("color", "Farbe", "Weiß|Blau|Bunt", 1);
}

void WarpScene::respawn(Star& s) {
    do {
        s.x = (int16_t)((int)fm::rnd(121) - 60);
        s.y = (int16_t)((int)fm::rnd(121) - 60);
    } while (abs(s.x) < 4 && abs(s.y) < 4);
    s.z = (int32_t)(80 * 16 + fm::rnd(80 * 16));      // 1/16 units
    s.hue = (uint8_t)fm::rnd(256);
}

void WarpScene::begin() {
    for (auto& s : stars_) respawn(s);
}

bool WarpScene::project(const Star& s, int32_t z, int& px, int& py) {
    if (z < 16) return false;
    px = 32 + (int)((int32_t)s.x * 32 * 16 / z);
    py = 32 + (int)((int32_t)s.y * 32 * 16 / z);
    return px >= 0 && px < 64 && py >= 0 && py < 64;
}

void WarpScene::render(uint32_t) {
    d_.clear();
    auto& m = d_.matrix();
    const int dz = 6 + val(O_SPEED) * 7;
    for (auto& s : stars_) {
        s.z -= dz;
        int x, y;
        if (!project(s, s.z, x, y)) { respawn(s); continue; }
        const int near = constrain(255 - (int)(s.z / 8), 40, 255);   // brighter when closer
        fm::RGB c;
        switch (val(O_COLOR)) {
            case 1: c = {(uint8_t)(near * 5 / 10), (uint8_t)(near * 7 / 10), (uint8_t)near}; break;
            case 2: c = fm::hsv(s.hue, 200, (uint8_t)near); break;
            default: c = {(uint8_t)near, (uint8_t)near, (uint8_t)near}; break;
        }
        if (val(O_TRAIL)) {
            int tx, ty;
            if (project(s, s.z + dz * 4, tx, ty)) {
                m.drawLine(tx, ty, x, y, fm::to565(fm::scale(c, 110)));
            }
        }
        d_.px(x, y, c.r, c.g, c.b);
    }
}
